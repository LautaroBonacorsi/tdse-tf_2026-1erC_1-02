/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
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
 * @file   : app.c
 * @brief  : Main application scheduler and dispatcher
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "app.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app_it.h"
#include "task_sensor.h"
#include "task_system.h"
#include "task_display.h"
#include "task_display_attribute.h"
#include "task_display_interface.h"

#include "task_actuator.h"

#include "driver_eeprom.h"
#include "driver_max30102.h"
#include "algorithm.h"
#include "task_telemetry.h"

extern I2C_HandleTypeDef hi2c1;

/********************** macros and definitions *******************************/
#define TASK_X_NOE_INI		0ul
#define TASK_X_LET_INI		0ul
#define TASK_X_BCET_INI		1000ul
#define TASK_X_WCET_INI		0ul
#define TASK_X_DELAY_MIN	0ul

typedef struct {
	void (*task_init)(void *);		// Pointer to task (must be a
									// 'void (void *)' function)
	void (*task_update)(void *);	// Pointer to task (must be a
									// 'void (void *)' function)
	void *parameters;				// Pointer to parameters
} task_cfg_t;

typedef struct {
    uint32_t NOE;		// Number of execution (numeral)
    uint32_t LET;		// Last execution time (microseconds)
    uint32_t BCET;		// Best-case execution time (microseconds)
    uint32_t WCET;		// Worst-case execution time (microseconds)
} task_dta_t;

/********************** internal data declaration ****************************/
const task_cfg_t task_cfg_list[] = {
	{task_sensor_init, task_sensor_update, NULL},
	{task_system_init, task_system_update, NULL},
	{task_display_init, task_display_update, NULL},
	{task_actuator_init, task_actuator_update, NULL},
	{task_telemetry_init, task_telemetry_update, NULL}
};

#define TASK_QTY	(sizeof(task_cfg_list)/sizeof(task_cfg_t))

task_dta_t task_dta_list[TASK_QTY];

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
uint32_t g_app_cnt = 0;
uint32_t g_wcet = 0;
uint32_t g_app_runtime_us = 0;

const char *p_app	= "Bare Metal - Event-Triggered Systems (ETS)";
const char *p_app_	= "App - Model Integration - C codig";
const char *p_app__	= "(Update by Time Code, period = 1mS)";

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
void app_init(void)
{
	uint32_t index;

	/* Print out: Application Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("app_init is running");

	LOGGER_INFO(" app is a %s", p_app);
	LOGGER_INFO(" app is a %s", p_app_);
	LOGGER_INFO(" app is a %s", p_app__);

	/* Init Cycle Counter */
	cycle_counter_init();
	
	/* Iniciar driver EEPROM (No Bloqueante) */
	eeprom_init(&hi2c1);
	
	/* Iniciar MAX30102 y Algoritmo */
	max30102_init(&hi2c1);
	algorithm_init();
	
    /* Go through the task arrays */
	for (index = 0; TASK_QTY > index; index++)
	{
		if (NULL != task_cfg_list[index].task_init)
		{
			/* Run task_x_init */
			(*task_cfg_list[index].task_init)(task_cfg_list[index].parameters);
		}
		
		/* Init variables */
		task_dta_list[index].NOE = TASK_X_NOE_INI;
		task_dta_list[index].LET = TASK_X_LET_INI;
		task_dta_list[index].BCET = TASK_X_BCET_INI;
		task_dta_list[index].WCET = TASK_X_WCET_INI;
	}
	
	LOGGER_INFO("Aplicacion iniciada correctamente.");
}

void app_update(void)
{
	uint32_t index;
	bool b_time_update_required = false;

	/* Protect shared resource */
	__asm("CPSID i");	/* disable interrupts */
	if (0 < g_app_tick_cnt)
	{
		/* Update Tick Counter */
		g_app_tick_cnt--;
		b_time_update_required = true;
	}
	__asm("CPSIE i");	/* enable interrupts */

	/* Check if it's time to run tasks */
	while (b_time_update_required)
	{
    	/* Update App Counter */
		g_app_cnt++;
		g_app_runtime_us = 0;

		/* Update tasks */
		uint32_t total_cycles_start = cycle_counter_get();
		
		/* Update EEPROM driver state machine */
		eeprom_update();
		
		/* Update MAX30102 driver state machine (polling del pin INT) */
		max30102_update();
		
		/* Go through the task arrays */
		for (index = 0; TASK_QTY > index; index++)
		{
			cycle_counter_reset();

			if (NULL != task_cfg_list[index].task_update)
			{
    			/* Run task_x_update */
				(*task_cfg_list[index].task_update)(task_cfg_list[index].parameters);
			}

			/* Update variables */
			task_dta_list[index].NOE++;

			task_dta_list[index].LET = cycle_counter_get_time_us();

			if (task_dta_list[index].BCET > task_dta_list[index].LET)
			{
				task_dta_list[index].BCET = task_dta_list[index].LET;
			}

			if (task_dta_list[index].WCET < task_dta_list[index].LET)
			{
				task_dta_list[index].WCET = task_dta_list[index].LET;
			}

			g_app_runtime_us += task_dta_list[index].LET;
		}

		/* Measure WCET of the entire loop */
		uint32_t total_cycles = cycle_counter_get() - total_cycles_start;
		if (g_wcet < total_cycles) {
			g_wcet = total_cycles;
		}

		/* Protect shared resource */
		__asm("CPSID i");	/* disable interrupts */
		if (g_app_tick_cnt > 0)
		{
			g_app_tick_cnt--;
			b_time_update_required = true;
		}
		else
		{
			b_time_update_required = false;
		}
		__asm("CPSIE i");	/* enable interrupts */
	}
}

/********************** HAL Callbacks (Demultiplexing) ***********************/
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    eeprom_rx_cplt_callback(hi2c);
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    eeprom_tx_cplt_callback(hi2c);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    eeprom_error_callback(hi2c);
}

/* Declaraciones externas para callbacks de telemetría (se definen en task_telemetry.c) */
extern void telemetry_rx_cplt_callback(UART_HandleTypeDef *huart);
extern void telemetry_tx_cplt_callback(UART_HandleTypeDef *huart);

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    telemetry_rx_cplt_callback(huart);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    telemetry_tx_cplt_callback(huart);
}

/********************** end of file ******************************************/
