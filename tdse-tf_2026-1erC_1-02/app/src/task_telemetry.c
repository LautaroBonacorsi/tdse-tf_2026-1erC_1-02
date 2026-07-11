#include "main.h"
#include "app.h"
#include "task_telemetry.h"
#include "task_telemetry_attribute.h"
#include "task_telemetry_interface.h"
#include "task_system.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart1;
extern int32_t g_algo_current_spo2;
extern int32_t g_algo_current_bpm;

task_telemetry_dta_t task_telemetry_dta_list[] = {
	{0, ST_TEL_IDLE, EV_TEL_IDLE, false}
};

task_telemetry_dta_t *p_task_telemetry_dta;

#define TX_BUFFER_SIZE 64ul
#define RX_BUFFER_SIZE 32ul

static char g_tx_buffer[TX_BUFFER_SIZE];
static char g_rx_buffer[RX_BUFFER_SIZE];
static uint8_t g_rx_byte;
static uint8_t g_rx_index = 0;

void task_telemetry_init(void *parameters)
{
	p_task_telemetry_dta = &task_telemetry_dta_list[0];
	p_task_telemetry_dta->state = ST_TEL_IDLE;
	p_task_telemetry_dta->event = EV_TEL_IDLE;
	
	init_event_task_telemetry();
	
	/* Iniciar recepción por interrupción (1 byte) */
	HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1);
}

void task_telemetry_update(void *parameters)
{
	if (true == any_event_task_telemetry())
	{
		p_task_telemetry_dta->flag = true;
		p_task_telemetry_dta->event = get_event_task_telemetry();
	}

	switch (p_task_telemetry_dta->state)
	{
		case ST_TEL_IDLE:
			if (true == p_task_telemetry_dta->flag)
			{
				p_task_telemetry_dta->flag = false;
				
				if (EV_TEL_SPO2_DATA == p_task_telemetry_dta->event)
				{
					bool alarm = task_system_is_alarm_active();
					snprintf(g_tx_buffer, TX_BUFFER_SIZE, "{\"type\":\"data\",\"spo2\":%ld,\"bpm\":%ld,\"alarm\":%d}\n", 
							 (long)g_algo_current_spo2, (long)g_algo_current_bpm, alarm ? 1 : 0);
							 
					p_task_telemetry_dta->state = ST_TEL_TX;
					HAL_UART_Transmit_DMA(&huart1, (uint8_t*)g_tx_buffer, strlen(g_tx_buffer));
				}
				else if (EV_TEL_SENSOR_ERR == p_task_telemetry_dta->event)
				{
					snprintf(g_tx_buffer, TX_BUFFER_SIZE, "{\"type\":\"error\",\"msg\":\"NO_FINGER\"}\n");
					
					p_task_telemetry_dta->state = ST_TEL_TX;
					HAL_UART_Transmit_DMA(&huart1, (uint8_t*)g_tx_buffer, strlen(g_tx_buffer));
				}
				else if (EV_TEL_RX_DATA == p_task_telemetry_dta->event)
				{
					/* Parse incoming command */
					char param[16];
					int val;
					if (2 == sscanf(g_rx_buffer, "CFG:%15[^:]:%d", param, &val))
					{
						int32_t spo2_min = -1;
						int32_t bpm_min = -1;
						int32_t bpm_max = -1;
						int8_t alarm_en = -1;
						
						if (0 == strcmp(param, "SPO2_MIN"))
						{
							spo2_min = val;
						}
						else if (0 == strcmp(param, "BPM_MIN"))
						{
							bpm_min = val;
						}
						else if (0 == strcmp(param, "BPM_MAX"))
						{
							bpm_max = val;
						}
						else if (0 == strcmp(param, "ALARM"))
						{
							alarm_en = val;
						}
						
						task_system_update_config_from_telemetry(spo2_min, bpm_min, bpm_max, alarm_en);
					}
				}
			}
			break;
			
		case ST_TEL_TX:
			if (true == p_task_telemetry_dta->flag)
			{
				p_task_telemetry_dta->flag = false;
				if (EV_TEL_TX_CPLT == p_task_telemetry_dta->event)
				{
					p_task_telemetry_dta->state = ST_TEL_IDLE;
				}
				else if (EV_TEL_TX_CPLT != p_task_telemetry_dta->event)
				{
					/* Re-encolar el evento para procesarlo cuando vuelva a IDLE */
					put_event_task_telemetry(p_task_telemetry_dta->event);
				}
			}
			break;
			
		default:
			p_task_telemetry_dta->state = ST_TEL_IDLE;
			break;
	}
}

/* ISR Callbacks a enlazar desde app.c */
void telemetry_rx_cplt_callback(UART_HandleTypeDef *huart)
{
	if (USART1 == huart->Instance)
	{
		if (('\n' == g_rx_byte) || ('\r' == g_rx_byte))
		{
			if (0 < g_rx_index)
			{
				g_rx_buffer[g_rx_index] = '\0';
				put_event_task_telemetry(EV_TEL_RX_DATA);
				g_rx_index = 0;
			}
		}
		else
		{
			if ((RX_BUFFER_SIZE - 1) > g_rx_index)
			{
				g_rx_buffer[g_rx_index++] = g_rx_byte;
			}
		}
		HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1);
	}
}

void telemetry_tx_cplt_callback(UART_HandleTypeDef *huart)
{
	if (USART1 == huart->Instance)
	{
		put_event_task_telemetry(EV_TEL_TX_CPLT);
	}
}
