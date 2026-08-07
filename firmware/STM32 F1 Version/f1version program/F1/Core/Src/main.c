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
#include <string.h>
#include "uart_frame.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
  int8_t left_pwm;
  int8_t right_pwm;
  uint32_t pulses;
  float_t angle_left;
  float_t angle_right;
} pc_motor_command_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define USART3_RX_BUFFER_SIZE 256U
#define USART3_RX_BUFFER_COUNT 4U
#define USART3_RX_QUEUE_SIZE (USART3_RX_BUFFER_COUNT - 1U)

#define PC_CMD_LINE_BUFFER_SIZE 96U
#define PC_CMD_PWM_MIN (-100)
#define PC_CMD_PWM_MAX 100
#define PC_CMD_PULSES_MAX 100000UL
#define PC_CMD_ANGLE_MIN (-90.0f)
#define PC_CMD_ANGLE_MAX 90.0f

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
static uint8_t usart3_rx_buffers[USART3_RX_BUFFER_COUNT][USART3_RX_BUFFER_SIZE];
static volatile uint8_t usart3_rx_active_index = 0U;
static volatile uint8_t usart3_rx_queue[USART3_RX_QUEUE_SIZE];
static volatile uint16_t usart3_rx_sizes[USART3_RX_QUEUE_SIZE];
static volatile uint8_t usart3_rx_queue_head = 0U;
static volatile uint8_t usart3_rx_queue_tail = 0U;
static volatile uint8_t usart3_rx_queue_count = 0U;
static volatile uint8_t usart3_rx_overflow = 0U;

static char pc_cmd_line_buffer[PC_CMD_LINE_BUFFER_SIZE];
static uint16_t pc_cmd_line_len = 0U;
static uint8_t agv_tx_frame_buffer[UART_FRAME_CALC_TOTAL_LEN(sizeof(ctrl_frame_t))];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */
static void USART3_StartReceiveToIdle(void);
static void USART3_ProcessReceivedData(void);
static void PC_CommandFeedBytes(const uint8_t *data, uint16_t size);
static void PC_CommandHandleLine(const char *line);
static uint8_t PC_CommandParse(const char *line, pc_motor_command_t *cmd);
static uint8_t PC_ParseIntField(const char **cursor, int32_t *out_value);
static uint8_t PC_ParseFloatField(const char **cursor, float_t *out_value);
static uint8_t PC_ParseComma(const char **cursor);
static uint8_t PC_ParseEnd(const char **cursor);
static void PC_SkipSpaces(const char **cursor);
static uint8_t AGV_SendMotorCommand(const pc_motor_command_t *cmd);
static void Debug_WriteString(const char *text);

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
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  USART3_StartReceiveToIdle();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    USART3_ProcessReceivedData();
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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 460800;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static void USART3_StartReceiveToIdle(void)
{
  (void)HAL_UARTEx_ReceiveToIdle_IT(&huart3,
                                    usart3_rx_buffers[usart3_rx_active_index],
                                    USART3_RX_BUFFER_SIZE);
}

static void USART3_ProcessReceivedData(void)
{
  uint8_t buffer_index = 0U;
  uint16_t size = 0U;
  uint8_t has_data = 0U;
  uint8_t has_overflow = 0U;
  uint32_t primask;
  static const uint8_t overflow_msg[] = "\r\n[USART3 RX overflow]\r\n";

  primask = __get_PRIMASK();
  __disable_irq();

  if (usart3_rx_overflow != 0U)
  {
    usart3_rx_overflow = 0U;
    has_overflow = 1U;
  }

  if (usart3_rx_queue_count > 0U)
  {
    buffer_index = usart3_rx_queue[usart3_rx_queue_head];
    size = usart3_rx_sizes[usart3_rx_queue_head];
    usart3_rx_queue_head = (uint8_t)((usart3_rx_queue_head + 1U) % USART3_RX_QUEUE_SIZE);
    usart3_rx_queue_count--;
    has_data = 1U;
  }

  if (primask == 0U)
  {
    __enable_irq();
  }

  if (has_overflow != 0U)
  {
    (void)HAL_UART_Transmit(&huart1,
                            (uint8_t *)overflow_msg,
                            (uint16_t)(sizeof(overflow_msg) - 1U),
                            HAL_MAX_DELAY);
  }

  if (has_data != 0U)
  {
    (void)HAL_UART_Transmit(&huart1,
                            usart3_rx_buffers[buffer_index],
                            size,
                            HAL_MAX_DELAY);
    PC_CommandFeedBytes(usart3_rx_buffers[buffer_index], size);
  }
}

static void PC_CommandFeedBytes(const uint8_t *data, uint16_t size)
{
  uint16_t i;

  if (data == NULL)
  {
    return;
  }

  for (i = 0U; i < size; i++)
  {
    char ch = (char)data[i];

    if (ch == '\r')
    {
      continue;
    }

    if (ch == '\n')
    {
      if (pc_cmd_line_len > 0U)
      {
        pc_cmd_line_buffer[pc_cmd_line_len] = '\0';
        PC_CommandHandleLine(pc_cmd_line_buffer);
        pc_cmd_line_len = 0U;
      }
      continue;
    }

    if (pc_cmd_line_len < (PC_CMD_LINE_BUFFER_SIZE - 1U))
    {
      pc_cmd_line_buffer[pc_cmd_line_len++] = ch;
    }
    else
    {
      pc_cmd_line_len = 0U;
      Debug_WriteString("\r\n[PC CMD overflow]\r\n");
    }
  }
}

static void PC_CommandHandleLine(const char *line)
{
  pc_motor_command_t cmd;

  if (PC_CommandParse(line, &cmd) == 0U)
  {
    Debug_WriteString("\r\n[PC CMD invalid]\r\n");
    return;
  }

  if (AGV_SendMotorCommand(&cmd) != 0U)
  {
    Debug_WriteString("\r\n[PC CMD -> USART2]\r\n");
  }
  else
  {
    Debug_WriteString("\r\n[PC CMD pack/send failed]\r\n");
  }
}

static uint8_t PC_CommandParse(const char *line, pc_motor_command_t *cmd)
{
  const char *cursor = line;
  int32_t left_pwm;
  int32_t right_pwm;
  int32_t pulses;
  float_t angle_left;
  float_t angle_right;

  if ((line == NULL) || (cmd == NULL))
  {
    return 0U;
  }

  if ((PC_ParseIntField(&cursor, &left_pwm) == 0U) || (PC_ParseComma(&cursor) == 0U) ||
      (PC_ParseIntField(&cursor, &right_pwm) == 0U) || (PC_ParseComma(&cursor) == 0U) ||
      (PC_ParseIntField(&cursor, &pulses) == 0U) || (PC_ParseComma(&cursor) == 0U) ||
      (PC_ParseFloatField(&cursor, &angle_left) == 0U) || (PC_ParseComma(&cursor) == 0U) ||
      (PC_ParseFloatField(&cursor, &angle_right) == 0U) || (PC_ParseEnd(&cursor) == 0U))
  {
    return 0U;
  }

  if ((left_pwm < PC_CMD_PWM_MIN) || (left_pwm > PC_CMD_PWM_MAX) ||
      (right_pwm < PC_CMD_PWM_MIN) || (right_pwm > PC_CMD_PWM_MAX) ||
      (pulses < 0) || ((uint32_t)pulses > PC_CMD_PULSES_MAX) ||
      (angle_left < PC_CMD_ANGLE_MIN) || (angle_left > PC_CMD_ANGLE_MAX) ||
      (angle_right < PC_CMD_ANGLE_MIN) || (angle_right > PC_CMD_ANGLE_MAX))
  {
    return 0U;
  }

  cmd->left_pwm = (int8_t)left_pwm;
  cmd->right_pwm = (int8_t)right_pwm;
  cmd->pulses = (uint32_t)pulses;
  cmd->angle_left = angle_left;
  cmd->angle_right = angle_right;

  return 1U;
}

static uint8_t PC_ParseIntField(const char **cursor, int32_t *out_value)
{
  int32_t sign = 1;
  int32_t value = 0;
  uint8_t has_digit = 0U;

  if ((cursor == NULL) || (*cursor == NULL) || (out_value == NULL))
  {
    return 0U;
  }

  PC_SkipSpaces(cursor);

  if (**cursor == '-')
  {
    sign = -1;
    (*cursor)++;
  }
  else if (**cursor == '+')
  {
    (*cursor)++;
  }

  while ((**cursor >= '0') && (**cursor <= '9'))
  {
    has_digit = 1U;
    value = (value * 10) + (int32_t)(**cursor - '0');
    (*cursor)++;
  }

  if (has_digit == 0U)
  {
    return 0U;
  }

  *out_value = value * sign;
  PC_SkipSpaces(cursor);
  return 1U;
}

static uint8_t PC_ParseFloatField(const char **cursor, float_t *out_value)
{
  int32_t sign = 1;
  uint32_t whole = 0U;
  uint32_t fraction = 0U;
  uint32_t scale = 1U;
  uint8_t has_digit = 0U;
  float_t value;

  if ((cursor == NULL) || (*cursor == NULL) || (out_value == NULL))
  {
    return 0U;
  }

  PC_SkipSpaces(cursor);

  if (**cursor == '-')
  {
    sign = -1;
    (*cursor)++;
  }
  else if (**cursor == '+')
  {
    (*cursor)++;
  }

  while ((**cursor >= '0') && (**cursor <= '9'))
  {
    has_digit = 1U;
    whole = (whole * 10U) + (uint32_t)(**cursor - '0');
    (*cursor)++;
  }

  if (**cursor == '.')
  {
    (*cursor)++;
    while ((**cursor >= '0') && (**cursor <= '9'))
    {
      has_digit = 1U;
      if (scale < 1000000UL)
      {
        fraction = (fraction * 10U) + (uint32_t)(**cursor - '0');
        scale *= 10U;
      }
      (*cursor)++;
    }
  }

  if (has_digit == 0U)
  {
    return 0U;
  }

  value = (float_t)whole + ((float_t)fraction / (float_t)scale);
  if (sign < 0)
  {
    value = -value;
  }

  *out_value = value;
  PC_SkipSpaces(cursor);
  return 1U;
}

static uint8_t PC_ParseComma(const char **cursor)
{
  if ((cursor == NULL) || (*cursor == NULL))
  {
    return 0U;
  }

  PC_SkipSpaces(cursor);
  if (**cursor != ',')
  {
    return 0U;
  }

  (*cursor)++;
  PC_SkipSpaces(cursor);
  return 1U;
}

static uint8_t PC_ParseEnd(const char **cursor)
{
  if ((cursor == NULL) || (*cursor == NULL))
  {
    return 0U;
  }

  PC_SkipSpaces(cursor);
  return (**cursor == '\0') ? 1U : 0U;
}

static void PC_SkipSpaces(const char **cursor)
{
  if ((cursor == NULL) || (*cursor == NULL))
  {
    return;
  }

  while ((**cursor == ' ') || (**cursor == '\t'))
  {
    (*cursor)++;
  }
}

static uint8_t AGV_SendMotorCommand(const pc_motor_command_t *cmd)
{
  ctrl_frame_t frame;
  uint16_t frame_len = 0U;
  int32_t pack_ret;

  if (cmd == NULL)
  {
    return 0U;
  }

  memset(&frame, 0, sizeof(frame));

  frame.angles.angle_left = cmd->angle_left;
  frame.angles.angle_right = cmd->angle_right;
  frame.angles.angle_camera = 0.0f;
  frame.flags.chassis_lock = 0U;
  frame.flags.motor0_break = 0U;
  frame.flags.motor1_break = 0U;
  frame.flags.diff_locked = 0U;
  frame.motor_rpm.left = cmd->left_pwm;
  frame.motor_rpm.right = cmd->right_pwm;
  frame.motor_rpm.reserved = 0U;
  frame.motor_rpm.pulses = cmd->pulses;

  pack_ret = uart_frame_pack(UART_FRAME_TYPE_CONTROL_CMD,
                             &frame,
                             (uint16_t)sizeof(frame),
                             agv_tx_frame_buffer,
                             sizeof(agv_tx_frame_buffer),
                             &frame_len);
  if (pack_ret != UART_FRAME_OK)
  {
    return 0U;
  }

  if (HAL_UART_Transmit(&huart2, agv_tx_frame_buffer, frame_len, HAL_MAX_DELAY) != HAL_OK)
  {
    return 0U;
  }

  return 1U;
}

static void Debug_WriteString(const char *text)
{
  const char *end = text;

  if (text == NULL)
  {
    return;
  }

  while (*end != '\0')
  {
    end++;
  }

  (void)HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)(end - text), HAL_MAX_DELAY);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART3)
  {
    if (Size > 0U)
    {
      if (usart3_rx_queue_count < USART3_RX_QUEUE_SIZE)
      {
        usart3_rx_queue[usart3_rx_queue_tail] = usart3_rx_active_index;
        usart3_rx_sizes[usart3_rx_queue_tail] = Size;
        usart3_rx_queue_tail = (uint8_t)((usart3_rx_queue_tail + 1U) % USART3_RX_QUEUE_SIZE);
        usart3_rx_queue_count++;

        usart3_rx_active_index++;
        if (usart3_rx_active_index >= USART3_RX_BUFFER_COUNT)
        {
          usart3_rx_active_index = 0U;
        }
      }
      else
      {
        usart3_rx_overflow = 1U;
      }
    }

    USART3_StartReceiveToIdle();
  }
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
