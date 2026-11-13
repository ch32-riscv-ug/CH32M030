/*********************************************************************
 * File Name          : spi.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/8/15
 * Description        : file for spi.
 *********************************************************************************
* Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __SPI_H__
#define __SPI_H__

#include "debug.h"
#include "config.h"

#define SPI_LCD             SPI1
#define SPI_LCD_DMA_TX_CH   DMA1_Channel3

#define SPI_LCD_DMA_TC_FLAG DMA1_FLAG_TC3

void SPI_LCD_Init(void);
void SPI_LCD_DMA_Init(void);
void SPI_LCD_Write(uint8_t data);
void SPI_LCD_SetDataSize(uint16_t dataSize);
void SPI_LCD_Write_DMA16(uint16_t *data, uint16_t count);

#endif
