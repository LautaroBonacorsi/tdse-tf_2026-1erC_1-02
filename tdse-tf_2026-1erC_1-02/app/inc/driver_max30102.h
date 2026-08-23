/*
 * driver_max30102.h
 */

#ifndef DRIVER_MAX30102_H_
#define DRIVER_MAX30102_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

#define MAX30102_I2C_ADDR 0xAE

void max30102_init(I2C_HandleTypeDef *hi2c);
void max30102_update(void);

/* Callbacks para enlazar con HAL_I2C */
void max30102_rx_cplt_callback(I2C_HandleTypeDef *hi2c);
void max30102_error_callback(I2C_HandleTypeDef *hi2c);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_MAX30102_H_ */
