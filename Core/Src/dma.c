/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.c
  * @brief   This file provides code for the configuration
  *          of all the requested memory to memory DMA transfers.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "dma.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure DMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
DMA_HandleTypeDef hdma_memtomem_dma1_stream7;

/**
  * Enable DMA controller clock
  * Configure DMA for memory to memory transfers
  *   hdma_memtomem_dma1_stream7
  */
void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_dma1_stream7 on DMA1_Stream7 */
  hdma_memtomem_dma1_stream7.Instance = DMA1_Stream7;
  hdma_memtomem_dma1_stream7.Init.Request = DMA_REQUEST_MEM2MEM;
  hdma_memtomem_dma1_stream7.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma1_stream7.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma1_stream7.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma1_stream7.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_memtomem_dma1_stream7.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_memtomem_dma1_stream7.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma1_stream7.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  hdma_memtomem_dma1_stream7.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdma_memtomem_dma1_stream7.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  hdma_memtomem_dma1_stream7.Init.MemBurst = DMA_MBURST_INC4;
  hdma_memtomem_dma1_stream7.Init.PeriphBurst = DMA_PBURST_INC4;
  if (HAL_DMA_Init(&hdma_memtomem_dma1_stream7) != HAL_OK)
  {
    Error_Handler();
  }

  /* DMA interrupt init */
  /* DMA1_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

