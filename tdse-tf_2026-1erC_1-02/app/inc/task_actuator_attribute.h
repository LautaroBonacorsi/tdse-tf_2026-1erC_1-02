/*
 * task_actuator_attribute.h
 *
 * Defines the attributes (states and events) for the Actuator Task.
 */

#ifndef TASK_ACTUATOR_ATTRIBUTE_H_
#define TASK_ACTUATOR_ATTRIBUTE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Events received by Actuator Task */
typedef enum {
    EV_ACT_IDLE,
    EV_ACT_ALARM_OFF,
    EV_ACT_ALARM_SPO2_CRIT, // Red LED + Fast Beep
    EV_ACT_ALARM_PULSE_WARN // Yellow LED + Slow Beep
} task_actuator_ev_t;

/* States of the Actuator FSM */
typedef enum {
    ST_ACT_OFF,
    ST_ACT_WARNING,
    ST_ACT_CRITICAL
} task_actuator_st_t;

typedef struct {
    uint32_t tick;
    task_actuator_st_t state;
    task_actuator_ev_t event;
    bool flag;
} task_actuator_dta_t;

#ifdef __cplusplus
}
#endif

#endif /* TASK_ACTUATOR_ATTRIBUTE_H_ */
