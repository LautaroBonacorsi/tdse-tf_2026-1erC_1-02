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
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "task_sensor.h"
#include "task_sensor_attribute.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"
#include <stdint.h>
#include <stdbool.h>

/********************** macros and definitions *******************************/
#define DEL_BTN_MIN		0ul
#define DEL_BTN_MED		25ul
#define DEL_BTN_MAX		50ul

#define SENSOR_CFG_QTY		(sizeof(task_sensor_cfg_list)/sizeof(task_sensor_cfg_t))
#define SENSOR_DTA_QTY		SENSOR_CFG_QTY

/********************** internal data declaration ****************************/
const task_sensor_cfg_t task_sensor_cfg_list[] = {
	{ID_BTN_MENU, BTN_MENU_PORT, BTN_MENU_PIN, BTN_PRESSED, DEL_BTN_MAX, EV_SYS_IDLE, EV_SYS_ESCAPE},
	{ID_BTN_UP,   BTN_UP_PORT,   BTN_UP_PIN,   BTN_PRESSED, DEL_BTN_MAX, EV_SYS_IDLE, EV_SYS_NEXT},
	{ID_BTN_DOWN, BTN_DOWN_PORT, BTN_DOWN_PIN, BTN_PRESSED, DEL_BTN_MAX, EV_SYS_IDLE, EV_SYS_DOWN},
	{ID_BTN_ACK,  BTN_ACK_PORT,  BTN_ACK_PIN,  BTN_PRESSED, DEL_BTN_MAX, EV_SYS_IDLE, EV_SYS_ENTER}
};

task_sensor_dta_t task_sensor_dta_list[SENSOR_DTA_QTY];
static uint8_t dip_switch_val = 0;

/********************** internal functions declaration ***********************/
void task_sensor_statechart(uint32_t index);

/********************** internal data definition *****************************/
const char *p_task_sensor 		= "Task Sensor (Sensor Statechart)";
const char *p_task_sensor_ 		= "Non-Blocking Code";
const char *p_task_sensor__ 	= "(Update by Time Code, period = 1mS)";

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
void task_sensor_init(void *parameters)
{
	uint32_t index;
	task_sensor_dta_t *p_task_sensor_dta;
	task_sensor_st_t state;
	task_sensor_ev_t event;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", GET_NAME(task_sensor_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_sensor), p_task_sensor);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_sensor), p_task_sensor_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_sensor), p_task_sensor__);

	for (index = 0; SENSOR_DTA_QTY > index; index++)
	{
		/* Update Task Sensor Data Pointer */
		p_task_sensor_dta = &task_sensor_dta_list[index];

		/* Init & Print out: Index & Task execution FSM */
		state = ST_BTN_UP;
		p_task_sensor_dta->state = state;

		event = EV_BTN_UP;
		p_task_sensor_dta->event = event;

		LOGGER_INFO(" ");
		LOGGER_INFO("   %s = %lu   %s = %lu   %s = %lu",
				    GET_NAME(index), index,
					GET_NAME(state), (uint32_t)state,
					GET_NAME(event), (uint32_t)event);
	}
}

void task_sensor_update(void *parameters)
{
	uint32_t index;

	/* 1. Update buttons state machines */
	for (index = 0; SENSOR_DTA_QTY > index; index++)
	{
		/* Run Task Statechart */
		task_sensor_statechart(index);
	}

	/* 2. Update DIP Switch state */
	uint8_t new_dip = 0;
	if (DIP_ON == HAL_GPIO_ReadPin(DIP1_PORT, DIP1_PIN)) { new_dip |= 0x01; }
	if (DIP_ON == HAL_GPIO_ReadPin(DIP2_PORT, DIP2_PIN)) { new_dip |= 0x02; }
	if (DIP_ON == HAL_GPIO_ReadPin(DIP3_PORT, DIP3_PIN)) { new_dip |= 0x04; }
	if (DIP_ON == HAL_GPIO_ReadPin(DIP4_PORT, DIP4_PIN)) { new_dip |= 0x08; }
	
	if (new_dip != dip_switch_val) {
		dip_switch_val = new_dip;
		LOGGER_INFO("DIP Switch modificado: 0x%02X", dip_switch_val);
	}
}

/********************** internal functions definition ************************/
void task_sensor_statechart(uint32_t index)
{
	const task_sensor_cfg_t *p_task_sensor_cfg;
	task_sensor_dta_t *p_task_sensor_dta;

	/* Update Task Sensor Configuration & Data Pointer */
	p_task_sensor_cfg = &task_sensor_cfg_list[index];
	p_task_sensor_dta = &task_sensor_dta_list[index];

	if (p_task_sensor_cfg->pressed == HAL_GPIO_ReadPin(p_task_sensor_cfg->gpio_port, p_task_sensor_cfg->pin))
	{
		p_task_sensor_dta->event =	EV_BTN_DOWN;
	}
	else
	{
		p_task_sensor_dta->event =	EV_BTN_UP;
	}

	switch (p_task_sensor_dta->state)
	{
		case ST_BTN_UP:

			if (EV_BTN_DOWN == p_task_sensor_dta->event)
			{
				p_task_sensor_dta->tick = p_task_sensor_cfg->tick_max;
				p_task_sensor_dta->state = ST_BTN_FALLING;
			}

			break;

		case ST_BTN_FALLING:

			if (p_task_sensor_dta->tick > 0) {
				p_task_sensor_dta->tick--;
			}
			else if (DEL_BTN_MIN == p_task_sensor_dta->tick)
			{
				if (EV_BTN_DOWN == p_task_sensor_dta->event)
				{
					put_event_task_system(p_task_sensor_cfg->signal_down);
					p_task_sensor_dta->state = ST_BTN_DOWN;
				}
				else if (EV_BTN_UP == p_task_sensor_dta->event)
				{
					p_task_sensor_dta->state = ST_BTN_UP;
				}
			}

			break;

		case ST_BTN_DOWN:

			if (EV_BTN_UP == p_task_sensor_dta->event)
			{
				p_task_sensor_dta->tick = p_task_sensor_cfg->tick_max;
				p_task_sensor_dta->state = ST_BTN_RISING;
			}

			break;

		case ST_BTN_RISING:

			if (p_task_sensor_dta->tick > 0) {
				p_task_sensor_dta->tick--;
			} else if (DEL_BTN_MIN == p_task_sensor_dta->tick)
			{
				if (EV_BTN_UP == p_task_sensor_dta->event)
				{
					put_event_task_system(p_task_sensor_cfg->signal_up);
					p_task_sensor_dta->state = ST_BTN_UP;
				}
				else if (EV_BTN_DOWN == p_task_sensor_dta->event)
				{
					p_task_sensor_dta->state = ST_BTN_DOWN;
				}
			}

			break;

		default:

			p_task_sensor_dta->tick  = DEL_BTN_MIN;
			p_task_sensor_dta->state = ST_BTN_UP;
			p_task_sensor_dta->event = EV_BTN_UP;

			break;
	}
}

/********************** end of file ******************************************/
