/*
 * algorithm.c
 *
 * Implementación optimizada del algoritmo de SpO2 y BPM (Maxim).
 */

#include "algorithm.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"
#include "task_telemetry_attribute.h"
#include "task_telemetry_interface.h"

int32_t g_algo_current_spo2 = 98;
int32_t g_algo_current_bpm = 75;

static uint32_t g_red_buffer[ALGO_BUFFER_SIZE];
static uint32_t g_ir_buffer[ALGO_BUFFER_SIZE];
static int32_t g_buffer_index = 0;

static bool b_finger_detected = true;
static uint64_t g_total_samples = 0;
static uint64_t g_last_trough_time = 0;

void algorithm_init(void)
{
	g_buffer_index = 0;
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

	/* Cuando juntamos 1 segundo de datos, procesamos. */
	if (ALGO_BUFFER_SIZE <= g_buffer_index)
	{
		g_buffer_index = 0;

		/* 1. Filtro de media móvil (5 puntos) para Red e IR */
		uint32_t ir_smooth[ALGO_BUFFER_SIZE];
		uint32_t red_smooth[ALGO_BUFFER_SIZE];
		int32_t i;
		int32_t j;

		for (i = 0; i < ALGO_BUFFER_SIZE; i++)
		{
			uint32_t sum_ir = 0;
			uint32_t sum_red = 0;
			int32_t count = 0;
			for (j = -2; j <= 2; j++)
			{
				if ((0 <= (i + j)) && (ALGO_BUFFER_SIZE > (i + j)))
				{
					sum_ir += g_ir_buffer[i + j];
					sum_red += g_red_buffer[i + j];
					count++;
				}
			}
			ir_smooth[i] = sum_ir / count;
			red_smooth[i] = sum_red / count;
		}

		/* 2. Calcular Componentes DC (Promedio de la señal suavizada) */
		uint32_t red_dc = 0;
		uint32_t ir_dc = 0;
		for (i = 0; i < ALGO_BUFFER_SIZE; i++)
		{
			red_dc += red_smooth[i];
			ir_dc += ir_smooth[i];
		}
		red_dc /= ALGO_BUFFER_SIZE;
		ir_dc /= ALGO_BUFFER_SIZE;

		/* 3. Calcular Componentes AC (Pico a Pico de la señal suavizada) */
		uint32_t red_min = 0xFFFFFFFFul;
		uint32_t red_max = 0ul;
		uint32_t ir_min = 0xFFFFFFFFul;
		uint32_t ir_max = 0ul;
		for (i = 0; i < ALGO_BUFFER_SIZE; i++)
		{
			if (red_smooth[i] < red_min)
			{
				red_min = red_smooth[i];
			}
			if (red_smooth[i] > red_max)
			{
				red_max = red_smooth[i];
			}
			if (ir_smooth[i] < ir_min)
			{
				ir_min = ir_smooth[i];
			}
			if (ir_smooth[i] > ir_max)
			{
				ir_max = ir_smooth[i];
			}
		}

		uint32_t red_ac = red_max - red_min;
		uint32_t ir_ac = ir_max - ir_min;

		/* 4. Ratio de Ratios: R = (AC_Red/DC_Red) / (AC_IR/DC_IR) */
		uint32_t ratio = 0;
		if ((20ul < ir_ac) && (0ul < red_dc) && (0ul < ir_dc))
		{
			uint32_t num = (red_ac * 10000ul) / red_dc;
			uint32_t den = (ir_ac * 10000ul) / ir_dc;
			if (0ul < den)
			{
				ratio = (num * 100ul) / den; /* Escalar por 100 */
			}
		}

		/* 5. Fórmula SpO2 (Ajustada para Maxim MAX30102) */
		if (0ul < ratio)
		{
			/* Fórmula clásica lineal empírica */
			int32_t spo2 = 104 - (17 * ratio) / 100;
			if (100 < spo2)
			{
				spo2 = 100;
			}
			if (0 > spo2)
			{
				spo2 = 0;
			}

			/* Rechazo de artefactos: si cae por debajo de 80, probable mal apoyo */
			if (80 < spo2)
			{
				/* Filtro IIR Fuerte (alpha = 0.125) para máxima estabilidad */
				g_algo_current_spo2 = (g_algo_current_spo2 * 7 + spo2) / 8;
			}
		}

		/* 6. Cálculo de BPM continuo entre ventanas */
		if (20ul < ir_ac)
		{
			for (i = 2; i < ALGO_BUFFER_SIZE - 2; i++)
			{
				/* Verificar que el valle sea profundo (rechazo de ruido) */
				if (ir_smooth[i] < (ir_dc - ir_ac / 3))
				{
					/* Mínimo local */
					if ((ir_smooth[i] <= ir_smooth[i - 1]) && (ir_smooth[i] <= ir_smooth[i + 1]))
					{
						uint64_t current_time = g_total_samples + i;

						if (0ull != g_last_trough_time)
						{
							uint64_t dist = current_time - g_last_trough_time;
							/* Rango de distancias: 35 a 150 (ignora el "Dicrotic Notch") */
							if ((35ull < dist) && (150ull > dist))
							{
								int32_t bpm = 6000 / dist; /* 60s * 100Hz */

								/* Si hay un salto gigante, adaptamos rápido. Si no, promediamos fuerte */
								if ((bpm > g_algo_current_bpm + 20) || (bpm < g_algo_current_bpm - 20))
								{
									g_algo_current_bpm = (g_algo_current_bpm * 3 + bpm) / 4;
								}
								else
								{
									g_algo_current_bpm = (g_algo_current_bpm * 7 + bpm) / 8;
								}
							}
						}
						g_last_trough_time = current_time;
						i += 30; /* Refractario de 300ms para saltar el Dicrotic Notch */
					}
				}
			}
		}

		g_total_samples += ALGO_BUFFER_SIZE;
		put_event_task_system(EV_SYS_SPO2_DATA);
		put_event_task_telemetry(EV_TEL_SPO2_DATA);
	}
}
