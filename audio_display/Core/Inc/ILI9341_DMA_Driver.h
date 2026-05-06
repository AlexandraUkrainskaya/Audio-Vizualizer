//
//--------------------------------------------
//	ILI9341 DMA Driver library for STM32 HAL
//	https://github.com/yuujiin/STM32_ILI9341_DMA_Driver
//--------------------------------------------
//
//
//	MIT License
//
//	Copyright (c) 2020 yuujiin
//	https://github.com/yuujiin
//
//	Based on library by Matej Artnak
//	Copyright (c) 2017 Matej Artnak
//	https://github.com/martnak/STM32-ILI9341
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.
//
//
//-----------------------------------
//	How to use this library
//-----------------------------------
//
//	* Generate SPI peripheral and 3 GPIO outputs
//		* STM32Cube code generation: separate .c/.h files for each peripheral
//		* SPI peripheral: CPOL=LOW, CPHA=1EDGE, 8 bits, MSB first, Software NSS, up to 50 MHz
//		* Tx DMA for SPI: Half Word (16 bits) data width
//		* GPIO: CS, DC and RST outputs
//		* GPIO_SPEED_FREQ_VERY_HIGH for CS and DC pins
//		* GPIO initial values: RST=Low, CS=High - LCD is in reset state before ILI_Init()
//	* Configure parameters in ILI9341_Driver.h:
//		* Define your ILI_SPI_HANDLE
//		* Define your CS, DC and RST outputs
//		* Check if ILI_SCREEN_WIDTH and ILI_SCREEN_HEIGHT match your LCD size and orientation.
//	* In your main program initialize LCD with ILI_Init() before use
//
//-----------------------------------
//	Code example
//-----------------------------------
//
//	/* Includes */
//	#include "ILI9341_DMA_Driver.h"
//	/* Global vars */
//	volatile uint16_t display_buf[ILI_SCREEN_WIDTH*ILI_SCREEN_HEIGHT] __ALIGNED(32);	// aligned for DCache
//	/* Interrupt callback */
//	void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
//		if (hspi->Instance == ILI_SPI_HANDLE.Instance) {
//			ILI_DMA_Callback();
//		}
//	}
//	/* Main program init section */
//	ILI_Init();
//	/* Splash screen */
//	ILI_DMA_Fill(BLACK);
//	while (ILI_DMA_Busy());
//	/* Draw something */
//	uint32_t i;
//	for (i = 0; i < (ILI_SCREEN_WIDTH*ILI_SCREEN_HEIGHT); i++) {
//	  if (i < ILI_SCREEN_HEIGHT/4*ILI_SCREEN_WIDTH) {
//		  display_buf[i] = RED;
//	  } else if (i < ILI_SCREEN_HEIGHT/2*ILI_SCREEN_WIDTH) {
//		  display_buf[i] = GREEN;
//	  } else if (i < ILI_SCREEN_HEIGHT/4*3*ILI_SCREEN_WIDTH) {
//		  display_buf[i] = BLUE;
//	  } else {
//		  display_buf[i] = YELLOW;
//	  }
//	}
//	/* Load frame buffer */
//	ILI_DMA_Load((uint16_t *)display_buf);
//
//-----------------------------------


#ifndef ILI9341_DMA_DRIVER_H
#define ILI9341_DMA_DRIVER_H

#include "main.h"
#include "spi.h"


/* Configuration ------------------------------------------------------ */

// LCD RESOLUTION
#define ILI_SCREEN_WIDTH 	320
#define ILI_SCREEN_HEIGHT 	240

// SCREEN ORIENTATION
#define ILI_ROTATION		SCREEN_HORIZONTAL_1

// SPI HANDLE
#define ILI_SPI_HANDLE		hspi3

// CHIP SELECT PIN AND PORT, STANDARD GPIO
#define ILI_CS_PORT			Screen_CS_GPIO_Port
#define ILI_CS_PIN			Screen_CS_Pin

// DATA COMMAND PIN AND PORT, STANDARD GPIO
#define ILI_DC_PORT			Screen_DC_GPIO_Port
#define ILI_DC_PIN			Screen_DC_Pin

// RESET PIN AND PORT, STANDARD GPIO
#define	ILI_RST_PORT		Screen_RST_GPIO_Port
#define	ILI_RST_PIN			Screen_RST_Pin

// DCACHE (Cortex-M7)
#define ILI_DCACHE_ENABLED

/* -------------------------------------------------------------------- */


/* RGB565 Colors */
#define RGB(R,G,B)  ((uint16_t)((((uint8_t)R >> 3) << 11) | (((uint8_t)G >> 2) << 5) | ((uint8_t)B >> 3)))
#define BLACK       0x0000
#define NAVY        0x000F
#define DARKGREEN   0x03E0
#define DARKCYAN    0x03EF
#define MAROON      0x7800
#define PURPLE      0x780F
#define OLIVE       0x7BE0
#define LIGHTGREY   0xC618
#define DARKGREY    0x7BEF
#define BLUE        0x001F
#define GREEN       0x07E0
#define CYAN        0x07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define ORANGE      0xFD20
#define GREENYELLOW 0xAFE5
#define PINK        0xF81F

/* Screen orientation */
#define SCREEN_VERTICAL_1		0
#define SCREEN_HORIZONTAL_1		1
#define SCREEN_VERTICAL_2		2
#define SCREEN_HORIZONTAL_2		3


/* Configuration functions */
void ILI_Reset(void);
void ILI_Enable(void);
void ILI_Init(void);
HAL_StatusTypeDef ILI_Write_Command(uint8_t Command);
HAL_StatusTypeDef ILI_Write_Data(uint8_t Data);
void ILI_Set_Rotation(uint8_t Rotation);
void ILI_Set_Address(uint16_t X, uint16_t Y, uint16_t W, uint16_t H);

/* DMA functions */
HAL_StatusTypeDef ILI_DMA_Load(uint16_t *Buf);
HAL_StatusTypeDef ILI_DMA_Fill(uint16_t Color);
HAL_StatusTypeDef ILI_DMA_Callback(void);
uint8_t ILI_DMA_Busy(void);


#endif
