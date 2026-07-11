/*
 * task_actuator.c
 *
 * Maneja los LEDs y el buzzer mediante PWM de forma no bloqueante,
 * escuchando eventos desde task_system.
 */

#include "main.h"
#include "board.h"
#include "logger.h"
#include "task_actuator.h"
#include "task_actuator_attribute.h"
#include "task_actuator_interface.h"

extern TIM_HandleTypeDef htim3;

static task_actuator_st_t g_current_state = ST_ACT_OFF;
static uint32_t g_pattern_timer = 0;
static bool b_toggle_flag = false;

void task_actuator_init(void *parameters)
{
	init_event_task_actuator();
	g_current_state = ST_ACT_OFF;
	
	LOGGER_INFO("Iniciando Autotest de Hardware...");
	
	/* Encender LEDs y Buzzer (PWM al 50%) */
	HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, LED_ON);
	HAL_GPIO_WritePin(LED_Y_PORT, LED_Y_PIN, LED_ON);
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, __HAL_TIM_GET_AUTORELOAD(&htim3) / 2);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	
	/* Delay bloqueante de 1 segundo (solo válido en la inicialización) */
	HAL_Delay(1000);
	
	/* Apagar todo */
	HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, LED_OFF);
	HAL_GPIO_WritePin(LED_Y_PORT, LED_Y_PIN, LED_OFF);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
	
	LOGGER_INFO("Autotest finalizado. Task Actuator Initialized");
}

void task_actuator_update(void *parameters)
{
	/* 1. Process Events */
	if (true == any_event_task_actuator())
	{
		task_actuator_ev_t ev = get_event_task_actuator();
		switch (ev)
		{
			case EV_ACT_ALARM_OFF:
				g_current_state = ST_ACT_OFF;
				break;
			case EV_ACT_ALARM_PULSE_WARN:
				if (ST_ACT_CRITICAL != g_current_state)
				{
					g_current_state = ST_ACT_WARNING;
				}
				break;
			case EV_ACT_ALARM_SPO2_CRIT:
				g_current_state = ST_ACT_CRITICAL;
				break;
			default:
				break;
		}
		/* Reset pattern on state change */
		g_pattern_timer = 0;
		b_toggle_flag = false;
		
		/* Ensure devices are turned off immediately upon entering a new state */
		HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, LED_OFF);
		HAL_GPIO_WritePin(LED_Y_PORT, LED_Y_PIN, LED_OFF);
		HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
	}

	/* 2. State Machine Action (Non-blocking pattern generation using ticks) */
	switch (g_current_state)
	{
		case ST_ACT_OFF:
			/* Already handled in event transition */
			break;
			
		case ST_ACT_WARNING:
			/* Yellow LED and slow beep: 500ms ON, 500ms OFF */
			g_pattern_timer++;
			if (500ul <= g_pattern_timer)
			{
				g_pattern_timer = 0;
				b_toggle_flag = !b_toggle_flag;
				
				if (true == b_toggle_flag)
				{
					HAL_GPIO_WritePin(LED_Y_PORT, LED_Y_PIN, LED_ON);
					HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
				}
				else
				{
					HAL_GPIO_WritePin(LED_Y_PORT, LED_Y_PIN, LED_OFF);
					HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
				}
			}
			break;
			
		case ST_ACT_CRITICAL:
			/* Red LED and fast beep: 200ms ON, 200ms OFF */
			g_pattern_timer++;
			if (200ul <= g_pattern_timer)
			{
				g_pattern_timer = 0;
				b_toggle_flag = !b_toggle_flag;
				
				if (true == b_toggle_flag)
				{
					HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, LED_ON);
					HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
				}
				else
				{
					HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, LED_OFF);
					HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
				}
			}
			break;
	}
}
