/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar>.
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

#ifndef BOARD_H_
#define BOARD_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "main.h"

/********************** macros ***********************************************/
/* NUCLEO_F103RB Board Definition */

/* Buttons (PC0 to PC3) */
#define BTN_MENU_PORT   GPIOC
#define BTN_MENU_PIN    GPIO_PIN_0
#define BTN_UP_PORT     GPIOC
#define BTN_UP_PIN      GPIO_PIN_1
#define BTN_DOWN_PORT   GPIOC
#define BTN_DOWN_PIN    GPIO_PIN_2
#define BTN_ACK_PORT    GPIOC
#define BTN_ACK_PIN     GPIO_PIN_3

/* DIP Switch (PA0, PA1, PA4, PB0) */
#define DIP1_PORT       GPIOA
#define DIP1_PIN        GPIO_PIN_0
#define DIP2_PORT       GPIOA
#define DIP2_PIN        GPIO_PIN_1
#define DIP3_PORT       GPIOA
#define DIP3_PIN        GPIO_PIN_4
#define DIP4_PORT       GPIOB
#define DIP4_PIN        GPIO_PIN_0

/* LEDs (PB1 Yellow, PB2 Red) */
#define LED_Y_PORT      GPIOB
#define LED_Y_PIN       GPIO_PIN_1
#define LED_R_PORT      GPIOB
#define LED_R_PIN       GPIO_PIN_2

/* MAX30102 INT (PB10) */
#define MAX_INT_PORT    GPIOB
#define MAX_INT_PIN     GPIO_PIN_10

/* States for inputs/outputs */
/* Asumiendo Pull-Down externo o interno configurado en CubeMX para botones */
#define BTN_PRESSED     GPIO_PIN_SET    
#define BTN_RELEASED    GPIO_PIN_RESET

#define DIP_ON          GPIO_PIN_SET
#define DIP_OFF         GPIO_PIN_RESET

#define LED_ON          GPIO_PIN_SET
#define LED_OFF         GPIO_PIN_RESET

/********************** typedef **********************************************/

/********************** external data declaration ****************************/

/********************** external functions declaration ***********************/

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* BOARD_H_ */

/********************** end of file ******************************************/
