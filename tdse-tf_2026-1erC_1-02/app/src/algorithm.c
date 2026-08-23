/*
 * algorithm.c
 *
 * Implementación del algoritmo de SpO2 y BPM (Maxim/Sparkfun).
 * Adaptada para muestreo a 100 Hz.
 */

#include "algorithm.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"
#include "task_telemetry_attribute.h"
#include "task_telemetry_interface.h"

int32_t g_algo_current_spo2 = 0; // Inicialmente 0 hasta recolectar 4 segs
int32_t g_algo_current_bpm = 0;

static uint32_t g_red_buffer[ALGO_BUFFER_SIZE];
static uint32_t g_ir_buffer[ALGO_BUFFER_SIZE];
static int32_t g_buffer_index = 0;

static bool b_finger_detected = true;

void algorithm_init(void)
{
	g_buffer_index = 0;
	g_algo_current_spo2 = 0;
	g_algo_current_bpm = 0;
}

void algorithm_process_sample(uint32_t red_sample, uint32_t ir_sample)
{
	/* Detección de dedo (mucha luz ambiental o baja reflexión) */
	if (50000ul > ir_sample)
	{
		g_buffer_index = 0;
		if (true == b_finger_detected)
		{
			b_finger_detected = false;
			g_algo_current_spo2 = 0;
			g_algo_current_bpm = 0;
			put_event_task_system(EV_SYS_SENSOR_ERR);
			put_event_task_telemetry(EV_TEL_SENSOR_ERR);
		}
		return;
	}
	else
	{
		b_finger_detected = true;
	}

	g_red_buffer[g_buffer_index] = red_sample;
	g_ir_buffer[g_buffer_index] = ir_sample;
	g_buffer_index++;

	/* Cuando juntamos 4 segundos de datos (400 muestras a 100Hz), procesamos. */
	if (ALGO_BUFFER_SIZE <= g_buffer_index)
	{
		int32_t spo2 = 0;
		int8_t valid_spo2 = 0;
		int32_t bpm = 0;
		int8_t valid_bpm = 0;

		/* Invocamos al algoritmo puro de SparkFun adaptado */
		maxim_heart_rate_and_oxygen_saturation(g_ir_buffer, ALGO_BUFFER_SIZE, g_red_buffer, &spo2, &valid_spo2, &bpm, &valid_bpm);

		/* Si el algoritmo indica lecturas válidas, filtramos */
		if (valid_spo2 && (0 < spo2) && (100 >= spo2))
		{
			if (0 == g_algo_current_spo2)
			{
				g_algo_current_spo2 = spo2; // Primera lectura rápida
			}
			else
			{
				/* Filtro IIR suave */
				g_algo_current_spo2 = (g_algo_current_spo2 * 7 + spo2) / 8;
			}
		}

		if (valid_bpm && (0 < bpm) && (250 > bpm))
		{
			if (0 == g_algo_current_bpm)
			{
				g_algo_current_bpm = bpm; // Primera lectura rápida
			}
			else
			{
				g_algo_current_bpm = (g_algo_current_bpm * 7 + bpm) / 8;
			}
		}

		/* Despachamos evento al sistema sólo si ya calibramos los primeros 4 segundos */
		if ((0 != g_algo_current_spo2) || (0 != g_algo_current_bpm))
		{
			put_event_task_system(EV_SYS_SPO2_DATA);
			put_event_task_telemetry(EV_TEL_SPO2_DATA);
		}

		/* 
		 * Desplazamos la ventana: movemos los últimos 3 segundos (300 muestras) al comienzo 
		 * y ajustamos el índice a 300, de manera que la próxima vez necesitemos sólo 100 muestras (1 segundo).
		 */
		int32_t i;
		for (i = 100; i < ALGO_BUFFER_SIZE; i++)
		{
			g_red_buffer[i - 100] = g_red_buffer[i];
			g_ir_buffer[i - 100] = g_ir_buffer[i];
		}
		g_buffer_index = ALGO_BUFFER_SIZE - 100; // 300
	}
}
