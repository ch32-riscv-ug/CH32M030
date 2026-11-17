/*********************************************************************
 * File Name          : spi.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/8/15
 * Description        : file for spi.
 *********************************************************************************
* Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "spi.h"

/*********************************************************************
 * @fn      SPI_LCD_Init
 * 
 * @brief   Initialize SPI1 for LCD (default mapping: SCK-PA1, MOSI-PC3)
 * 
 * @return  none
 */
void SPI_LCD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    SPI_InitTypeDef  SPI_InitStructure  = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOA | RCC_PB2Periph_GPIOC | RCC_PB2Periph_SPI1, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction         = SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial     = 7;
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
}

/*********************************************************************
 * @fn      SPI_LCD_DMA_Init
 * 
 * @brief   Initialize DMA for LCD SPI TX
 * 
 * @return  none
 */
void SPI_LCD_DMA_Init()
{
    DMA_InitTypeDef DMA_InitStructure = {0};

    RCC_HBPeriphClockCmd(RCC_HBPeriph_DMA1, ENABLE);

    DMA_DeInit(SPI_LCD_DMA_TX_CH);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DATAR;
    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)NULL;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize         = 0;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(SPI_LCD_DMA_TX_CH, &DMA_InitStructure);

    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
}

/*********************************************************************
 * @fn      SPI_LCD_Write
 * 
 * @brief   send one byte
 * 
 * @param   data -  the data to send
 * 
 * @return  none
 */
void SPI_LCD_Write(uint8_t data)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
        ;
    SPI_I2S_SendData(SPI1, data);

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET)
        ;
}

/*********************************************************************
 * @fn      SPI_LCD_SetDataSize
 *
 * @brief   切换 SPI1 帧长度（8 位/16 位）
 *
 * @param   dataSize - SPI_DataSize_8b 或 SPI_DataSize_16b
 *
 * @return  none
 */
void SPI_LCD_SetDataSize(uint16_t dataSize)
{
    uint16_t ctlr1;

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET)
        ;

    /* DFF 位只能在 SPI 关闭时修改 */
    SPI_Cmd(SPI1, DISABLE);
    ctlr1 = SPI1->CTLR1;
    ctlr1 &= (uint16_t)(~SPI_DataSize_16b);
    ctlr1 |= dataSize;
    SPI1->CTLR1 = ctlr1;
    SPI_Cmd(SPI1, ENABLE);
}

/*********************************************************************
 * @fn      SPI_LCD_Write_DMA16
 *
 * @brief   SPI1 16 位帧 DMA 发送（一个半字对应一个像素）
 *
 * @param   data  - 半字缓冲，直接存 RGB565 颜色，无需交换字节序
 *          count - 半字个数
 *
 * @return  none
 */
void SPI_LCD_Write_DMA16(uint16_t *data, uint16_t count)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET)
        ;

    DMA_Cmd(SPI_LCD_DMA_TX_CH, DISABLE);
    DMA_ClearFlag(SPI_LCD_DMA_TC_FLAG);

    SPI_LCD_DMA_TX_CH->MADDR = (uint32_t)data;
    SPI_LCD_DMA_TX_CH->CNTR  = count;
    /* 0x2590: 存储器到外设、地址递增、高优先级、半字宽度 */
    SPI_LCD_DMA_TX_CH->CFGR = 0x2590;

    DMA_Cmd(SPI_LCD_DMA_TX_CH, ENABLE);
}
