
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f3xx_hal.h"
#include "adc.h"
#include "dma.h"
#include "opamp.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include <string.h>
//#include "ds18b20.h"
#include <math.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
extern bool adc_conv_halfcplt_flag;
extern bool adc_conv_cplt_flag;
#define true 1
#define false 0
#define MID_ADC_READING 2048
uint32_t time2;
uint32_t time1;
uint32_t time_diff;

// Serial output buffer
char log_buffer[250];

// Flag
bool readings_ready = false;

// Calibration
float VCAL = 268.97;
float ICAL = 90.9;

// Number of waveforms to count
uint8_t waveforms = 250;

// ISR accumulators
typedef struct channel_
{
  int64_t sum_P;
  uint64_t sum_V_sq;
  uint64_t sum_I_sq;
  int32_t sum_V;
  int32_t sum_I;
  uint32_t count;
  
  uint32_t positive_V;
  uint32_t last_positive_V;
  uint32_t cycles;
} channel_t;

#define NUMBER_OF_CHANNELS 3
static channel_t channels[NUMBER_OF_CHANNELS];
static channel_t channels_copy[NUMBER_OF_CHANNELS];

double Ws_acc[NUMBER_OF_CHANNELS] = {0}; // Watt second accumulator

uint32_t pulseCount = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

void process_frame(uint16_t offset)
{
  int32_t sample_V, sample_I, signed_V, signed_I;

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
  for (int i = 0; i < 3000; i += NUMBER_OF_CHANNELS)
  {
    // Cycle through channels
    for (int n = 0; n < NUMBER_OF_CHANNELS; n++)
    {
      channel_t *channel = &channels[n];

      // ----------------------------------------
      // Voltage
      sample_V = adc4_dma_buff[offset + i + n];
      signed_V = sample_V - MID_ADC_READING;
      channel->sum_V += signed_V;
      channel->sum_V_sq += signed_V * signed_V;
      // ----------------------------------------
      // Current
      sample_I = adc1_dma_buff[offset + i + n];
      signed_I = sample_I - MID_ADC_READING;
      channel->sum_I += signed_I;
      channel->sum_I_sq += signed_I * signed_I;
      // ----------------------------------------
      // Power
      channel->sum_P += signed_V * signed_I;
      // ----------------------------------------
      // Sample Count
      channel->count++;
      // ----------------------------------------
      // Zero crossing detection
      channel->last_positive_V = channel->positive_V;
      if (signed_V > 5)
      {
        channel->positive_V = true;
      }
      else if (signed_V < -5)
      {
        channel->positive_V = false;
      }
      
      if (!channel->last_positive_V && channel->positive_V)
      {
        channel->cycles++;
      }

      // ----------------------------------------
      // Complete Waveform Cycles to count
      if (channel->cycles == waveforms)
      {
        channel->cycles = 0;

        channel_t *channel_copy = &channels_copy[n];
        // Copy accumulators for use in main loop
        memcpy((void *)channel_copy, (void *)channel, sizeof(channel_t));
        // Reset accumulators to zero ready for next set of measurements
        memset((void *)channel, 0, sizeof(channel_t));

        if (n == NUMBER_OF_CHANNELS - 1)
        {
          readings_ready = true;
        }
      }
    }
  }
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  float V_RATIO = VCAL * (3.3 / 4096.0);
  float I_RATIO = ICAL * (3.3 / 4096.0);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_OPAMP2_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_ADC4_Init();
  MX_TIM8_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  HAL_OPAMP_Start(&hopamp2);

  start_ADCs();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (readings_ready)
    {
      readings_ready = false;
      time2 = time1;
      time1 = HAL_GetTick();
      time_diff = time1 - time2;
      for (int n = 0; n < NUMBER_OF_CHANNELS; n++)
      {
        channel_t *chn = &channels_copy[n];

        float Vmean = chn->sum_V / (float)chn->count;
        float Imean = chn->sum_I / (float)chn->count;

        chn->sum_V_sq /= (float)chn->count;
        chn->sum_V_sq -= (Vmean * Vmean); // offset subtraction

        if (chn->sum_V_sq < 0) // if offset removal cause a negative number,
          chn->sum_V_sq = 0;   // make it 0 to avoid a nan at sqrt.

        float Vrms = V_RATIO * sqrtf((float)chn->sum_V_sq);

        chn->sum_I_sq /= (float)chn->count;
        chn->sum_I_sq -= (Imean * Imean);

        if (chn->sum_I_sq < 0)
          chn->sum_I_sq = 0;

        float Irms = I_RATIO * sqrtf((float)chn->sum_I_sq);

        float mean_P = (chn->sum_P / (float)chn->count) - (Vmean * Imean);
        float realPower = V_RATIO * I_RATIO * mean_P;

        float apparentPower = Vrms * Irms;

        float powerFactor;
        if (apparentPower != 0) // prevents 'inf' at division
        {
          powerFactor = realPower / apparentPower;
        }
        else
          powerFactor = 0;

        Ws_acc[n] += ((float)(time_diff / 1000.0)) * realPower; // Watt second accumulator.
        float Wh_acc = Ws_acc[n] / 3600.0;                      // Wh_acc
        float frequency = 250.0 / (float)(time_diff / 1000.0);    // Hz

        int _n = n + 1; // for nicer serial print out
        sprintf(log_buffer, "V%d:%.2f,I%d:%.3f,RP%d:%.1f,AP%d:%.1f,PF%d:%.3f,Wh%d:%.3f,Hz%d:%.2f,C%d:%ld", _n, Vrms, _n, Irms, _n, realPower, _n, apparentPower, _n, powerFactor, _n, Wh_acc, _n, frequency, _n, chn->count);
        debug_printf(log_buffer);
      }

      debug_printf("\r\n");

      /*
      CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
      ITM->LAR = 0xC5ACCE55;
      DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
      DWT->CYCCNT = 0;
      uint32_t DWTcounter = DWT->CYCCNT;
      sprintf(log_buffer, "here:%ld\r\n", DWTcounter);
      debug_printf(log_buffer);
      */
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

  /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART3 | RCC_PERIPHCLK_TIM8;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.Tim8ClockSelection = RCC_TIM8CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

  /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */

void onPulse()
{
  pulseCount++;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
