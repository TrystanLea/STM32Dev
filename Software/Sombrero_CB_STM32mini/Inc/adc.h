/**
  ******************************************************************************
  * File Name          : ADC.h
  * Description        : This file provides code for the configuration
  *                      of the ADC instances.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2021 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __adc_H
#define __adc_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdbool.h>

// #define ADC_DMA_BUFFSIZE 6000    // must me integer multiple of number of channels?
// volatile uint16_t adcV_dma_buff[ADC_DMA_BUFFSIZE];
// volatile uint16_t adcI_dma_buff[ADC_DMA_BUFFSIZE];

// total buffer = ADC_DMA_BUFFSIZE_PERCHANNEL * CTn
#define ADC_DMA_BUFFSIZE_PERCHANNEL 2000
#define CTn 3 // !!! number of CT channels, changing this number sould correlate with scan conversion settings in cubeMx.

uint16_t const adc_buff_size;
uint16_t const adc_buff_half_size;
uint16_t adcV_dma_buff[CTn * ADC_DMA_BUFFSIZE_PERCHANNEL];
uint16_t adcI_dma_buff[CTn * ADC_DMA_BUFFSIZE_PERCHANNEL];
//uint16_t adc4_dma_buff[ADC_DMA_BUFFSIZE_PERCHANNEL];
bool conv_hfcplt_flag;
bool conv_cplt_flag;
bool adc_buffer_overflow;

int32_t usec_lag;

extern DMA_HandleTypeDef hdma_adc2;
extern DMA_HandleTypeDef hdma_adc4;
/* USER CODE END Includes */

extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc4;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

extern void _Error_Handler(char *, int);

void MX_ADC2_Init(void);
void MX_ADC4_Init(void);

/* USER CODE BEGIN Prototypes */
void start_ADCs (int32_t usec_lag);
// void process_frame(uint16_t offset);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ adc_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/