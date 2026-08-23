/*
 * task_actuator_interface.h
 *
 * Provides the API to push events to the Actuator Task.
 */

#ifndef TASK_ACTUATOR_INTERFACE_H_
#define TASK_ACTUATOR_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "task_actuator_attribute.h"
#include <stdbool.h>

void init_event_task_actuator(void);
void put_event_task_actuator(task_actuator_ev_t event);
task_actuator_ev_t get_event_task_actuator(void);
bool any_event_task_actuator(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_ACTUATOR_INTERFACE_H_ */
