#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

/* ======================================================================*/
/* Smart Stand Mixer Firmware - Hardware-Driven Motor Controller                  */
/* ======================================================================*/

#define    MAX_SPEED_LEVEL     10
#define    RAMP_STEP_PERCENT   5
#define    RAMP_TICK_MS        100
#define    MOTOR_INDICATOR_PIN 5

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
#define    HAS_GPIOB 1
static const struct device *const gpiob_dev = DEVICE_DT_GET(DT_NODELABEL(gpiob));
#else
#define    HAS_GPIOB 0
static const struct device *const gpiob_dev = NULL;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay)
#define    HAS_GPIOA 1
static const struct device *const gpioa_dev = DEVICE_DT_GET(DT_NODELABEL(gpioa));
#else
#define    HAS_GPIOA 0
static const struct device *const gpioa_dev = NULL;
#endif

typedef enum {
    MIXER_STATE_IDLE = 0,
    MIXER_STATE_RAMPING,
    MIXER_STATE_RUNNING,
    MIXER_STATE_SAFETY_STOP
} mixer_state_t;

typedef struct {
    uint8_t       level;
    const char    *name;
    uint8_t       target_duty_percent;
    uint16_t      estimated_rpm;
} speed_profile_t;

static const speed_profile_t SPEED_TABLE[MAX_SPEED_LEVEL + 1] = {
    { 0,  "OFF / IDLE",        0,    0    },
    { 1,  "STIR",             10,   100  },
    { 2,  "SLOW MIX",         20,   250  },
    { 3,  "LOW FOLD",         30,   380  },
    { 4,  "MIX / BEAT",       40,   500  },
    { 5,  "CREAMING LOW",     50,   620  },
    { 6,  "CREAMING HIGH",    60,   750  },
    { 7,  "FAST MIX",         70,   880  },
    { 8,  "FAST BEATING",     80,   1000 },
    { 9,  "WHIP LOW",         90,   1150  },
    { 10, "FASSWHIP",        100,  1300 }
};

typedef struct {
    mixer_state_t          state;
    uint8_t                requested_speed;
    uint8_t                current_duty_percent;
    uint16_t               current_rpm;
    int16_t                motor_temp_c;
} mixer_controller_t;

static mixer_controller_t g_mixer = {
    .state = MIXER_STATE_IDLE,
    .requested_speed = 0,
    .current_duty_percent = 0,
    .current_rpm = 0,
    .motor_temp_c = 25
};

static int hardware_init(void)
{
#if HAS_GPIOB
    if (!device_is_ready(gpiob_dev)) {
        printk("[ERR] GPIOB controller not ready!\n");
        return -1;
    }
    for (int pin = 0; pin <= MAX_SPEED_LEVEL; pin++) {
        int ret = gpio_pin_configure(gpiob_dev, pin, GPIO_INPUT | GPIO_PULL_DOWN);
        if (ret < 0) {
            printk("[WARN] Failed to configure PB%d (ret: %d)\n", pin, ret);
        }
    }
    printk("[HW] Configured PB0-PB10 as Slider GPIO Inputs (Pull-Down enabled).\n");
#endif

#if HAS_GPIOA
    if (device_is_ready(gpioa_dev)) {
        gpio_pin_configure(gpioa_dev, MOTOR_INDICATOR_PIN, GPIO_OUTPUT_INACTIVE);
        printk("[HW] Configured PA%d as Motor Driver output.\n", MOTOR_INDICATOR_PIN);
    }
#endif
    return 0;
}

static uint8_t read_slider_hardware(void)
{
#if HAS_GPIOB
    if (device_is_ready(gpiob_dev)) {
        for (int pin = MAX_SPEED_LEVEL; pin >= 1; pin--) {
            if (gpio_pin_get(gpiob_dev, pin) > 0) {
                return (uint8_t)pin;
            }
        }
    }
#endif
    return 0;
}

static void set_motor_actuator(bool running)
{
#if HAS_GPIOA
    if (device_is_ready(gpioa_dev)) {
        gpio_pin_set(gpioa_dev, MOTOR_INDICATOR_PIN, running ? 1 : 0);
    }
#endif
}

static void mixer_set_slider_speed(uint8_t speed)
{
    if (speed > MAX_SPEED_LEVEL) {
        speed = MAX_SPEED_LEVEL;
    }
    g_mixer.requested_speed = speed;
    printk("[SLIDER EVENT] PB%d is HIGH -> Selected Speed %d (%s) | Target: %d%% PWM\n",
           speed, speed, SPEED_TABLE[speed].name, SPEED_TABLE[speed].target_duty_percent);
}

static void mixer_process_motor(void)
{
    uint8_t target_duty = SPEED_TABLE[g_mixer.requested_speed].target_duty_percent;
    uint16_t target_rpm = SPEED_TABLE[g_mixer.requested_speed].estimated_rpm;

    if (g_mixer.motor_temp_c > 85) {
        g_mixer.state = MIXER_STATE_SAFETY_STOP;
        g_mixer.current_duty_percent = 0;
        g_mixer.current_rpm = 0;
        printk("[FATAL] Motor overheat (%d C > 85 C)! Emergency shutdown.\n", g_mixer.motor_temp_c);
        return;
    }

    if (g_mixer.current_duty_percent < target_duty) {
        g_mixer.state = MIXER_STATE_RAMPING;
        if (g_mixer.current_duty_percent + RAMP_STEP_PERCENT > target_duty) {
            g_mixer.current_duty_percent = target_duty;
        } else {
            g_mixer.current_duty_percent += RAMP_STEP_PERCENT;
        }
        g_mixer.current_rpm = (target_rpm * g_mixer.current_duty_percent) / (target_duty ? target_duty : 1);
        printk("[MOTOR RAMP] +%d%% -> Current Duty: %d%% | RPM: %d\n",
               RAMP_STEP_PERCENT, g_mixer.current_duty_percent, g_mixer.current_rpm);
    } else if (g_mixer.current_duty_percent > target_duty) {
        g_mixer.state = MIXER_STATE_RAMPING;
        if (g_mixer.current_duty_percent < RAMP_STEP_PERCENT) {
            g_mixer.current_duty_percent = 0;
        } else {
            g_mixer.current_duty_percent -= RAMP_STEP_PERCENT;
        }
        g_mixer.current_rpm = (target_rpm * g_mixer.current_duty_percent) / (target_duty ? target_duty : 1);
        printk("[MOTOR RAMP] -%d%% -> Current Duty: %d%% | RPM: %d\n",
               RAMP_STEP_PERCENT, g_mixer.current_duty_percent, g_mixer.current_rpm);
    } else {
        if (target_duty == 0) {
            if (g_mixer.state != MIXER_STATE_IDLE) {
                g_mixer.state = MIXER_STATE_IDLE;
                printk("[MOTOR IDLE] Motor stopped (0 RPM, 0%% PWM).\n");
            }
        } else {
            if (g_mixer.state != MIXER_STATE_RUNNING) {
                g_mixer.state = MIXER_STATE_RUNNING;
                printk("[MOTOR STEADY] Speed %d [%s] Stable at %d RPM (Duty: %d%%)\n",
                       g_mixer.requested_speed, SPEED_TABLE[g_mixer.requested_speed].name,
                       g_mixer.current_rpm, g_mixer.current_duty_percent);
            }
        }
    }
}

int main(void)
{
    printk("Hello, Edge DevOps Pipeline!\n");
    printk("Booting on board: %s\n", CONFIG_BOARD);
    printk("Motor Supervisory Controller is Idle...\n");

    printk("[APPLIANCE] Stand Mixer Supervisory System Online.\n");
    hardware_init();

    while (1) {
        uint8_t slider_pos = read_slider_hardware();
        if (slider_pos != g_mixer.requested_speed) {
            mixer_set_slider_speed(slider_pos);
        }

        mixer_process_motor();
        set_motor_actuator(g_mixer.current_duty_percent > 0);

        k_msleep(RAMP_TICK_MS);
    }
    return 0;
}
