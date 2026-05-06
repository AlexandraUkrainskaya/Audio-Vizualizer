/* USER CODE BEGIN Header */
//https://github.com/wjklimek1/ILI9341_DMA_library
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"
#include "cmsis_os.h"
#include "dma.h"
#include "i2s.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "ILI9341_DMA_Driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//TASK HANDLES
//a task to send output to the screen
TaskHandle_t xScreenTaskHandle = NULL;
//a task to communicate with the phone via bluetooth
TaskHandle_t xBTTaskHandle     = NULL;
//a task to read sensor outputs
TaskHandle_t xDataProcessHandle  = NULL;

//SAMPLE BUFFER
#define MIC_PDM_BUFFER_TOTAL 128
volatile uint16_t pdmBuffer[MIC_PDM_BUFFER_TOTAL];

//current mode of display
enum display_mode_enum {FREQUENCY_BARS};


//INTERRUPT HANDLERS

//SPI DMA interrupt
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == ILI_SPI_HANDLE.Instance) {
		ILI_DMA_Callback();
	}
}

//I2S DMA interrupt
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	/* Cast pointer to uint32_t and send as notification value */
	xTaskNotifyFromISR(xDataProcessHandle,
	                   (uint32_t)&pdmBuffer[MIC_PDM_BUFFER_TOTAL/2],        /* first half pointer */
	                   eSetValueWithOverwrite,
	                   &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		/* Cast pointer to uint32_t and send as notification value */
		xTaskNotifyFromISR(xDataProcessHandle,
		                   (uint32_t)&pdmBuffer[0],        /* first half pointer */
		                   eSetValueWithOverwrite,
		                   &xHigherPriorityTaskWoken);
		    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

//task functions
void dataProcessTask(void * pvParameters ) {
	uint32_t ulNotifiedValue;
	while(1) {
		//wait for the data
		xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotifiedValue, portMAX_DELAY);
		//process the data
		printf("data received, the address of the array is %d \n", ulNotifiedValue);
	}
}

void screenTask(void * pvParameters ) {
	uint32_t ulNotifiedValue;
	while(1) {
		//wait for the data
		xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotifiedValue, portMAX_DELAY);
		//process the data
		printf("data received, the address of the array is %d \n", ulNotifiedValue);
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S2_Init();
  MX_SPI3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  //turn on the screen
  HAL_GPIO_WritePin(Screen_BL_GPIO_Port, Screen_BL_Pin, GPIO_PIN_SET);
  //init screen
  ILI_Init();
  //fill it with the default color
  ILI_DMA_Fill(WHITE);
  //first DMA transfer
  HAL_I2S_Transmit_DMA(&hi2s2, pdmBuffer, MIC_PDM_BUFFER_TOTAL);
  //create tasks
  BaseType_t xReturned;
  /* Create the task, storing the handle. */
  xReturned = xTaskCreate(
              dataProcessTask,       /* Function that implements the task. */
              "data_proc_task",          /* Text name for the task. */
              512,      /* Stack size in words, not bytes. */
              ( void * ) NULL,    /* Parameter passed into the task. */
              3,/* Priority at which the task is created. */
              &xDataProcessHandle);      /* Used to pass out the created task's handle. */
  vTaskStartScheduler();
  xReturned = xTaskCreate(
                dataProcessTask,       /* Function that implements the task. */
                "screen_update_task",          /* Text name for the task. */
                512,      /* Stack size in words, not bytes. */
                ( void * ) NULL,    /* Parameter passed into the task. */
                3,/* Priority at which the task is created. */
                &xDataProcessHandle);      /* Used to pass out the created task's handle. */
    vTaskStartScheduler();
  /* USER CODE END 2 */

  /* Init scheduler */
  //osKernelInitialize();

  /* Call init function for freertos objects (in cmsis_os2.c) */
  //MX_FREERTOS_Init();

  /* Start scheduler */
  //osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
