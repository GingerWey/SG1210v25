//-----------------------------------------------------------------------------
/*
   File        : SPI.c
   Version     : V2.2
   By          : 银网科技

   Description :SPI初始化
          
   Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------
#include "spi.h"

//-----------------------------------------------------------------------------
#include "SPIChls.h"

#include "DevRegs.h"
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  SPI1 = W5500
//-----------------------------------------------------------------------------
// SPI1 init function
// SPI1: APB2 84M
// W5500             最高50MHz
// MB85RS            最高20MHz
// FLASH             最高50MHz
void MX_SPI1_Init(void)
{

  SPI_HandleTypeDef *hspi = &SPICtrls[SPI1_CHL].SPI;
  
  hspi->Instance = SPI1;
  hspi->Init.Mode           = SPI_MODE_MASTER;
  hspi->Init.Direction      = SPI_DIRECTION_2LINES;
  hspi->Init.DataSize       = SPI_DATASIZE_8BIT;
  hspi->Init.CLKPolarity    = SPI_POLARITY_HIGH;
  hspi->Init.CLKPhase       = SPI_PHASE_2EDGE;
  hspi->Init.NSS            = SPI_NSS_SOFT;
  hspi->Init.FirstBit       = SPI_FIRSTBIT_MSB;
  hspi->Init.TIMode         = SPI_TIMODE_DISABLE;
  hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi->Init.CRCPolynomial  = 7;
  hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;   // 42M
  if (HAL_SPI_Init(hspi) != HAL_OK)
    {
    SetHWFault( RHF_SPI_ERR );
    }
}
//-----------------------------------------------------------------------------
void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(spiHandle->Instance == SPI1)
  {
    /* SPI1 clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();
  
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**SPI1 GPIO Configuration    
    PB3     ------> SPI1_SCK
    PB4     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI 
    */
    GPIO_InitStruct.Pin   = NvRAM_SCLK_Pin | NvRAM_MISO_Pin | NvRAM_MOSI_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(NvRAM_SCLK_Port, &GPIO_InitStruct);

    /* SPI1 DMA Init */
    /* SPI1_RX Init */
    DMA_HandleTypeDef *hdma = &SPICtrls[SPI1_CHL].rxDMA;
    hdma->Instance = DMA2_Stream0;
    hdma->Init.Channel   = DMA_CHANNEL_3;
    hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma->Init.PeriphInc = DMA_PINC_DISABLE;
    hdma->Init.MemInc    = DMA_MINC_ENABLE;
    hdma->Init.Mode      = DMA_NORMAL;
    hdma->Init.Priority  = DMA_PRIORITY_LOW;
    hdma->Init.FIFOMode  = DMA_FIFOMODE_DISABLE;
    hdma->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    if (HAL_DMA_Init(hdma) != HAL_OK)
      {
      SetHWFault( RHF_SPI_ERR );
      }

    __HAL_LINKDMA(spiHandle, hdmarx, *hdma);

    /* SPI1_TX Init */
    hdma = &SPICtrls[SPI1_CHL].txDMA;
    hdma->Instance = DMA2_Stream3;
    hdma->Init.Channel   = DMA_CHANNEL_3;
    hdma->Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma->Init.PeriphInc = DMA_PINC_DISABLE;
    hdma->Init.MemInc    = DMA_MINC_ENABLE;
    hdma->Init.Mode      = DMA_NORMAL;
    hdma->Init.Priority  = DMA_PRIORITY_LOW;
    hdma->Init.FIFOMode  = DMA_FIFOMODE_DISABLE;
    hdma->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    if (HAL_DMA_Init(hdma) != HAL_OK)
      {
      SetHWFault( RHF_SPI_ERR );
      }

    __HAL_LINKDMA(spiHandle, hdmatx, *hdma);

    /* SPI1 interrupt Init */
    HAL_NVIC_SetPriority(SPI1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
  }
}
//-----------------------------------------------------------------------------
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{

  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspDeInit 0 */

  /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();
  
    /**SPI1 GPIO Configuration    
    PB3     ------> SPI1_SCK
    PB4     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI 
    */
    HAL_GPIO_DeInit(NvRAM_SCLK_Port, 
                    NvRAM_SCLK_Pin | NvRAM_MISO_Pin | NvRAM_MOSI_Pin);

    /* SPI1 DMA DeInit */
    HAL_DMA_DeInit(spiHandle->hdmarx);
    HAL_DMA_DeInit(spiHandle->hdmatx);

    /* SPI1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SPI1_IRQn);
  /* USER CODE BEGIN SPI1_MspDeInit 1 */

  /* USER CODE END SPI1_MspDeInit 1 */
  }
} 
//-----------------------------------------------------------------------------
