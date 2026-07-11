#ifndef TASK_TELEMETRY_INTERFACE_H_
#define TASK_TELEMETRY_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "task_telemetry_attribute.h"

extern void init_event_task_telemetry(void);
extern void put_event_task_telemetry(task_telemetry_ev_t event);
extern task_telemetry_ev_t get_event_task_telemetry(void);
extern bool any_event_task_telemetry(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_TELEMETRY_INTERFACE_H_ */
