/*
 * algorithm.h
 *
 * SpO2 & Heart Rate Algorithm 
 * Basado en Maxim Integrated y adaptado para enteros de 32 bits
 */

#ifndef ALGORITHM_H_
#define ALGORITHM_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "spo2_algorithm.h"

/* Tamaño de búfer para 4 segundos a 100 Hz */
#define ALGO_BUFFER_SIZE BUFFER_SIZE

/* Variables globales para la vista desde task_system */
extern int32_t g_algo_current_spo2;
extern int32_t g_algo_current_bpm;

/* Inicialización */
void algorithm_init(void);

/* Procesar una muestra nueva que llega desde el I2C */
void algorithm_process_sample(uint32_t red_sample, uint32_t ir_sample);

#ifdef __cplusplus
}
#endif

#endif /* ALGORITHM_H_ */
