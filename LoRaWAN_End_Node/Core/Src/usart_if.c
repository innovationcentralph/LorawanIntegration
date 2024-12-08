/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart_if.c
  * @author  MCD Application Team
  * @brief   Configuration of UART driver interface for hyperterminal communication
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
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
#include "usart_if.h"

/* USER CODE BEGIN Includes */
#include "sys_app.h"
#include "stdbool.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/**
  * @brief DMA handle
  */
extern DMA_HandleTypeDef hdma_usart2_tx;

/**
  * @brief UART handle
  */
extern UART_HandleTypeDef huart2;

/**
  * @brief buffer to receive 1 character
  */
uint8_t charRx;

/* USER CODE BEGIN EV */
#define RX_BUFFER_SIZE 100
char rxBuffer[RX_BUFFER_SIZE];
uint8_t rxIndex = 0;




/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief Trace driver callbacks handler
  */
const UTIL_ADV_TRACE_Driver_s UTIL_TraceDriver =
{
  vcom_Init,
  vcom_DeInit,
  vcom_ReceiveInit,
  vcom_Trace_DMA,
};

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// Structure to hold parsed data

RollingQueue commandQueue = { .head = 0, .tail = 0, .count = 0 };

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/**
  * @brief  TX complete callback
  * @return none
  */
static void (*TxCpltCallback)(void *);
/**
  * @brief  RX complete callback
  * @param  rxChar ptr of chars buffer sent by user
  * @param  size buffer size
  * @param  error errorcode
  * @return none
  */
static void (*RxCpltCallback)(uint8_t *rxChar, uint16_t size, uint8_t error);

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */
// Function to validate and parse the command
// Function to validate and parse the command


// Function to add an element to the queue (enqueue)
void enqueue(CommandData data) {
    commandQueue.buffer[commandQueue.head] = data;  // Store data in the queue

    // Move the head to the next position
    commandQueue.head = (commandQueue.head + 1) % QUEUE_SIZE;

    // If the queue is full, move the tail to overwrite the oldest element
    if (commandQueue.count == QUEUE_SIZE) {
        commandQueue.tail = (commandQueue.tail + 1) % QUEUE_SIZE;
    } else {
        commandQueue.count++;  // Increment the count until the queue is full
    }
}

// Optional function to dequeue (read and remove the oldest element from the queue)
CommandData dequeue() {
    CommandData data = commandQueue.buffer[commandQueue.tail];  // Get the oldest element
    commandQueue.tail = (commandQueue.tail + 1) % QUEUE_SIZE;   // Move the tail to the next element
    commandQueue.count--;  // Decrement the count
    return data;
}

// Function to print the current queue for debugging
void printQueue() {
	APP_LOG(TS_OFF, VLEVEL_M, "Queue contents (latest 5):\n");

    for (uint8_t i = 0; i < commandQueue.count; i++) {
        uint8_t index = (commandQueue.tail + i) % QUEUE_SIZE;

        APP_LOG(TS_OFF, VLEVEL_M, "Water Level: %u, Battery Voltage: %u, System State: %u\n",
               commandQueue.buffer[index].waterLevel,
               commandQueue.buffer[index].batteryVoltage,
               commandQueue.buffer[index].systemState);


    }
    APP_LOG(TS_OFF, VLEVEL_M, "\n");
}

bool parseCommand(const char *input, CommandData *data) {
    // Check if the command starts with "MSG" and contains '<' and '>'
    const char *start = strchr(input, '<');
    const char *end = strchr(input, '>');

    if (start == NULL || end == NULL || start >= end) {
        // Invalid command if no < or > or if < appears after >
        return false;
    }

    // Extract the content inside '<' and '>'
    char buffer[RX_BUFFER_SIZE];
    strncpy(buffer, start + 1, end - start - 1);  // Copy content inside '<' and '>'
    buffer[end - start - 1] = '\0';               // Null-terminate the string

    // Temporary variables to hold the parsed float values
    float waterLevel, batteryVoltage;
    int systemState;

    // Parse the water level, battery voltage, and system state
    if (sscanf(buffer, "%f,%f,%d", &waterLevel, &batteryVoltage, &systemState) != 3) {
        // Parsing failed
        return false;
    }

    // Store values, multiplying floats by 100 to store them as integers
    data->waterLevel = (uint16_t)(waterLevel * 100);
    data->batteryVoltage = (uint16_t)(batteryVoltage * 100);
    data->systemState = (uint8_t)systemState;

    enqueue(*data);
    printQueue();

    // Command is valid and parsed successfully
    return true;
}


/* USER CODE END PFP */

/* Exported functions --------------------------------------------------------*/

UTIL_ADV_TRACE_Status_t vcom_Init(void (*cb)(void *))
{
  /* USER CODE BEGIN vcom_Init_1 */

  /* USER CODE END vcom_Init_1 */
  TxCpltCallback = cb;
  MX_DMA_Init();
  MX_USART2_UART_Init();
  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_27);
  return UTIL_ADV_TRACE_OK;
  /* USER CODE BEGIN vcom_Init_2 */

  /* USER CODE END vcom_Init_2 */
}

UTIL_ADV_TRACE_Status_t vcom_DeInit(void)
{
  /* USER CODE BEGIN vcom_DeInit_1 */

  /* USER CODE END vcom_DeInit_1 */
  /* ##-1- Reset peripherals ################################################## */
  __HAL_RCC_USART2_FORCE_RESET();
  __HAL_RCC_USART2_RELEASE_RESET();

  /* ##-2- MspDeInit ################################################## */
  HAL_UART_MspDeInit(&huart2);

  /* ##-3- Disable the NVIC for DMA ########################################### */
  /* USER CODE BEGIN 1 */
  HAL_NVIC_DisableIRQ(DMA1_Channel5_IRQn);

  return UTIL_ADV_TRACE_OK;
  /* USER CODE END 1 */
  /* USER CODE BEGIN vcom_DeInit_2 */

  /* USER CODE END vcom_DeInit_2 */
}

void vcom_Trace(uint8_t *p_data, uint16_t size)
{
  /* USER CODE BEGIN vcom_Trace_1 */

  /* USER CODE END vcom_Trace_1 */
  HAL_UART_Transmit(&huart2, p_data, size, 1000);
  /* USER CODE BEGIN vcom_Trace_2 */

  /* USER CODE END vcom_Trace_2 */
}

UTIL_ADV_TRACE_Status_t vcom_Trace_DMA(uint8_t *p_data, uint16_t size)
{
  /* USER CODE BEGIN vcom_Trace_DMA_1 */

  /* USER CODE END vcom_Trace_DMA_1 */
  HAL_UART_Transmit_DMA(&huart2, p_data, size);
  return UTIL_ADV_TRACE_OK;
  /* USER CODE BEGIN vcom_Trace_DMA_2 */

  /* USER CODE END vcom_Trace_DMA_2 */
}

UTIL_ADV_TRACE_Status_t vcom_ReceiveInit(void (*RxCb)(uint8_t *rxChar, uint16_t size, uint8_t error))
{
  /* USER CODE BEGIN vcom_ReceiveInit_1 */

  /* USER CODE END vcom_ReceiveInit_1 */
  UART_WakeUpTypeDef WakeUpSelection;

  /*record call back*/
  RxCpltCallback = RxCb;

  /*Set wakeUp event on start bit*/
  WakeUpSelection.WakeUpEvent = UART_WAKEUP_ON_STARTBIT;

  HAL_UARTEx_StopModeWakeUpSourceConfig(&huart2, WakeUpSelection);

  /* Make sure that no UART transfer is on-going */
  while (__HAL_UART_GET_FLAG(&huart2, USART_ISR_BUSY) == SET);

  /* Make sure that UART is ready to receive)   */
  while (__HAL_UART_GET_FLAG(&huart2, USART_ISR_REACK) == RESET);

  /* Enable USART interrupt */
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_WUF);

  /*Enable wakeup from stop mode*/
  HAL_UARTEx_EnableStopMode(&huart2);

  /*Start LPUART receive on IT*/
  HAL_UART_Receive_IT(&huart2, &charRx, 1);

  return UTIL_ADV_TRACE_OK;
  /* USER CODE BEGIN vcom_ReceiveInit_2 */

  /* USER CODE END vcom_ReceiveInit_2 */
}

void vcom_Resume(void)
{
  /* USER CODE BEGIN vcom_Resume_1 */

  /* USER CODE END vcom_Resume_1 */
  /*to re-enable lost UART settings*/
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

  /*to re-enable lost DMA settings*/
  if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN vcom_Resume_2 */

  /* USER CODE END vcom_Resume_2 */
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  /* USER CODE BEGIN HAL_UART_TxCpltCallback_1 */

  /* USER CODE END HAL_UART_TxCpltCallback_1 */
  /* buffer transmission complete*/
  if (huart->Instance == USART2)
  {
    TxCpltCallback(NULL);
  }
  /* USER CODE BEGIN HAL_UART_TxCpltCallback_2 */

  /* USER CODE END HAL_UART_TxCpltCallback_2 */
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* USER CODE BEGIN HAL_UART_RxCpltCallback_1 */
  if (huart->Instance == USART2)
  {
	if ((NULL != RxCpltCallback) && (HAL_UART_ERROR_NONE == huart->ErrorCode))
	{
	  RxCpltCallback(&charRx, 1, 0);
	}
	HAL_UART_Receive_IT(huart, &charRx, 1);
  }
  /* USER CODE END HAL_UART_RxCpltCallback_1 */
//  if (huart->Instance == USART2)
//  {
//    if ((NULL != RxCpltCallback) && (HAL_UART_ERROR_NONE == huart->ErrorCode))
//    {
//      RxCpltCallback(&charRx, 1, 0);
//    }
//    HAL_UART_Receive_IT(huart, &charRx, 1);
//  }
  /* USER CODE BEGIN HAL_UART_RxCpltCallback_2 */
  if (huart->Instance == USART1)
   {

		if (charRx != '\n')
		{
		  if (rxIndex < RX_BUFFER_SIZE - 1)  // Ensure buffer doesn't overflow
		  {
			rxBuffer[rxIndex++] = charRx;
		  }
		}
		else  // '\n' received, terminate the string and print the message
		{
		  rxBuffer[rxIndex] = '\0';  // Null-terminate the string
		  APP_LOG(TS_OFF, VLEVEL_M, "Received message: %s\r\n", rxBuffer);

		  // Check if the message is "send"
		  if (strstr(rxBuffer, "send") != NULL) {
			  // If the command is "send", dequeue and process the data
			  if (commandQueue.count > 0) {
				  CommandData dequeuedData = dequeue();
				  APP_LOG(TS_OFF, VLEVEL_M, "Dequeued Data:\n");
				  APP_LOG(TS_OFF, VLEVEL_M, "Water Level (x100): %d\n", dequeuedData.waterLevel);
				  APP_LOG(TS_OFF, VLEVEL_M, "Battery Voltage (x100): %d\n", dequeuedData.batteryVoltage);
				  APP_LOG(TS_OFF, VLEVEL_M, "System State: %u\n", dequeuedData.systemState);
			  } else {
				  APP_LOG(TS_OFF, VLEVEL_M, "Queue is empty!\n");
			  }
		  }
		  else
		  {
			  /*
			   * To TTN
			   * Water Level
			   * Battery Voltage
			   * System State
			   *
			   * MSG <123.456,234.567,9>
			   */

			  CommandData data;
			  if (parseCommand(rxBuffer, &data)) {
				  APP_LOG(TS_OFF, VLEVEL_M, "Command parsed successfully:\n");
				  APP_LOG(TS_OFF, VLEVEL_M, "Water Level (x100): %d\n", data.waterLevel);
				  APP_LOG(TS_OFF, VLEVEL_M, "Battery Voltage (x100): %d\n", data.batteryVoltage);
				  APP_LOG(TS_OFF, VLEVEL_M, "System State: %u\n", data.systemState);
			  } else {
				  APP_LOG(TS_OFF, VLEVEL_M, "Invalid command format!\n");
			  }
		  }


		  rxIndex = 0;  // Reset the index for the next message
		}

		 //APP_LOG(TS_OFF, VLEVEL_M, "%c", charRx);
		 HAL_UART_Receive_IT(huart, &charRx, 1);
	   }

  /* USER CODE END HAL_UART_RxCpltCallback_2 */
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private Functions Definition -----------------------------------------------*/

/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */
