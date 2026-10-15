#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "telemetry.h"

static const char* speed_names[] = {
    "OFF / IDLE", "STIR", "SLOW MIX", "LOW FOLD", "MIX / BEAT", 
    "CREAMING LOW", "CREAMING HIGH", "FAST MIX", "FAST BEATING", 
    "WHIP LOW", "FAST WHIP"
};

void telemetry_init(void)
{
    printk("[APPLIANCE] Stand Mixer Supervisory System Online.\n");
}

void telemetry_report_status(const mixer_status_t *status)
{
    if (!status->is_powered_on) {
        // power off
    } else if (status->state == MIXER_STATE_IDLE) {
        printk("[MOTOR IDLE] Motor stopped (0 RPM, 0%% PWM).\n");
    } else if (status->state == MIXER_STATE_RAMPING) {
        printk("[MOTOR RAMP] -> Current Duty: %d%% | RPM: %d\n", status->current_duty_percent, status->current_rpm);
    } else if (status->state == MIXER_STATE_RUNNING) {
        const char *name = "UNKNOWN";
        if (status->requested_speed <= 10) name = speed_names[status->requested_speed];
        printk("[MOTOR STEADY] Speed %d [%s] Stable at %d RPM (Duty: %d%%)\n",
               status->requested_speed, name, status->current_rpm, status->current_duty_percent);
    }
}
