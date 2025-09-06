/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stdio.h"
#include "string.h"

#include "LoRa.h"
#include "sht2x_for_stm32_hal.h"
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
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;

osThreadId defaultTaskHandle;
osThreadId myTask03Handle;
osMessageQId myQueue01Handle;
osSemaphoreId myBinarySem01Handle;
/* USER CODE BEGIN PV */
QueueHandle_t Queue02Handle;
QueueHandle_t Queue03Handle;
QueueHandle_t Queue04Handle;
QueueHandle_t Queue05Handle;
QueueHandle_t Queue06Handle;

uint8_t Rx_data;

LoRa myLoRa;
uint8_t LoRa_status;
uint8_t RxBuffer[128];
uint8_t AddrNode = 0x09;
uint8_t AddrGate = 0x25;
uint8_t AddrNet = 0x56;
uint8_t Adv = 0x00;
uint8_t Accepted = 0x01;
uint8_t Ack = 0x02;
uint8_t Send_data = 0x03;
uint8_t RequestJoin = 0x04;
uint8_t RequestData = 0x05;
BaseType_t xHigherPriorityTaskWoken;
#define MAX_BUFFER_SIZE 128
uint8_t Joined = 0;
uint8_t Send = 0;
typedef struct
{
	uint8_t Id;
	float Temp;
}infor_t;

union FloatToBytes {
	float value;
	uint8_t bytes[4];
};
int threshold = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void const * argument);
void StartTask03(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return ch;
}
// Chuyển đổi int thành 2 byte (uint8_t)
void int_to_2uint8(int value, uint8_t *high_byte, uint8_t *low_byte) {
    *low_byte = value & 0xFF;          // Byte thấp
    *high_byte = (value >> 8) & 0xFF;  // Byte cao
}

// Chuyển đổi 2 byte (uint8_t) thành int
int uint8_to_int(uint8_t high_byte, uint8_t low_byte) {
    int result = (high_byte << 8) | low_byte; // Kết hợp hai byte
    if (high_byte & 0x80) { // Kiểm tra bit dấu (nếu high_byte >= 128)
        result -= 0x10000;  // �?i�?u chỉnh cho số âm
    }
    return result;
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
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  myLoRa = newLoRa();
      	myLoRa.CS_port 		= NSS_GPIO_Port;
      	myLoRa.CS_pin 		= NSS_Pin;
      	myLoRa.reset_port 	= RST_GPIO_Port;
      	myLoRa.reset_pin 	= RST_Pin;
      	myLoRa.DIO0_port 	= DIO0_GPIO_Port;
      	myLoRa.DIO0_pin 	= DIO0_Pin;
      	myLoRa.hSPIx 		= &hspi1;
      	myLoRa.frequency             = 433;             // default = 433 MHz
      	myLoRa.spredingFactor        = SF_9;            // default = SF_7
      	myLoRa.bandWidth             = BW_125KHz;       // default = BW_125KHz
      	myLoRa.crcRate               = CR_4_5;          // default = CR_4_5
      	myLoRa.power                 = POWER_17db;      // default = 20db
      	myLoRa.overCurrentProtection = 130;             // default = 100 mA
      	myLoRa.preamble              = 8;              // default = 8;*/

      	LoRa_setAddr(&myLoRa, AddrNode);
  	if(LoRa_init(&myLoRa) == LORA_OK)
  		LoRa_status = 1;
  	printf("Khoi tao LoRa thanh cong\n");

  	LoRa_startReceiving(&myLoRa);
  	SHT2x_Init(&hi2c1);
  	SHT2x_SetResolution(RES_14_12);
  	printf("Khoi tao sensor thanh cong\n");

  	HAL_UART_Receive_IT(&huart1, &Rx_data, 1);
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of myBinarySem01 */
  osSemaphoreDef(myBinarySem01);
  myBinarySem01Handle = osSemaphoreCreate(osSemaphore(myBinarySem01), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of myQueue01 */
  osMessageQDef(myQueue01, 9, uint8_t);
  myQueue01Handle = osMessageCreate(osMessageQ(myQueue01), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  Queue02Handle = xQueueCreate(10, sizeof(uint8_t)*10);
  Queue03Handle = xQueueCreate(10, sizeof(uint8_t) * 8);
  Queue04Handle = xQueueCreate(10, sizeof(uint8_t) * 8);
  Queue05Handle = xQueueCreate(10, sizeof(uint8_t) * 8);
  Queue06Handle = xQueueCreate(1, sizeof(uint8_t) * 4);

  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of myTask03 */
  osThreadDef(myTask03, StartTask03, osPriorityHigh, 0, 256);
  myTask03Handle = osThreadCreate(osThread(myTask03), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
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
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : NSS_Pin */
  GPIO_InitStruct.Pin = NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(NSS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RST_Pin PB5 PB8 PB9 */
  GPIO_InitStruct.Pin = RST_Pin|GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : DIO0_Pin */
  GPIO_InitStruct.Pin = DIO0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DIO0_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	xHigherPriorityTaskWoken = pdFALSE;
	uint8_t ISR_data = 0x32;
	//char output[length + 1];
	if(GPIO_Pin == DIO0_Pin)
	{
		LoRa_receive(&myLoRa, &RxBuffer, 128);
		printf("%02X %02X %02X %02X", RxBuffer[0], RxBuffer[1], RxBuffer[2],RxBuffer[3]);
		if ((RxBuffer[0] == Adv) ||
		    (RxBuffer[0] == Accepted && Joined == 0) ||
		    RxBuffer[0] == Ack || RxBuffer[0] == RequestData) {
		    if (RxBuffer[0] == Adv) {
		        printf("Nhan quang ba tu GW\n");
		        xQueueSendFromISR(Queue02Handle, &RxBuffer, &xHigherPriorityTaskWoken);
		    } else if (RxBuffer[0] == Accepted && RxBuffer[2] == AddrNode && Joined == 0) {
		        printf("GW cho phep vao mang, gui nhiet do di\n");
		        xQueueSendFromISR(Queue03Handle, &RxBuffer, &xHigherPriorityTaskWoken);
		    } else if (RxBuffer[0] == RequestData && RxBuffer[1] == AddrGate && RxBuffer[2] == AddrNode && RxBuffer[3] == AddrNet && Joined == 1)
		    {
		    	printf("GW yeu cau gui du lieu\n");
		    	xQueueSendFromISR(Queue02Handle, &RxBuffer, &xHigherPriorityTaskWoken);
		    }else if (RxBuffer[0] == Ack && RxBuffer[1] == AddrGate && RxBuffer[2] == AddrNode && Joined == 1)
		    {
		    	printf("GW da nhan duoc du lieu\n");
		    	xQueueSendFromISR(Queue05Handle, &RxBuffer, &xHigherPriorityTaskWoken);
		    }
		    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);
//		    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
		    //xQueueSendFromISR(Queue02Handle, &RxBuffer, &xHigherPriorityTaskWoken);
		}

		//osSemaphoreRelease(myBinarySem01Handle);
		//xQueueSendFromISR(Queue02Handle, (void *) &output, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
//		if(RxBuffer[1] == 0x01)
//		{
//			Addr = RxBuffer[3];
//			AddrNet = RxBuffer[2];
//			memset(RxBuffer, 0, sizeof(RxBuffer));
//		}
	}
}
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
	uint32_t Task_data = 1999;
		char temp_str[16];
		float temp ;
		float rh ;
		osEvent Recv_Task_data;
		union FloatToBytes dataConverter;
		uint8_t Test[4];
		uint8_t Remove[4];
  /* Infinite loop */
  for(;;)
  {
	  temp = SHT2x_GetTemperature(1);
	  temp = roundf(temp * 100) / 100.0;
	  dataConverter.value = temp;
	  for (int i = 0; i < 4; i++)
		  Test[i]=dataConverter.bytes[i];
	  //rh = SHT2x_GetRelativeHumidity(1);
	  //snprintf(temp_str, sizeof(temp_str), "Temp: %.4f", (double)temp);
	  if (threshold > 0 && temp > (float)threshold)
	  {
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
		  printf("Hello------------------------------\n");
	  }else{
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
	  }
	  printf("Send data from TASK\n");
	  //osMessagePut(myQueue01Handle, Task_data, 10);
//	  if (osMessagePut(myQueue01Handle, temp, 10) == osOK)
//	  {
//		  printf("Gui vao hang doi thanh cong nhiet do");
//	  }else {
//		 Recv_Task_data = osMessageGet(myQueue01Handle, 5);
//		 osMessagePut(myQueue01Handle, temp, 10);
//	  }
	  if(xQueueSend(Queue06Handle, &Test, 100) != pdTRUE)
	  {
		  xQueueReceive(Queue06Handle, &Remove, 0);
		  xQueueSend(Queue06Handle, &Test, 100);
	  }else{
		  printf("Gui vao hang doi thanh cong\n");
	  }
    osDelay(5000);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the myTask03 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(void const * argument)
{
  /* USER CODE BEGIN StartTask03 */
	uint8_t Recv_ISR_data[56];
	uint8_t BuffTx[56];
	uint8_t request_count = 0;   //�?ếm số lần gửi request
	uint8_t send_count = 0;   //�?ếm số lần gửi data
	//osEvent Recv_Task_data;
	uint8_t Recv_Task_data[4];
	TickType_t start;
	TickType_t end;
	const TickType_t delay = pdMS_TO_TICKS(23000);
	union FloatToBytes dataConverter;
	uint8_t Highthres, Lowthres;
  /* Infinite loop */
  for(;;)
  {
	  //printf("Waiting data from ISR\n");
	  //osSemaphoreWait(myBinarySem01Handle, osWaitForever);
//	  xQueueReceive(Queue02Handle, &Recv_ISR_data, osWaitForever);
//	  if(Recv_ISR_data[0] == 0x00 && Joined == 0)
//	  {
//		  BuffTx[0] = 0x04;
//		  BuffTx[1] = AddrNode;
//		  // Gui ban tin Request
//		  LoRa_transmit(&myLoRa, &BuffTx, 2, 200);
//		  printf("Da gui Request tham gia mang\n");
//	  }else if(Recv_ISR_data[0] == 0x01)
//	  {
//		  Joined == 1;
//		  printf("Da Join mang, bat dau gui du lieu");
//	  }
	  if (xQueueReceive(Queue02Handle, &Recv_ISR_data, pdMS_TO_TICKS(2000)) == pdTRUE)
	  {
		  printf("%02X %02X %02X %02X %02X\n", Recv_ISR_data[0], Recv_ISR_data[1], Recv_ISR_data[2], Recv_ISR_data[3], Recv_ISR_data[4]);
		  if ((Recv_ISR_data[0] == Adv && Joined == 0) || (Joined == 1 && (xTaskGetTickCount()-start) > delay && Recv_ISR_data[0] == Adv)) //Goi tin quang ba
		  {
			  Joined = 0;
			  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
			  printf("Hello 2 %lu\n", xTaskGetTickCount()- start);
			  printf("Da nhan duoc goi quang ba, bat dau gui Request.\n");
			  // Bat dau gui request trong vong 2 giay
			  TickType_t start_time = xTaskGetTickCount();
			  request_count = 0;
			  //&& (xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(3000)
			  while (request_count < 3 && Joined == 0)
			  {
				  BuffTx[0] = RequestJoin;
				  BuffTx[1] = AddrNode;
				  //Gui Request tham gia mang

				  LoRa_transmit(&myLoRa, &BuffTx, 2, 200);
				  printf("Da gui Request tham gia mang lan %d\n", request_count + 1);

				  request_count ++;  //Tang so lần gửi request

				  //Ch�? ngắt ISR trong 600ms hoặc đến khi hết 2 giây
				  if(xQueueReceive(Queue03Handle, &Recv_ISR_data, pdMS_TO_TICKS(1000)) == pdTRUE)
				  {
					  Joined = 1; //�?ánh dấu đã tham gia mạng
					  //start = xTaskGetTickCount();
					  AddrGate = Recv_ISR_data[1];
					  AddrNet = Recv_ISR_data[3];
					  threshold = Recv_ISR_data[4];
					  printf("Da joined mang, khong gui request nua.\n");
					  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
					  request_count = 0;
					  break;
				  }
				  else
				  {
					  printf("Khong nhan duoc thong tin goi trong thoi gian cho\n");
				  }
			  }

		  }else if (Recv_ISR_data[0] == RequestData && Joined == 1)
		  {
			  printf("GW yeu cau gui data\n");
			  // Bat dau gui data trong vong 4 giay
			  //TickType_t start_time = xTaskGetTickCount();
			  //&& (xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(3000)
			  Highthres = Recv_ISR_data[4];
			  Lowthres = Recv_ISR_data[5];
			  threshold = uint8_to_int(Highthres, Lowthres);
			  printf("Da cap nhat nguong %d\n", threshold);
			  //HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
			  send_count = 0;
			  while (send_count < 3)
			  {
				  //Recv_Task_data = osMessageGet(myQueue01Handle, pdMS_TO_TICKS(1000));
				  xQueueReceive(Queue06Handle, &Recv_Task_data, pdMS_TO_TICKS(1000));
				  //dataConverter.value = (float)Recv_Task_data.value.v;
				  for (int i = 0; i < 4; i++)
					  printf("Hello 2 %02X", Recv_Task_data[i]);
				  BuffTx[0] = Send_data;
				  BuffTx[1] = AddrNode;
				  BuffTx[2] = AddrGate;
				  BuffTx[3] = AddrNet;
				  BuffTx[4] = Recv_Task_data[0];
				  BuffTx[5] = Recv_Task_data[1];
				  BuffTx[6] = Recv_Task_data[2];
				  BuffTx[7] = Recv_Task_data[3];
				  //printf("%02X %02X", BuffTx[4], BuffTx[5]);

				  //Gui Request tham gia mang

				  LoRa_transmit(&myLoRa, &BuffTx, 8, 200);
				  printf("Da gui data lan %d\n", send_count + 1);

				  send_count ++;  //Tang so lần gửi data
				  osDelay(pdMS_TO_TICKS(1000));

				  //Ch�? ngắt ISR trong 600ms hoặc đến khi hết 2 giây
				  if(xQueueReceive(Queue05Handle, &Recv_ISR_data, pdMS_TO_TICKS(1000)) == pdTRUE)
				  {
					  printf("GW nhan du lieu thanh cong\n");
					  start = xTaskGetTickCount();
					  printf("Hello 1 %lu\n", start);
					  printf("%lu", start);
					  send_count = 0;
					  break;
				  }
				  else
				  {
					  printf("Khong nhan duoc thong tin goi trong thoi gian cho\n");
					  if (send_count == 3)
					  {
						  Joined = 0;
						  printf("Da bi out khoi mang");
						  send_count = 0;
						  break;
					  }
				  }
				  }
			  }
		  }
	  //printf("Da nhan\n");
    osDelay(100);
  }
  /* USER CODE END StartTask03 */
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
