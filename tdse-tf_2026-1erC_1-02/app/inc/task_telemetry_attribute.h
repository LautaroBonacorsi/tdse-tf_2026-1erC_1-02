#ifndef TASK_TELEMETRY_ATTRIBUTE_H_
#define TASK_TELEMETRY_ATTRIBUTE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EV_TEL_IDLE,
    EV_TEL_SPO2_DATA,
    EV_TEL_SENSOR_ERR,
    EV_TEL_TX_CPLT,
    EV_TEL_RX_DATA
} task_telemetry_ev_t;

typedef enum {
    ST_TEL_IDLE,
    ST_TEL_TX
} task_telemetry_st_t;

typedef struct {
    uint32_t            tick;
    task_telemetry_st_t state;
    task_telemetry_ev_t event;
    bool                flag;
} task_telemetry_dta_t;

extern task_telemetry_dta_t task_telemetry_dta_list[];

#ifdef __cplusplus
}
#endif

#endif /* TASK_TELEMETRY_ATTRIBUTE_H_ */
