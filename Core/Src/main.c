/* USER CODE BEGIN Header */
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ceva_bno085.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BOTTOM_CAN_TX_PERIOD_MS 10U
#define BOTTOM_CAN_ID_RPY      0x210U
#define BOTTOM_CAN_ID_ACCEL    0x211U
#define BOTTOM_CAN_ID_COMMAND  0x212U
#define BOTTOM_CAN_ID_STATUS   0x213U
#define BOTTOM_CAN_CMD_RELAY   0x01U
#define BOTTOM_RELAY_ON_LEVEL  GPIO_PIN_RESET
#define BOTTOM_RELAY_OFF_LEVEL GPIO_PIN_SET

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;
FDCAN_HandleTypeDef hfdcan1;

SPI_HandleTypeDef hspi2;

/* USER CODE BEGIN PV */
static uint32_t bottom_can_last_tx_ms;
static uint8_t bottom_can_tx_seq;
static uint8_t bottom_can_relay_state;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */
static void Bottom_CAN_Init(void);
static void Bottom_CAN_Service(void);
static void Bottom_CAN_ProcessRx(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_FDCAN1_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
  Bottom_CAN_Init();
  CEVA_BNO085_Init();

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Bottom_CAN_Service();
    CEVA_BNO085_Service();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
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
  * @brief FDCAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN1_Init(void)
{

  /* USER CODE BEGIN FDCAN1_Init 0 */

  /* USER CODE END FDCAN1_Init 0 */

  /* USER CODE BEGIN FDCAN1_Init 1 */

  /* USER CODE END FDCAN1_Init 1 */
  hfdcan1.Instance = FDCAN1;
  hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan1.Init.AutoRetransmission = ENABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 10;
  hfdcan1.Init.NominalSyncJumpWidth = 3;
  hfdcan1.Init.NominalTimeSeg1 = 13;
  hfdcan1.Init.NominalTimeSeg2 = 3;
  hfdcan1.Init.DataPrescaler = 1;
  hfdcan1.Init.DataSyncJumpWidth = 1;
  hfdcan1.Init.DataTimeSeg1 = 1;
  hfdcan1.Init.DataTimeSeg2 = 1;
  hfdcan1.Init.StdFiltersNbr = 1;
  hfdcan1.Init.ExtFiltersNbr = 0;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN1_Init 2 */

  /* USER CODE END FDCAN1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BRAKE_GPIO_Port, BRAKE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SPI2_RST_Pin|SPI2_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : BRAKE_Pin */
  GPIO_InitStruct.Pin = BRAKE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BRAKE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI2_RST_Pin SPI2_CS_Pin */
  GPIO_InitStruct.Pin = SPI2_RST_Pin|SPI2_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_INT_Pin */
  GPIO_InitStruct.Pin = SPI2_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SPI2_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LIMIT_SW1_1_Pin LIMIT_SW1_2_Pin LIMIT_SW2_1_Pin LIMIT_SW2_2_Pin */
  GPIO_InitStruct.Pin = LIMIT_SW1_1_Pin|LIMIT_SW1_2_Pin|LIMIT_SW2_1_Pin|LIMIT_SW2_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static int16_t clamp_i16(int32_t value)
{
  if (value > 32767) {
    return 32767;
  }
  if (value < -32768) {
    return -32768;
  }

  return (int16_t)value;
}

static int16_t scale_i16(float value, float scale)
{
  float scaled = value * scale;

  if (scaled >= 0.0f) {
    scaled += 0.5f;
  } else {
    scaled -= 0.5f;
  }

  return clamp_i16((int32_t)scaled);
}

static void put_i16_be(uint8_t *buffer, uint32_t offset, int16_t value)
{
  uint16_t raw = (uint16_t)value;

  buffer[offset] = (uint8_t)(raw >> 8);
  buffer[offset + 1U] = (uint8_t)(raw & 0xFFU);
}

static uint8_t read_limit_bits(void)
{
  uint8_t bits = 0U;

  bits |= (HAL_GPIO_ReadPin(LIMIT_SW1_1_GPIO_Port, LIMIT_SW1_1_Pin) == GPIO_PIN_RESET) ? 0x01U : 0U;
  bits |= (HAL_GPIO_ReadPin(LIMIT_SW1_2_GPIO_Port, LIMIT_SW1_2_Pin) == GPIO_PIN_RESET) ? 0x02U : 0U;
  bits |= (HAL_GPIO_ReadPin(LIMIT_SW2_1_GPIO_Port, LIMIT_SW2_1_Pin) == GPIO_PIN_RESET) ? 0x04U : 0U;
  bits |= (HAL_GPIO_ReadPin(LIMIT_SW2_2_GPIO_Port, LIMIT_SW2_2_Pin) == GPIO_PIN_RESET) ? 0x08U : 0U;

  return bits;
}

static void set_relay_state(uint8_t state)
{
  bottom_can_relay_state = (state != 0U) ? 1U : 0U;
  HAL_GPIO_WritePin(BRAKE_GPIO_Port, BRAKE_Pin,
                    (bottom_can_relay_state != 0U) ? BOTTOM_RELAY_ON_LEVEL : BOTTOM_RELAY_OFF_LEVEL);
}

static HAL_StatusTypeDef bottom_can_send(uint32_t std_id, const uint8_t data[8])
{
  FDCAN_TxHeaderTypeDef tx_header = {0};

  if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0U) {
    return HAL_BUSY;
  }

  tx_header.Identifier = std_id;
  tx_header.IdType = FDCAN_STANDARD_ID;
  tx_header.TxFrameType = FDCAN_DATA_FRAME;
  tx_header.DataLength = FDCAN_DLC_BYTES_8;
  tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  tx_header.BitRateSwitch = FDCAN_BRS_OFF;
  tx_header.FDFormat = FDCAN_CLASSIC_CAN;
  tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  tx_header.MessageMarker = 0U;

  return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, (uint8_t *)data);
}

static void Bottom_CAN_Init(void)
{
  FDCAN_FilterTypeDef rx_filter = {0};

  rx_filter.IdType = FDCAN_STANDARD_ID;
  rx_filter.FilterIndex = 0U;
  rx_filter.FilterType = FDCAN_FILTER_MASK;
  rx_filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  rx_filter.FilterID1 = BOTTOM_CAN_ID_COMMAND;
  rx_filter.FilterID2 = 0x7FFU;

  if (HAL_FDCAN_ConfigFilter(&hfdcan1, &rx_filter) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT,
                                   FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK) {
    Error_Handler();
  }

  set_relay_state(0U);

  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
    Error_Handler();
  }

  bottom_can_last_tx_ms = HAL_GetTick();
}

static void Bottom_CAN_HandleCommand(const FDCAN_RxHeaderTypeDef *rx_header, const uint8_t data[8])
{
  if ((rx_header->IdType != FDCAN_STANDARD_ID) ||
      (rx_header->Identifier != BOTTOM_CAN_ID_COMMAND) ||
      (rx_header->RxFrameType != FDCAN_DATA_FRAME) ||
      (rx_header->DataLength != FDCAN_DLC_BYTES_8)) {
    return;
  }

  if (data[0] == BOTTOM_CAN_CMD_RELAY) {
    set_relay_state(data[1]);
  }
}

static void Bottom_CAN_ProcessRx(void)
{
  FDCAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[8];

  while (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0U) {
    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rx_header, rx_data) != HAL_OK) {
      return;
    }

    Bottom_CAN_HandleCommand(&rx_header, rx_data);
  }
}

static void Bottom_CAN_Service(void)
{
  uint32_t now_ms = HAL_GetTick();
  uint8_t limits;
  uint8_t rpy_data[8] = {0};
  uint8_t accel_data[8] = {0};
  uint8_t status_data[8] = {0};

  Bottom_CAN_ProcessRx();

  if ((now_ms - bottom_can_last_tx_ms) < BOTTOM_CAN_TX_PERIOD_MS) {
    return;
  }
  bottom_can_last_tx_ms = now_ms;

  limits = read_limit_bits();

  if (bno085_dbg.rpy_count > 0U) {
    put_i16_be(rpy_data, 0U, scale_i16(bno085_dbg.roll_deg, 100.0f));
    put_i16_be(rpy_data, 2U, scale_i16(bno085_dbg.pitch_deg, 100.0f));
    put_i16_be(rpy_data, 4U, scale_i16(bno085_dbg.yaw_deg, 100.0f));
    rpy_data[6] = limits;
    rpy_data[7] = bottom_can_tx_seq;
    (void)bottom_can_send(BOTTOM_CAN_ID_RPY, rpy_data);
  }

  if (bno085_dbg.accel_count > 0U) {
    put_i16_be(accel_data, 0U, scale_i16(bno085_dbg.accel_x, 1000.0f));
    put_i16_be(accel_data, 2U, scale_i16(bno085_dbg.accel_y, 1000.0f));
    put_i16_be(accel_data, 4U, scale_i16(bno085_dbg.accel_z, 1000.0f));
    accel_data[6] = limits;
    accel_data[7] = bno085_dbg.report_id;
    (void)bottom_can_send(BOTTOM_CAN_ID_ACCEL, accel_data);
  }

  status_data[0] = limits;
  status_data[1] = bottom_can_relay_state;
  status_data[2] = bno085_dbg.rx_ready;
  status_data[7] = bottom_can_tx_seq;
  (void)bottom_can_send(BOTTOM_CAN_ID_STATUS, status_data);

  bottom_can_tx_seq++;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  CEVA_BNO085_EXTI_Callback(GPIO_Pin);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  CEVA_BNO085_SPI_TxRxCpltCallback(hspi);
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  CEVA_BNO085_SPI_ErrorCallback(hspi);
}

/* USER CODE END 4 */

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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
