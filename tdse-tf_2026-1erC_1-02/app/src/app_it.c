/*
 * @file   : app_it.c
 * @brief  : Application interrupt handlers (callbacks)
 */

/********************** inclusions *******************************************/
#include "main.h"
#include "app_it.h"

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/
volatile uint32_t g_app_tick_cnt = 0;

/********************** external functions definition ************************/
void HAL_SYSTICK_Callback(void)
{
	g_app_tick_cnt++;
}

/********************** end of file ******************************************/
