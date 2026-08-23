/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *	contributors may be used to endorse or promote products derived from
 *	this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"

#include "task_display_attribute.h"
#include "task_display_interface.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"

#include "driver_eeprom.h"
#include "algorithm.h"
#include "task_actuator_attribute.h"
#include "task_actuator_interface.h"
extern I2C_HandleTypeDef hi2c1;

/********************** macros and definitions *******************************/
#define DEL_SYS_MIN			0ul
#define DISPLAY_COLS		16u

/* Modes to excite Task System */
typedef enum task_system_mode {
	NORMAL,
	SETUP,
	BLOCKED_RESET,
	BLOCKED_ERROR,
	MODE_QTY
} task_system_mode_t;

#define SYSTEM_DTA_QTY		MODE_QTY

/********************** internal data declaration ****************************/
task_system_dta_t task_system_dta_list[SYSTEM_DTA_QTY];

/*
 * Variables del modelo de Itemis.
 *
 * g_selected_sensor:
 * 0 = oximetro
 * 1 = pulsometro
 *
 * g_param_umbral:
 * 0 = umbral minimo
 * 1 = umbral maximo
 * 2 = alarma
 */

static int32_t g_selected_sensor = 0;
static int32_t g_param_umbral = 0;

static int32_t g_ou_max = 10;
static int32_t g_ou_min = 0;
static bool b_o_alarma = false;

static int32_t g_pu_max = 10;
static int32_t g_pu_min = 0;
static bool b_p_alarma = false;

static int32_t g_aux_max = 10;
static int32_t g_aux_min = 0;
static bool b_aux_alarma = false;

static char display_line_1[DISPLAY_COLS + 1u];
static char display_line_2[DISPLAY_COLS + 1u];

#pragma pack(push, 1)
typedef struct {
	uint32_t magic_word;
	int32_t g_ou_max;
	int32_t g_ou_min;
	int32_t g_pu_max;
	int32_t g_pu_min;
	uint8_t b_o_alarma;
	uint8_t b_p_alarma;
} sys_config_t;
#pragma pack(pop)

static sys_config_t g_current_config;
static sys_config_t g_last_saved_config;
static uint8_t g_current_profile_id = 1;

/********************** internal functions declaration ***********************/
static void task_system_load_config(void);
static void task_system_save_config(void);

static void task_system_normal_statechart(void);
static void task_system_setup_statechart(void);

static void task_system_set_mode(task_system_mode_t task_system_mode);
static void task_system_set_setup_state(task_system_st_t state);

static void task_system_show_normal(void);
static void task_system_show_setup_state(void);

static void task_system_write_display(const char *line_1,
									  const char *line_2);

/********************** internal data definition *****************************/
const char *p_task_system 		= "Task System (System Statechart)";
const char *p_task_system_ 		= "Non-Blocking Code";
const char *p_task_system__ 	= "(Update by Time Code, period = 1mS)";

/********************** external data declaration ****************************/
task_system_mode_t g_task_system_mode;

/********************** external functions definition ************************/
void task_system_init(void *parameters)
{
	uint32_t index;
	task_system_dta_t *p_task_system_dta;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu",
				GET_NAME(task_system_init),
				HAL_GetTick());

	LOGGER_INFO("   %s is a %s",
				GET_NAME(task_system),
				p_task_system);

	LOGGER_INFO("   %s is a %s",
				GET_NAME(task_system),
				p_task_system_);

	LOGGER_INFO("   %s is a %s",
				GET_NAME(task_system),
				p_task_system__);

	init_event_task_system();

	for (index = 0; index < SYSTEM_DTA_QTY; index++)
	{
		p_task_system_dta = &task_system_dta_list[index];

		p_task_system_dta->tick = DEL_SYS_MIN;
		p_task_system_dta->state = ST_SYS_NORMAL;
		p_task_system_dta->event = EV_SYS_IDLE;
		p_task_system_dta->flag = false;
	}

	task_system_load_config();

	if ((BLOCKED_RESET != g_task_system_mode) && (BLOCKED_ERROR != g_task_system_mode))
	{
		task_system_set_mode(NORMAL);
		task_system_show_normal();
	}
}


void task_system_update(void *parameters)
{
	switch (g_task_system_mode)
	{
		case NORMAL:
			task_system_normal_statechart();
			break;

		case SETUP:
			task_system_setup_statechart();
			break;
			
		case BLOCKED_RESET:
		case BLOCKED_ERROR:
			/* Ignorar eventos, mantener bloqueado */
			if (true == any_event_task_system())
			{
				get_event_task_system(); /* Consumir y descartar */
			}
			break;

		default:
			task_system_set_mode(NORMAL);
			task_system_show_normal();
			break;
	}
}


static void task_system_normal_statechart(void)
{
	task_system_dta_t *p_task_system_dta;

	p_task_system_dta = &task_system_dta_list[NORMAL];

	if (false == any_event_task_system())
	{
		return;
	}

	p_task_system_dta->event = get_event_task_system();
	p_task_system_dta->flag = true;

	/*
	 * En modo NORMAL, el botón MENU ingresa
	 * al menú de configuración. (mapeado a EV_SYS_ESCAPE)
	 */
	if (EV_SYS_ESCAPE == p_task_system_dta->event)
	{
		task_system_set_setup_state(ST_SYS_MENU1_SENSOR);
		task_system_set_mode(SETUP);
	}
	else if (EV_SYS_SPO2_DATA == p_task_system_dta->event)
	{
		char line1[17];
		char line2[17];
		snprintf(line1, sizeof(line1), "SpO2: %3ld%%", (long)g_algo_current_spo2);
		snprintf(line2, sizeof(line2), " BPM: %3ld ", (long)g_algo_current_bpm);
		task_system_write_display(line1, line2);

		bool spo2_crit = false;
		bool pulse_warn = false;

		if (b_o_alarma) {
			if (g_algo_current_spo2 > g_ou_max || g_algo_current_spo2 < g_ou_min) {
				spo2_crit = true;
			}
		}

		if (b_p_alarma) {
			if (g_algo_current_bpm > g_pu_max || g_algo_current_bpm < g_pu_min) {
				pulse_warn = true;
			}
		}

		if (spo2_crit) {
			put_event_task_actuator(EV_ACT_ALARM_SPO2_CRIT);
		} else if (pulse_warn) {
			put_event_task_actuator(EV_ACT_ALARM_PULSE_WARN);
		} else {
			put_event_task_actuator(EV_ACT_ALARM_OFF);
		}
	}
	else if (EV_SYS_SENSOR_ERR == p_task_system_dta->event)
	{
		task_system_write_display("NO FINGER DETECT", "WAITING...	  ");
		put_event_task_actuator(EV_ACT_ALARM_OFF);
	}

	p_task_system_dta->flag = false;
}


static void task_system_setup_statechart(void)
{
	task_system_dta_t *p_task_system_dta;

	p_task_system_dta = &task_system_dta_list[SETUP];

	if (false == any_event_task_system())
	{
		return;
	}

	p_task_system_dta->event = get_event_task_system();
	p_task_system_dta->flag = true;

	switch (p_task_system_dta->state)
	{
		/*
		 * Menu 1:
		 * Selección entre oxímetro y pulsómetro.
		 */
		case ST_SYS_MENU1_SENSOR:

			if (EV_SYS_NEXT == p_task_system_dta->event)
			{
				g_selected_sensor =
					(0 == g_selected_sensor) ? 1 : 0;

				task_system_show_setup_state();
			}
			else if (EV_SYS_ENTER ==
					 p_task_system_dta->event)
			{
				task_system_set_setup_state(
					ST_SYS_MENU2_PARAMETER);
			}
			else if (EV_SYS_ESCAPE ==
					 p_task_system_dta->event)
			{
				/*
				 * Esta transición no aparece en el modelo,
				 * pero es necesaria para salir de SETUP.
				 */
				task_system_save_config();

				task_system_set_mode(NORMAL);
				task_system_show_normal();
			}

			break;

		/*
		 * Menu 2:
		 * Selección entre mínimo, máximo y alarma.
		 */
		case ST_SYS_MENU2_PARAMETER:

			if (EV_SYS_NEXT == p_task_system_dta->event)
			{
				g_param_umbral = (g_param_umbral + 1) % 3;

				task_system_show_setup_state();
			}
			else if (EV_SYS_ESCAPE ==
					 p_task_system_dta->event)
			{
				task_system_set_setup_state(
					ST_SYS_MENU1_SENSOR);
			}
			else if (EV_SYS_ENTER ==
					 p_task_system_dta->event)
			{
				/*
				 * Implementación del pseudoestado Choice
				 * del modelo de Itemis.
				 */
				if (0 == g_param_umbral)
				{
					task_system_set_setup_state(
						ST_SYS_MENU3_MINIMUM);
				}
				else if (1 == g_param_umbral)
				{
					task_system_set_setup_state(
						ST_SYS_MENU3_MAXIMUM);
				}
				else
				{
					task_system_set_setup_state(
						ST_SYS_MENU3_ALARM);
				}
			}

			break;

		/*
		 * Menu 3:
		 * Edición del umbral mínimo.
		 */
		case ST_SYS_MENU3_MINIMUM:

			if (EV_SYS_NEXT ==
				p_task_system_dta->event)
			{
				g_aux_min++;
				task_system_show_setup_state();
			}
			else if (EV_SYS_DOWN ==
					 p_task_system_dta->event)
			{
				g_aux_min--;
				task_system_show_setup_state();
			}
			else if (EV_SYS_ESCAPE ==
					 p_task_system_dta->event)
			{
				/*
				 * Escape cancela la edición porque
				 * g_aux_min todavía no fue copiado.
				 */
				task_system_set_setup_state(
					ST_SYS_MENU2_PARAMETER);
			}
			else if (EV_SYS_ENTER ==
					 p_task_system_dta->event)
			{
				if ((0 == g_selected_sensor) &&
					(g_aux_min < g_ou_max))
				{
					g_ou_min = g_aux_min;

					task_system_set_setup_state(
						ST_SYS_MENU2_PARAMETER);
				}
				else if ((1 == g_selected_sensor) &&
						 (g_aux_min < g_pu_max))
				{
					g_pu_min = g_aux_min;

					task_system_set_setup_state(
						ST_SYS_MENU2_PARAMETER);
				}
			}

			break;

		/*
		 * Menu 3:
		 * Edición del umbral máximo.
		 */
		case ST_SYS_MENU3_MAXIMUM:

			if (EV_SYS_NEXT ==
				p_task_system_dta->event)
			{
				g_aux_max++;
				task_system_show_setup_state();
			}
			else if (EV_SYS_DOWN ==
					 p_task_system_dta->event)
			{
				g_aux_max--;
				task_system_show_setup_state();
			}
			else if (EV_SYS_ESCAPE ==
					 p_task_system_dta->event)
			{
				task_system_set_setup_state(
					ST_SYS_MENU2_PARAMETER);
			}
			else if (EV_SYS_ENTER ==
					 p_task_system_dta->event)
			{
				/*
				 * En el statechart aparece g_aux_min.
				 * Acá corresponde comparar g_aux_max.
				 */
				if ((0 == g_selected_sensor) &&
					(g_aux_max > g_ou_min))
				{
					g_ou_max = g_aux_max;

					task_system_set_setup_state(
						ST_SYS_MENU2_PARAMETER);
				}
				else if ((1 == g_selected_sensor) &&
						 (g_aux_max > g_pu_min))
				{
					g_pu_max = g_aux_max;

					task_system_set_setup_state(
						ST_SYS_MENU2_PARAMETER);
				}
			}

			break;

		/*
		 * Menu 3:
		 * Activación o desactivación de alarma.
		 */
		case ST_SYS_MENU3_ALARM:

			if (EV_SYS_NEXT ==
				p_task_system_dta->event)
			{
				b_aux_alarma = !b_aux_alarma;

				task_system_show_setup_state();
			}
			else if (EV_SYS_ESCAPE ==
					 p_task_system_dta->event)
			{
				task_system_set_setup_state(
					ST_SYS_MENU2_PARAMETER);
			}
			else if (EV_SYS_ENTER ==
					 p_task_system_dta->event)
			{
				if (0 == g_selected_sensor)
				{
					b_o_alarma = b_aux_alarma;
				}
				else
				{
					b_p_alarma = b_aux_alarma;
				}

				task_system_set_setup_state(
					ST_SYS_MENU2_PARAMETER);
			}

			break;

		default:

			task_system_set_setup_state(
				ST_SYS_MENU1_SENSOR);

			break;
	}

	/*
	 * Cada evento se procesa una sola vez.
	 * EV_SYS_IDLE, generado al soltar un botón,
	 * simplemente se descarta.
	 */
	p_task_system_dta->flag = false;
}


static void task_system_set_setup_state(task_system_st_t state)
{
	task_system_dta_t *p_task_system_dta;

	p_task_system_dta = &task_system_dta_list[SETUP];

	p_task_system_dta->state = state;

	/*
	 * Acciones de entrada de los estados Menu 3.
	 * Copian el valor real a una variable auxiliar.
	 */
	switch (state)
	{
		case ST_SYS_MENU3_MINIMUM:

			g_aux_min = (0 == g_selected_sensor) ?
					 g_ou_min :
					 g_pu_min;

			break;

		case ST_SYS_MENU3_MAXIMUM:

			g_aux_max = (0 == g_selected_sensor) ?
					 g_ou_max :
					 g_pu_max;

			break;

		case ST_SYS_MENU3_ALARM:

			b_aux_alarma = (0 == g_selected_sensor) ?
						b_o_alarma :
						b_p_alarma;

			break;

		default:

			break;
	}

	task_system_show_setup_state();
}


static void task_system_show_normal(void)
{
	task_system_write_display(
		"task_system_mode",
		" NORMAL");
}


static void task_system_show_setup_state(void)
{
	task_system_st_t state;

	state = task_system_dta_list[SETUP].state;

	switch (state)
	{
		case ST_SYS_MENU1_SENSOR:

			task_system_write_display(
				"SELECT SENSOR",
				(0 == g_selected_sensor) ?
				"> OXIMETER" :
				"> PULSE METER");

			break;

		case ST_SYS_MENU2_PARAMETER:

			if (0 == g_param_umbral)
			{
				task_system_write_display(
					(0 == g_selected_sensor) ?
					"OXIMETER SETUP" :
					"PULSE SETUP",
					"> MINIMUM");
			}
			else if (1 == g_param_umbral)
			{
				task_system_write_display(
					(0 == g_selected_sensor) ?
					"OXIMETER SETUP" :
					"PULSE SETUP",
					"> MAXIMUM");
			}
			else
			{
				task_system_write_display(
					(0 == g_selected_sensor) ?
					"OXIMETER SETUP" :
					"PULSE SETUP",
					"> ALARM");
			}

			break;

		case ST_SYS_MENU3_MINIMUM:

			snprintf(
				display_line_1,
				sizeof(display_line_1),
				"%-16.16s",
				"MINIMUM VALUE");

			snprintf(
				display_line_2,
				sizeof(display_line_2),
				"VALUE: %-9ld",
				(long)g_aux_min);

			put_event_task_display(
				0,
				0,
				display_line_1);

			put_event_task_display(
				0,
				1,
				display_line_2);

			break;

		case ST_SYS_MENU3_MAXIMUM:

			snprintf(
				display_line_1,
				sizeof(display_line_1),
				"%-16.16s",
				"MAXIMUM VALUE");

			snprintf(
				display_line_2,
				sizeof(display_line_2),
				"VALUE: %-9ld",
				(long)g_aux_max);

			put_event_task_display(
				0,
				0,
				display_line_1);

			put_event_task_display(
				0,
				1,
				display_line_2);

			break;

		case ST_SYS_MENU3_ALARM:

			task_system_write_display(
				"ALARM STATE",
				b_aux_alarma ?
				"> ON" :
				"> OFF");

			break;

		default:

			task_system_write_display(
				"SETUP MENU",
				"INVALID STATE");

			break;
	}
}


static void task_system_write_display(const char *line_1,
									  const char *line_2)
{
	/*
	 * %-16.16s completa cada línea con espacios
	 * y limita el texto a las 16 columnas del LCD.
	 */
	snprintf(
		display_line_1,
		sizeof(display_line_1),
		"%-16.16s",
		line_1);

	snprintf(
		display_line_2,
		sizeof(display_line_2),
		"%-16.16s",
		line_2);

	put_event_task_display(
		0,
		0,
		display_line_1);

	put_event_task_display(
		0,
		1,
		display_line_2);
}

static void task_system_set_mode(
	task_system_mode_t task_system_mode)
{
	g_task_system_mode = task_system_mode;
}

static uint16_t get_eeprom_address(uint8_t profile)
{
	if (profile < 1 || profile > 3) profile = 1;
	return (uint16_t)((profile - 1) * 32); /* Separados por 32 bytes */
}

static void set_default_thresholds(uint8_t profile)
{
	if (1 == profile)
	{
		g_ou_min = 92;
		g_ou_max = 100;
		g_pu_min = 70;
		g_pu_max = 130;
	}
	else if (2 == profile)
	{
		g_ou_min = 90;
		g_ou_max = 100;
		g_pu_min = 60;
		g_pu_max = 100;
	}
	else
	{
		g_ou_min = 88;
		g_ou_max = 100;
		g_pu_min = 50;
		g_pu_max = 90;
	}
	b_o_alarma = true;
	b_p_alarma = true;
}

static void task_system_load_config(void)
{
	bool d4 = (DIP_ON == HAL_GPIO_ReadPin(DIP1_PORT, DIP1_PIN));
	bool d3 = (DIP_ON == HAL_GPIO_ReadPin(DIP2_PORT, DIP2_PIN));
	bool d2 = (DIP_ON == HAL_GPIO_ReadPin(DIP3_PORT, DIP3_PIN));
	bool d1 = (DIP_ON == HAL_GPIO_ReadPin(DIP4_PORT, DIP4_PIN));
//	if(d2 == 1){
		//printf("d2 = 1");
	//}
	
	if (d1 && d2 && d3 && d4)
	{
		/* Reinicio de fabrica. Borramos las 3 posiciones (seteamos magic word a 0) */
		sys_config_t empty_cfg;
		memset(&empty_cfg, 0, sizeof(sys_config_t));
		HAL_I2C_Mem_Write(&hi2c1, EEPROM_I2C_ADDR, get_eeprom_address(1), I2C_MEMADD_SIZE_16BIT, (uint8_t*)&empty_cfg, sizeof(sys_config_t), 100);
		HAL_Delay(10);
		HAL_I2C_Mem_Write(&hi2c1, EEPROM_I2C_ADDR, get_eeprom_address(2), I2C_MEMADD_SIZE_16BIT, (uint8_t*)&empty_cfg, sizeof(sys_config_t), 100);
		HAL_Delay(10);
		HAL_I2C_Mem_Write(&hi2c1, EEPROM_I2C_ADDR, get_eeprom_address(3), I2C_MEMADD_SIZE_16BIT, (uint8_t*)&empty_cfg, sizeof(sys_config_t), 100);
		HAL_Delay(10);
		
		task_system_set_mode(BLOCKED_RESET);
		task_system_write_display("Reinicio Fabrica", "Reinicie equipo ");
		LOGGER_INFO("Bloqueado: Reset Fabrica");
		return;
	}
	
	uint8_t count = (d1?1:0) + (d2?1:0) + (d3?1:0);
	if (1 != count)
	{
		task_system_set_mode(BLOCKED_ERROR);
		task_system_write_display("Error de perfil ", "Elija 1 y reset ");
		LOGGER_INFO("Bloqueado: Error seleccion perfil");
		return;
	}
	
	g_current_profile_id = d1 ? 1 : (d2 ? 2 : 3);
	uint16_t mem_addr = get_eeprom_address(g_current_profile_id);
	
	sys_config_t temp_config;
	if (HAL_OK == HAL_I2C_Mem_Read(&hi2c1, EEPROM_I2C_ADDR, mem_addr, I2C_MEMADD_SIZE_16BIT, (uint8_t*)&temp_config, sizeof(sys_config_t), 100))
	{
		if (0x12345678 == temp_config.magic_word)
		{
			g_ou_max = temp_config.g_ou_max;
			g_ou_min = temp_config.g_ou_min;
			g_pu_max = temp_config.g_pu_max;
			g_pu_min = temp_config.g_pu_min;
			b_o_alarma = (0 != temp_config.b_o_alarma);
			b_p_alarma = (0 != temp_config.b_p_alarma);
			g_last_saved_config = temp_config;
			LOGGER_INFO("Perfil %d cargado desde EEPROM", g_current_profile_id);
		}
		else
		{
			LOGGER_INFO("EEPROM vacia para perfil %d. Cargando defaults.", g_current_profile_id);
			set_default_thresholds(g_current_profile_id);
			memset(&g_last_saved_config, 0, sizeof(sys_config_t));
			task_system_save_config(); /* Guardamos defaults */
		}
	}
	else
	{
		LOGGER_ERROR("Error I2C en EEPROM perfil %d", g_current_profile_id);
		set_default_thresholds(g_current_profile_id);
		memset(&g_last_saved_config, 0, sizeof(sys_config_t));
	}
}

static void task_system_save_config(void)
{
	sys_config_t new_config;
	new_config.magic_word = 0x12345678;
	new_config.g_ou_max = g_ou_max;
	new_config.g_ou_min = g_ou_min;
	new_config.g_pu_max = g_pu_max;
	new_config.g_pu_min = g_pu_min;
	new_config.b_o_alarma = b_o_alarma ? 1 : 0;
	new_config.b_p_alarma = b_p_alarma ? 1 : 0;
	
	if (0 == memcmp(&new_config, &g_last_saved_config, sizeof(sys_config_t)))
	{
		return;
	}
	
	g_current_config = new_config;
	g_last_saved_config = new_config;
	uint16_t mem_addr = get_eeprom_address(g_current_profile_id);
	
	if (EEPROM_OK == eeprom_write_it(mem_addr, (uint8_t*)&g_current_config, sizeof(sys_config_t)))
	{
		LOGGER_INFO("Guardando configuracion asincrono...");
	}
}

void task_system_update_config_from_telemetry(int32_t spo2_min, int32_t bpm_min, int32_t bpm_max, int8_t alarm_en)
{
	if (spo2_min >= 0) g_ou_min = spo2_min;
	if (bpm_min >= 0) g_pu_min = bpm_min;
	if (bpm_max >= 0) g_pu_max = bpm_max;
	if (alarm_en == 1) {
		b_o_alarma = true;
		b_p_alarma = true;
	} else if (alarm_en == 0) {
		b_o_alarma = false;
		b_p_alarma = false;
	}
	task_system_save_config();
}

bool task_system_is_alarm_active(void)
{
	extern int32_t g_algo_current_spo2;
	extern int32_t g_algo_current_bpm;
	
	/* Master switch overriding */
	if (DIP_OFF == HAL_GPIO_ReadPin(DIP4_PORT, DIP4_PIN))
	{
		return false;
	}
	
	if (b_o_alarma && (g_algo_current_spo2 > g_ou_max || g_algo_current_spo2 < g_ou_min)) return true;
	if (b_p_alarma && (g_algo_current_bpm > g_pu_max || g_algo_current_bpm < g_pu_min)) return true;
	return false;
}

/********************** end of file ******************************************/
