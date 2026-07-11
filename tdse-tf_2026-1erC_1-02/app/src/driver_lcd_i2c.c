#include "driver_lcd_i2c.h"
#include "main.h"
#include "dwt.h"
#include <stdbool.h>

extern I2C_HandleTypeDef hi2c1;

#define LCD_I2C_ADDR (0x27 << 1) // Default PCF8574 address, might be 0x3F<<1

#define LCD_EN 0x04  // Enable bit
#define LCD_RW 0x02  // Read/Write bit
#define LCD_RS 0x01  // Register select bit
#define LCD_BL 0x08  // Backlight bit

#define DISPLAY_IR_CLEAR_DISPLAY   0b00000001
#define DISPLAY_IR_ENTRY_MODE_SET  0b00000100
#define DISPLAY_IR_DISPLAY_CONTROL 0b00001000
#define DISPLAY_IR_FUNCTION_SET    0b00100000
#define DISPLAY_IR_SET_DDRAM_ADDR  0b10000000

#define DISPLAY_IR_ENTRY_MODE_SET_INCREMENT 0b00000010
#define DISPLAY_IR_ENTRY_MODE_SET_NO_SHIFT  0b00000000

#define DISPLAY_IR_DISPLAY_CONTROL_DISPLAY_ON  0b00000100
#define DISPLAY_IR_DISPLAY_CONTROL_CURSOR_OFF  0b00000000
#define DISPLAY_IR_DISPLAY_CONTROL_BLINK_OFF   0b00000000

#define DISPLAY_IR_FUNCTION_SET_4BITS    0b00000000
#define DISPLAY_IR_FUNCTION_SET_2LINES   0b00001000
#define DISPLAY_IR_FUNCTION_SET_5x8DOTS  0b00000000

#define DISPLAY_20x4_LINE1_FIRST_CHARACTER_ADDRESS 0
#define DISPLAY_20x4_LINE2_FIRST_CHARACTER_ADDRESS 64
#define DISPLAY_20x4_LINE3_FIRST_CHARACTER_ADDRESS 20
#define DISPLAY_20x4_LINE4_FIRST_CHARACTER_ADDRESS 84

#define DISPLAY_RS_INSTRUCTION 0
#define DISPLAY_RS_DATA        1

static display_t display;
static uint16_t lcd_i2c_addr = (0x27 << 1); // Default

static void lcd_send_cmd(uint8_t cmd);
static void lcd_send_data(uint8_t data);
static void lcd_write_byte(uint8_t val);
static void delay_us(uint32_t us);

static void delay_us(uint32_t us)
{
    uint32_t start = cycle_counter_get();
    uint32_t cycles_per_us = SystemCoreClock / 1000000ul;
    while ((cycle_counter_get() - start) < (us * cycles_per_us));
}

static void lcd_write_byte(uint8_t val)
{
    uint8_t data = val | LCD_BL;
    
    // Si el bus I2C está siendo usado por una lectura/escritura asíncrona (IT)
    // del MAX30102 o la EEPROM, debemos esperar a que se libere.
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
        // Spin-wait (máximo ~500us para 6 bytes a 100kHz)
    }
    
    HAL_I2C_Master_Transmit(&hi2c1, lcd_i2c_addr, &data, 1, 10);
}

static void lcd_send_cmd(uint8_t cmd)
{
    uint8_t high_nib = (cmd & 0xF0) | LCD_BL;
    uint8_t low_nib  = ((cmd << 4) & 0xF0) | LCD_BL;
    
    // High nibble
    lcd_write_byte(high_nib | LCD_EN);
    delay_us(1);
    lcd_write_byte(high_nib & ~LCD_EN);
    delay_us(50);
    
    // Low nibble
    lcd_write_byte(low_nib | LCD_EN);
    delay_us(1);
    lcd_write_byte(low_nib & ~LCD_EN);
    delay_us(50);
}

static void lcd_send_data(uint8_t data)
{
    uint8_t high_nib = (data & 0xF0) | LCD_RS | LCD_BL;
    uint8_t low_nib  = ((data << 4) & 0xF0) | LCD_RS | LCD_BL;
    
    // High nibble
    lcd_write_byte(high_nib | LCD_EN);
    delay_us(1);
    lcd_write_byte(high_nib & ~LCD_EN);
    delay_us(50);
    
    // Low nibble
    lcd_write_byte(low_nib | LCD_EN);
    delay_us(1);
    lcd_write_byte(low_nib & ~LCD_EN);
    delay_us(50);
}

void displayInit(displayConnection_t connection)
{
    display.connection = connection;
    
    // Auto-detect I2C address
    if (HAL_I2C_IsDeviceReady(&hi2c1, (0x27 << 1), 3, 100) == HAL_OK) {
        lcd_i2c_addr = (0x27 << 1);
    } else if (HAL_I2C_IsDeviceReady(&hi2c1, (0x3F << 1), 3, 100) == HAL_OK) {
        lcd_i2c_addr = (0x3F << 1);
    }
    
    HAL_Delay(50);
    
    // Init in 4-bit mode
    lcd_write_byte(0x30 | LCD_EN);
    delay_us(1);
    lcd_write_byte(0x30 & ~LCD_EN);
    HAL_Delay(5);
    
    lcd_write_byte(0x30 | LCD_EN);
    delay_us(1);
    lcd_write_byte(0x30 & ~LCD_EN);
    HAL_Delay(1);
    
    lcd_write_byte(0x30 | LCD_EN);
    delay_us(1);
    lcd_write_byte(0x30 & ~LCD_EN);
    HAL_Delay(1);
    
    // Set to 4-bit interface
    lcd_write_byte(0x20 | LCD_EN);
    delay_us(1);
    lcd_write_byte(0x20 & ~LCD_EN);
    HAL_Delay(1);
    
    lcd_send_cmd(DISPLAY_IR_FUNCTION_SET | DISPLAY_IR_FUNCTION_SET_4BITS | DISPLAY_IR_FUNCTION_SET_2LINES | DISPLAY_IR_FUNCTION_SET_5x8DOTS);
    HAL_Delay(1);
    
    lcd_send_cmd(DISPLAY_IR_DISPLAY_CONTROL | DISPLAY_IR_DISPLAY_CONTROL_DISPLAY_ON | DISPLAY_IR_DISPLAY_CONTROL_CURSOR_OFF | DISPLAY_IR_DISPLAY_CONTROL_BLINK_OFF);
    HAL_Delay(1);
    
    lcd_send_cmd(DISPLAY_IR_CLEAR_DISPLAY);
    HAL_Delay(2);
    
    lcd_send_cmd(DISPLAY_IR_ENTRY_MODE_SET | DISPLAY_IR_ENTRY_MODE_SET_INCREMENT | DISPLAY_IR_ENTRY_MODE_SET_NO_SHIFT);
    HAL_Delay(1);
}

void displayCharPositionWrite(uint8_t charPositionX, uint8_t charPositionY)
{
    switch(charPositionY) {
        case 0:
            lcd_send_cmd(DISPLAY_IR_SET_DDRAM_ADDR | (DISPLAY_20x4_LINE1_FIRST_CHARACTER_ADDRESS + charPositionX));
            break;
        case 1:
            lcd_send_cmd(DISPLAY_IR_SET_DDRAM_ADDR | (DISPLAY_20x4_LINE2_FIRST_CHARACTER_ADDRESS + charPositionX));
            break;
        case 2:
            lcd_send_cmd(DISPLAY_IR_SET_DDRAM_ADDR | (DISPLAY_20x4_LINE3_FIRST_CHARACTER_ADDRESS + charPositionX));
            break;
        case 3:
            lcd_send_cmd(DISPLAY_IR_SET_DDRAM_ADDR | (DISPLAY_20x4_LINE4_FIRST_CHARACTER_ADDRESS + charPositionX));
            break;
    }
}

void displayStringWrite(const char *str)
{
    while (*str) {
        lcd_send_data(*str++);
    }
}

void displayDataWrite(const char data)
{
    lcd_send_data(data);
}
