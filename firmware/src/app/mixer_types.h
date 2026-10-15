#ifndef MIXER_TYPES_H
#define MIXER_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Mixer Operational States */
typedef enum {
    MIXER_STATE_POWER_OFF = 0,
    MIXER_STATE_IDLE,
    MIXER_STATE_RAMPING,
    MIXER_STATE_RUNNING,
    MIXER_STATE_SAFETY_STOP
} mixer_state_t;

/* Event Types for State Machine */
typedef enum {
    MIXER_EVENT_POWER_TOGGLE = 0,
    MIXER_EVENT_SLIDER_CHANGE,
    MIXER_EVENT_THERMAL_UPDATE
} mixer_event_type_t;


/* Event Payload Structure */
typedef struct {
    mixer_event_type_t type;
    union {
        bool power_on;
        uint8_t slider_level;
        int16_t temperature_c;
    } payload;
} mixer_event_t;


/* Global Status Snapshot */
typedef struct {
    mixer_state_t state;
    bool is_powered_on;
    uint8_t requested_speed;
    uint8_t current_duty_percent;
    uint16_t current_rpm;
    int16_t motor_temp_c;
} mixer_status_t;

/* Speed Level Configuration */
typedef struct {
    uint8_t  level;
    const char *name;
    uint8_t  target_duty_percent;
    uint16_t estimated_rpm;
} speed_profile_t;


#endif /* MIXER_TYPES_H */
