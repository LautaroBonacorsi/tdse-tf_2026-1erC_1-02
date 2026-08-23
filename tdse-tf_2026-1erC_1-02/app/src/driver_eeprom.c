/*
 * driver_eeprom.c
 *
 * Driver no bloqueante para EEPROM I2C (ej. AT24C32/AT24C256)
 */

#include "driver_eeprom.h"
#include "logger.h"

static I2C_HandleTypeDef *g_eeprom_hi2c = NULL;
static volatile eeprom_status_t g_eeprom_state = EEPROM_READY;

/* 
 * Máquina de estados interna para writes.
 * Cuando se pide un Write IT, I2C transmite los datos. Al terminar, la EEPROM entra
 * en un ciclo interno de escritura (write cycle, ~5ms). Durante ese ciclo no responde (NACK).
 * Debemos hacer polling con HAL_I2C_IsDeviceReady hasta que responda con ACK, indicando que terminó.
 */
static bool b_wait_for_ready_flag = false;
static uint32_t g_wait_start_tick = 0;
#define EEPROM_WRITE_TIMEOUT_MS 20ul

void eeprom_init(I2C_HandleTypeDef *hi2c)
{
	g_eeprom_hi2c = hi2c;
	g_eeprom_state = EEPROM_READY;
	b_wait_for_ready_flag = false;
	LOGGER_INFO("EEPROM Driver Initialized");
}

void eeprom_update(void)
{
	if (true == b_wait_for_ready_flag)
	{
		/* Hacemos polling de la EEPROM para ver si terminó el ciclo de escritura interno.
		 * Usamos un timeout mínimo (1 trial, 1ms timeout) para no bloquear el superloop.
		 */
		if (HAL_OK == HAL_I2C_IsDeviceReady(g_eeprom_hi2c, EEPROM_I2C_ADDR, 1, 1))
		{
			b_wait_for_ready_flag = false;
			g_eeprom_state = EEPROM_READY;
			LOGGER_INFO("EEPROM Write Cycle Complete");
		}
		else
		{
			/* Timeout safety check */
			if (EEPROM_WRITE_TIMEOUT_MS < (HAL_GetTick() - g_wait_start_tick))
			{
				b_wait_for_ready_flag = false;
				g_eeprom_state = EEPROM_ERROR;
				LOGGER_ERROR("EEPROM Write Cycle Timeout!");
			}
		}
	}
}

eeprom_status_t eeprom_read_it(uint16_t mem_address, uint8_t *data, uint16_t size)
{
	if (EEPROM_READY != g_eeprom_state)
	{
		return EEPROM_BUSY;
	}

	g_eeprom_state = EEPROM_BUSY;
	
	if (HAL_OK != HAL_I2C_Mem_Read_IT(g_eeprom_hi2c, EEPROM_I2C_ADDR, mem_address, I2C_MEMADD_SIZE_16BIT, data, size))
	{
		g_eeprom_state = EEPROM_ERROR;
		return EEPROM_ERROR;
	}
	
	return EEPROM_OK;
}

eeprom_status_t eeprom_write_it(uint16_t mem_address, uint8_t *data, uint16_t size)
{
	if (EEPROM_READY != g_eeprom_state)
	{
		return EEPROM_BUSY;
	}

	g_eeprom_state = EEPROM_BUSY;
	
	if (HAL_OK != HAL_I2C_Mem_Write_IT(g_eeprom_hi2c, EEPROM_I2C_ADDR, mem_address, I2C_MEMADD_SIZE_16BIT, data, size))
	{
		g_eeprom_state = EEPROM_ERROR;
		return EEPROM_ERROR;
	}
	
	return EEPROM_OK;
}

eeprom_status_t eeprom_get_status(void)
{
	return g_eeprom_state;
}

/* 
 * Callbacks de interrupción de I2C.
 * Deben ser llamadas desde HAL_I2C_MemTxCpltCallback y HAL_I2C_MemRxCpltCallback en main.c o it.c.
 * Por convención STM32, se redefinen los callbacks de HAL como __weak en hal_i2c.c,
 * por lo que podemos implementarlos aquí de manera global o en un archivo dedicado de callbacks.
 */

void eeprom_rx_cplt_callback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == g_eeprom_hi2c->Instance)
	{
		g_eeprom_state = EEPROM_READY; // Read complete immediately
	}
}

void eeprom_tx_cplt_callback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == g_eeprom_hi2c->Instance)
	{
		/* Transmisión I2C completada. Ahora comienza el ciclo interno de la EEPROM. */
		b_wait_for_ready_flag = true;
		g_wait_start_tick = HAL_GetTick();
	}
}

void eeprom_error_callback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == g_eeprom_hi2c->Instance)
	{
		g_eeprom_state = EEPROM_ERROR;
		b_wait_for_ready_flag = false;
	}
}

bool eeprom_test_blocking(void)
{
	uint8_t test_data_write[4] = {0xDE, 0xAD, 0xBE, 0xEF};
	uint8_t test_data_read[4] = {0, 0, 0, 0};
	
	if (NULL == g_eeprom_hi2c)
	{
		return false;
	}

	LOGGER_INFO("Test EEPROM: Escribiendo firma (0xDEADBEEF)...");
	if (HAL_OK != HAL_I2C_Mem_Write(g_eeprom_hi2c, EEPROM_I2C_ADDR, 0x0000, I2C_MEMADD_SIZE_16BIT, test_data_write, 4, 100))
	{
		LOGGER_ERROR("Test EEPROM: Fallo escritura I2C. Revisar SDA/SCL.");
		return false;
	}
	
	LOGGER_INFO("Test EEPROM: Esperando write cycle (10ms)...");
	HAL_Delay(10); 
	
	LOGGER_INFO("Test EEPROM: Leyendo...");
	if (HAL_OK != HAL_I2C_Mem_Read(g_eeprom_hi2c, EEPROM_I2C_ADDR, 0x0000, I2C_MEMADD_SIZE_16BIT, test_data_read, 4, 100))
	{
		LOGGER_ERROR("Test EEPROM: Fallo lectura I2C.");
		return false;
	}
	
	if ((0xDE == test_data_read[0]) && (0xAD == test_data_read[1]) && (0xBE == test_data_read[2]) && (0xEF == test_data_read[3]))
	{
		LOGGER_INFO("Test EEPROM: EXITO! Datos leídos correctamente (Hardware OK).");
		return true;
	}
	else
	{
		LOGGER_ERROR("Test EEPROM: Datos corruptos: %02X %02X %02X %02X", test_data_read[0], test_data_read[1], test_data_read[2], test_data_read[3]);
		return false;
	}
}
