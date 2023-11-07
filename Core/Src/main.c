/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "message_buffer.h"
#include <stdio.h>
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
UART_HandleTypeDef hlpuart1;

osThreadId defaultTaskHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */
SemaphoreHandle_t 		mutex_lpuart;
SemaphoreHandle_t 		sem_lpuart;
MessageBufferHandle_t 	print_messages_buffer;
TimerHandle_t 			blinky_tm;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void serial_task1(void *param){
	uint32_t cnt = 0;
	while(1){
		if (xSemaphoreTake(mutex_lpuart, portMAX_DELAY) == pdTRUE){
			//HAL_UART_Transmit_IT(&hlpuart1, (uint8_t *)"Teste de serial 1!\n\r", 20);
			//xSemaphoreTake(sem_lpuart, portMAX_DELAY);
			if (xMessageBufferSend(print_messages_buffer, "Teste de serial 1!\n\r", 20, portMAX_DELAY) == 20){
				cnt++;
			}
			xSemaphoreGive(mutex_lpuart);
			//cnt++;
		}
		portYIELD();
	}
}

volatile size_t space = 0;
static void serial_task2(void *param){
	uint32_t cnt = 0;
	while(1){
		if (xSemaphoreTake(mutex_lpuart, portMAX_DELAY) == pdTRUE){
			//HAL_UART_Transmit_IT(&hlpuart1, (uint8_t *)"Teste de serial 2!\n\r", 20);
			//xSemaphoreTake(sem_lpuart, portMAX_DELAY);
			if (xMessageBufferSend(print_messages_buffer, "Teste de serial 2!\n\r", 20, portMAX_DELAY) == 20) {
				cnt++;
			}
			xSemaphoreGive(mutex_lpuart);
			space = xMessageBufferSpaceAvailable(print_messages_buffer);
			//cnt++;
		}
		portYIELD();
	}
}

static void sobra_cpu(void *param){
	uint32_t cnt = 0;
	while(1){
		cnt++;
		if (cnt == 100000000) {
			xTimerChangePeriod(blinky_tm, 1000, 0);
		}
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	portBASE_TYPE pxHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(sem_lpuart, &pxHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

static void print_task(void *param){
	char mensagem[128];
	while(1){
		size_t len = xMessageBufferReceive(print_messages_buffer, mensagem, 128, portMAX_DELAY);
		if (len) {
			HAL_UART_Transmit_IT(&hlpuart1, (uint8_t *)mensagem, len);
			xSemaphoreTake(sem_lpuart, portMAX_DELAY);
		}
	}
}

void led_blinky_cb(TimerHandle_t xTimer)
{
  (void) xTimer;
  HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);

}

#define EVENTO_0 ( 1 << 0 )
#define EVENTO_1 ( 1 << 1 )

/* Assume-se que o grupo de eventos foi criado anteriormente */
EventGroupHandle_t 	exemplo_grupo_eventos;
EventBits_t 		exemplo_bits_eventos;


static void produtor1(void *param){
	while(1){
		 vTaskDelay(33);
		 exemplo_bits_eventos = xEventGroupSetBits(exemplo_grupo_eventos, EVENTO_0);
	}
}

static void produtor2(void *param){
	while(1){
		vTaskDelay(75);
		exemplo_bits_eventos = xEventGroupSetBits(exemplo_grupo_eventos, EVENTO_1);
	}
}

static void consumidor(void *param){

	while(1){
		exemplo_bits_eventos = xEventGroupWaitBits(
				exemplo_grupo_eventos, 								/* Grupo de eventos sendo testado */
				EVENTO_0 | EVENTO_1, 								/* Bits do grupo de eventos para esperar */
				pdTRUE,												/* Os bits devem ser definidos como 0 antes de retornar. */
				pdTRUE,												/* Espera por ambos os bits */
				100 );												/* Espera por no máximo 100 ms. */

		TickType_t ct = xTaskGetTickCount();
		if (xSemaphoreTake(mutex_lpuart, portMAX_DELAY) == pdTRUE){
			char message[64];
			int len = sprintf(message, "Sincronizacao no tempo: %ld\n", ct);
			(void)xMessageBufferSend(print_messages_buffer, message, len, portMAX_DELAY);
			xSemaphoreGive(mutex_lpuart);
		}
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
  //board_init();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  mutex_lpuart = xSemaphoreCreateMutex();
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  sem_lpuart = xSemaphoreCreateBinary();
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  blinky_tm = xTimerCreate("Blink timer", pdMS_TO_TICKS(500), pdTRUE, NULL, led_blinky_cb);
  xTimerStart(blinky_tm, 0);
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  print_messages_buffer = xMessageBufferCreate(1024);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  xTaskCreate(serial_task1, "Task serial 1", 256, NULL, 5, NULL);
  xTaskCreate(serial_task2, "Task serial 2", 256, NULL, 5, NULL);
  xTaskCreate(print_task, "Print Task", 256, NULL, 10, NULL);
  xTaskCreate(sobra_cpu, "Sobra CPU", 256, NULL, 1, NULL);


  xTaskCreate(produtor1, "Tarefa produtora 1", 256, NULL, 7, NULL);
  xTaskCreate(produtor2, "Tarefa produtora 2", 256, NULL, 8, NULL);
  xTaskCreate(consumidor, "Tarefa consumidora", 256, NULL, 8, NULL);

  /* Tentativa de criar o grupo */
  exemplo_grupo_eventos = xEventGroupCreate();

  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV6;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_LED_Pin */
  GPIO_InitStruct.Pin = USER_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USER_LED_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (HAL_GetTick() >= 1000){
	  HAL_IncTick();
  }
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
