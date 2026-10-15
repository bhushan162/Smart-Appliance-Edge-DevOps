#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "mixer_types.h"
#include "mixer_sm.h"
#include "slider_gpio.h"
#include "power_btn.h"
#include "motor_pwm.h"
#include "telemetry.h"

K_MSGQ_DEFINE(mixer_event_msgq, sizeof(mixer_event_t), 10, 4);

void input_scanner_thread(void *a, void *b, void *c)
{
    uint8_t last_speed = 255;
    bool last_power = false;

    while (1) {
        bool current_power = power_btn_is_on();
        if (current_power != last_power) {
            mixer_event_t evt = { .type = MIXER_EVENT_POWER_TOGGLE };
            evt.payload.power_on = current_power;
            k_msgq_put(&mixer_event_msgq, &evt, K_NO_WAIT);
            last_power = current_power;
        }

        uint8_t current_speed = slider_gpio_read();
        if (current_speed != last_speed) {
            mixer_event_t evt = { .type = MIXER_EVENT_SLIDER_CHANGE };
            evt.payload.slider_level = current_speed;
            k_msgq_put(&mixer_event_msgq, &evt, K_NO_WAIT);
            last_speed = current_speed;
            
            const char* names[] = {"OFF / IDLE", "STIR", "SLOW MIX", "LOW FOLD", "MIX / BEAT", 
                                   "CREAMING LOW", "CREAMING HIGH", "FAST MIX", "FAST BEATING", 
                                  "WHIP LOW", "FAST WHIP"};
            if (current_speed <= 10) {
                printk("[SLIDER EVENT] PB%d is HIGH -> Selected Speed %d (%s)\n", 
                       current_speed, current_speed, names[current_speed]);
            }
        }
        k_msleep(50);
    }
}

void motor_control_thread(void *a, void *b, void *c)
{
    while (1) {
        mixer_event_t evt;
        while (k_msgq_get(&mixer_event_msgq, &evt, K_NO_WAIT) == 0) {
            mixer_sm_handle_event(&evt);
        }

        mixer_sm_process_tick();
        
        mixer_status_t status;
        mixer_sm_get_status(&status);
        motor_pwm_set_duty(status.current_duty_percent);

        k_msleep(CONFIG_APP_RAMP_TICK_MS);
    }
}

void telemetry_thread(void *a, void *b, void *c)
{
    k_msleep(300);
    while (1) {
        mixer_status_t status;
        mixer_sm_get_status(&status);
        telemetry_report_status(&status);
        k_msleep(100); 
    }
}

K_THREAD_DEFINE(input_tid, 1024, input_scanner_thread, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(motor_tid, 1024, motor_control_thread, NULL, NULL, NULL, 2, 0, 0);
K_THREAD_DEFINE(telemetry_tid, 1024, telemetry_thread, NULL, NULL, NULL, 10, 0, 0);

int main(void)
{
    printk("Hello, Edge DevOps Pipeline!\n");
    printk("Booting on board: %s\n", CONFIG_BOARD);
    printk("Motor Supervisory Controller is Idle...\n");

    slider_gpio_init();
    power_btn_init();
    motor_pwm_init();
    telemetry_init();
    mixer_sm_init();

    return 0;
}
