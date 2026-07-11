/*
 * driver_eeprom.h
 *
 * Driver no bloqueante para EEPROM I2C (ej. AT24C32/AT24C256)
 */
#ifndef DRIVER_EEPROM_H_
#define DRIVER_EEPROM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* I2C Address for standard 24Cxx EEPROM (A0=A1=A2=0 -> 0x50 << 1 = 0xA0) */
#define EEPROM_I2C_ADDR 0xA0

/* Estructura para configuración de EEPROM */
typedef struct {
	I2C_HandleTypeDef *hi2c;
	uint16_t device_address;
	uint16_t page_size;    // e.g. 32 for AT24C32, 64 for AT24C256
} eeprom_cfg_t;

/* Status codes */
typedef enum {
	EEPROM_OK = 0,
	EEPROM_BUSY,
	EEPROM_ERROR,
	EEPROM_READY
} eeprom_status_t;

void eeprom_init(I2C_HandleTypeDef *hi2c);
void eeprom_update(void); // Must be called in superloop

/* Inicia lectura. Retorna EEPROM_OK si inició, BUSY si está ocupado.
 * 'data' debe apuntar a un buffer que exista hasta que la lectura termine.
 */
eeprom_status_t eeprom_read_it(uint16_t mem_address, uint8_t *data, uint16_t size);

/* Inicia escritura. Retorna EEPROM_OK si inició, BUSY si está ocupado.
 * Soporta escritura de una página (no cruza boundaries por simplicidad, o se asume tamaño corto).
 */
eeprom_status_t eeprom_write_it(uint16_t mem_address, uint8_t *data, uint16_t size);

eeprom_status_t eeprom_get_status(void);

/* Callbacks para enlazar con HAL_I2C */
void eeprom_rx_cplt_callback(I2C_HandleTypeDef *hi2c);
void eeprom_tx_cplt_callback(I2C_HandleTypeDef *hi2c);
void eeprom_error_callback(I2C_HandleTypeDef *hi2c);

/* Función de prueba bloqueante para verificar conexión hardware (usar solo en INIT) */
bool eeprom_test_blocking(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_EEPROM_H_ */
