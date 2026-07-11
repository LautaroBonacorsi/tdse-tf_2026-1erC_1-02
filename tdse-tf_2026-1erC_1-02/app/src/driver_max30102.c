/*
 * driver_max30102.c
 */

#include "driver_max30102.h"
#include "board.h"
#include "logger.h"
#include "algorithm.h"

#define REG_INTR_STATUS_1 0x00
#define REG_INTR_ENABLE_1 0x02
#define REG_FIFO_WR_PTR   0x04
#define REG_OVF_COUNTER   0x05
#define REG_FIFO_RD_PTR   0x06
#define REG_FIFO_DATA     0x07
#define REG_FIFO_CONFIG   0x08
#define REG_MODE_CONFIG   0x09
#define REG_SPO2_CONFIG   0x0A
#define REG_LED1_PA       0x0C // Red
#define REG_LED2_PA       0x0D // IR

static I2C_HandleTypeDef *g_max_hi2c = NULL;
static uint8_t g_fifo_buffer[6];
static bool b_reading = false;

static void max30102_write_reg(uint8_t reg, uint8_t value)
{
	HAL_I2C_Mem_Write(g_max_hi2c, MAX30102_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100);
}

void max30102_init(I2C_HandleTypeDef *hi2c)
{
	g_max_hi2c = hi2c;
	b_reading = false;
	
	// Reset sensor
	max30102_write_reg(REG_MODE_CONFIG, 0x40);
	HAL_Delay(100);
	
	// Configurar interrupciones: Habilitar PPG_RDY (A_FULL = 0, PPG_RDY = 1, ALC_OVF = 0)
	max30102_write_reg(REG_INTR_ENABLE_1, 0x40);
	
	// FIFO Config: sample average = 1 (0x00), roll on full = 1 (0x10), FIFO almost full = 0 (0x0F) -> 0x1F
	max30102_write_reg(REG_FIFO_CONFIG, 0x1F);
	
	// Mode Config: SpO2 mode (0x03)
	max30102_write_reg(REG_MODE_CONFIG, 0x03);
	
	// SpO2 Config: SPO2_ADC_RGE = 2048 (0x20), SPO2_SR = 100 (0x04), LED_PW = 411 (0x03)
	max30102_write_reg(REG_SPO2_CONFIG, 0x27);
	
	// LED1 (Red) and LED2 (IR) Pulse Amplitude
	max30102_write_reg(REG_LED1_PA, 0x24); // ~7mA
	max30102_write_reg(REG_LED2_PA, 0x24); // ~7mA
	
	// Reset FIFO Pointers
	max30102_write_reg(REG_FIFO_WR_PTR, 0x00);
	max30102_write_reg(REG_OVF_COUNTER, 0x00);
	max30102_write_reg(REG_FIFO_RD_PTR, 0x00);
	
	uint8_t temp;
	HAL_I2C_Mem_Read(g_max_hi2c, MAX30102_I2C_ADDR, REG_INTR_STATUS_1, I2C_MEMADD_SIZE_8BIT, &temp, 1, 100); // Clear interrupts
	
	LOGGER_INFO("MAX30102 Initialized");
}

void max30102_update(void)
{
	// Polling the INT pin (PB10) which is active low when PPG_RDY is fired
	if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(MAX_INT_PORT, MAX_INT_PIN))
	{
		// Lectura bloqueante (toma ~600us). Evita corrupción de estado HAL_I2C al mezclarse con el LCD.
		if (HAL_OK == HAL_I2C_Mem_Read(g_max_hi2c, MAX30102_I2C_ADDR, REG_FIFO_DATA, I2C_MEMADD_SIZE_8BIT, g_fifo_buffer, 6, 10))
		{
			// Reconstruir 24 bits
			uint32_t red = ((uint32_t)g_fifo_buffer[0] << 16) | ((uint32_t)g_fifo_buffer[1] << 8) | g_fifo_buffer[2];
			red &= 0x03FFFFul; 
			
			uint32_t ir = ((uint32_t)g_fifo_buffer[3] << 16) | ((uint32_t)g_fifo_buffer[4] << 8) | g_fifo_buffer[5];
			ir &= 0x03FFFFul;
			
			algorithm_process_sample(red, ir);
		}
	}
}
