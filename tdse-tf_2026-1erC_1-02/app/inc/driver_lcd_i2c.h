#ifndef APP_INC_DRIVER_LCD_I2C_H_
#define APP_INC_DRIVER_LCD_I2C_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include <stdint.h>

/********************** macros ***********************************************/
// Connection types (kept for compatibility with task_display.c)
typedef enum {
    DISPLAY_CONNECTION_GPIO_4BITS,
    DISPLAY_CONNECTION_GPIO_8BITS,
    DISPLAY_CONNECTION_I2C
} displayConnection_t;

/********************** typedef **********************************************/
typedef struct {
    displayConnection_t connection;
} display_t;

/********************** external data declaration ****************************/

/********************** external functions declaration ***********************/
void displayInit(displayConnection_t connection);
void displayCharPositionWrite(uint8_t charPositionX, uint8_t charPositionY);
void displayStringWrite(const char *str);
void displayDataWrite(const char data);

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* APP_INC_DRIVER_LCD_I2C_H_ */
