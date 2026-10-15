#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "mixer_sm.h"

#define MAX_SPEED CONFIG_APP_MAX_SPEED_LEVELS
#define RAMP_STEP CONFIG_APP_RAMP_STEP_PERCENT
#define THERMAL_CUTOFF CONFIG_APP_THERMAL_CUTOFF_TEMP_C

static const speed_profile_t SPEED_TABLE[] = {
    { 0,  "OFF / IDLE",        0,    0    },
    { 1,  "STIR",             10,   100  },
    { 2,  "SLOW MIX",         20,   250  },
    { 3,  "LOW FOLD",         30,   380  },
    { 4,  "MIX / BEAT",       40,   500  },
    { 5,  "CREAMING LOW",     50,   620  },
    { 6,  "CREAMING HIGH",    60,   750  },
    { 7,  "FAST MIX",         70,   880  },
    { 8,  "FAST BEATING",     80,   1000 },
    { 9,  "WHIP LOW",         90,   1150 },
    { 10, "FAST WHIP",        100,  1300 }
};


static mixer_status_t g_mixer;

void mixer_sm_init(void)
{
    g_mixer.state = MIXER_STATE_POWER_OFF;
    g_mixer.is_powered_on = false;
    g_mixer.requested_speed = 0;
    g_mixer.current_duty_percent = 0;
    g_mixer.current_rpm = 0;
    g_mixer.motor_temp_c = 25;
}

void mixer_sm_handle_event(const mixer_event_t *evt)
{
    if (evt->type == MIXER_EVENT_POWER_TOGGLE) {
        g_mixer.is_powered_on = evt->payload.power_on;
        if (!g_mixer.is_powered_on) {
            g_mixer.state = MIXER_STATE_POWER_OFF;
            printk("[SM] Power OFF Event received.\n");
        } else {
            g_mixer.state = MIXER_STATE_IDLE;
            printk("[SM] Power ON Event received.\n");
        }
    } 
    else if (evt->type == MIXER_EVENT_SLIDER_CHANGE) {
        uint8_t speed = evt->payload.slider_level;
        if (speed > MAX_SPEED) speed = MAX_SPEED;
        g_mixer.requested_speed = speed;
        printk("[SM] Slider changed to Speed %d (%s)\n", speed, SPEED_TABLE[speed].name);
    }
    else if (evt->type == MIXER_EVENT_THERMAL_UPDATE) {
        g_mixer.motor_temp_c = evt->payload.temperature_c;
    }
}

void mixer_sm_process_tick(void)
{
    // Thermal Check First
    if (g_mixer.motor_temp_c > THERMAL_CUTOFF) {
        g_mixer.state = MIXER_STATE_SAFETY_STOP;
        g_mixer.current_duty_percent = 0;
        g_mixer.current_rpm = 0;
        printk("[FATAL] Motor Overheat (%d C)! Safety Cutoff.\n", g_mixer.motor_temp_c);
        return;
    }

    if (!g_mixer.is_powered_on || g_mixer.state == MIXER_STATE_SAFETY_STOP) {
        g_mixer.current_duty_percent = 0;
        g_mixer.current_rpm = 0;
        return;
    }

    uint8_t target_duty = SPEED_TABLE[g_mixer.requested_speed].target_duty_percent;
    uint16_t target_rpm = SPEED_TABLE[g_mixer.requested_speed].estimated_rpm;

    if (g_mixer.current_duty_percent < target_duty) {
        g_mixer.state = MIXER_STATE_RAMPING;
        if (g_mixer.current_duty_percent + RAMP_STEP > target_duty) {
            g_mixer.current_duty_percent = target_duty;
        } else {
            g_mixer.current_duty_percent += RAMP_STEP;
        }
    } 
    else if (g_mixer.current_duty_percent > target_duty) {
        g_mixer.state = MIXER_STATE_RAMPING;
        if (g_mixer.current_duty_percent < RAMP_STEP) {
            g_mixer.current_duty_percent = 0;
        } else {
            g_mixer.current_duty_percent -= RAMP_STEP;
        }
    } 
    else {
        if (target_duty == 0) {
            g_mixer.state = MIXER_STATE_IDLE;
        } else {
            g_mixer.state = MIXER_STATE_RUNNING;
        }
    }

    g_mixer.current_rpm = (target_rpm * g_mixer.current_duty_percent) / (target_duty ? target_duty : 1);
}

void mixer_sm_get_status(mixer_status_t *out_status)
{
    if (out_status) {
        *out_status = g_mixer;
    }
}
