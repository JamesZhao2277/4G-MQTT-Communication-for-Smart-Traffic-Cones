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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "uart_frame.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* PC 通过 MQTT/DTU 下发的五参数电机控制命令。 */
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
/* USART3 单次最大接收长度，超过该长度会触发一次接收完成回调 */
#define USART3_RX_BUFFER_SIZE 256U

/* USART3 接收缓冲区块数量，其中一块用于当前正在接收 */
#define USART3_RX_BUFFER_COUNT 4U

/* 已接收数据队列长度，保留一块缓冲区给下一次接收使用 */
#define USART3_RX_QUEUE_SIZE (USART3_RX_BUFFER_COUNT - 1U)

/* PC 命令格式: left_pwm,right_pwm,pulses,angle_left,angle_right\n */
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

/* USER CODE BEGIN PV */
/* 多缓冲区接收 USART3 数据，避免回调还没处理完就覆盖数据 */
static uint8_t usart3_rx_buffers[USART3_RX_BUFFER_COUNT][USART3_RX_BUFFER_SIZE];

/* 当前正在给 HAL 接收函数使用的缓冲区下标 */
static volatile uint8_t usart3_rx_active_index = 0U;

/* 已接收完成的数据队列，队列里保存的是缓冲区下标 */
static volatile uint8_t usart3_rx_queue[USART3_RX_QUEUE_SIZE];

/* 每个队列元素对应的有效数据长度 */
static volatile uint16_t usart3_rx_sizes[USART3_RX_QUEUE_SIZE];

/* 队列头尾和当前队列元素数量，由中断回调和主循环共同访问 */
static volatile uint8_t usart3_rx_queue_head = 0U;
static volatile uint8_t usart3_rx_queue_tail = 0U;
static volatile uint8_t usart3_rx_queue_count = 0U;

static volatile uint8_t usart3_rx_overflow = 0U;

/* USART3 是连续字节流，先缓存到换行符，再作为一条完整命令处理。 */
static char pc_cmd_line_buffer[PC_CMD_LINE_BUFFER_SIZE];
static uint16_t pc_cmd_line_len = 0U;
static uint8_t agv_tx_frame_buffer[UART_FRAME_CALC_TOTAL_LEN(sizeof(ctrl_frame_t))];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_USART3_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* 开启 USART3 空闲中断接收，收到一段数据后进入 HAL_UARTEx_RxEventCallback */
  USART3_StartReceiveToIdle();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 主循环中把 USART3 收到的数据通过 USART1 打印到 PC */
    /* USART3 收到 PC/DTU 命令后，回显到 USART1 并尝试转成 USART2 AGV 控制帧。 */
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* 启动 USART3 接收，接收满缓冲区或检测到总线空闲都会触发回调 */
static void USART3_StartReceiveToIdle(void)
{
  (void)HAL_UARTEx_ReceiveToIdle_IT(&huart3,
                                    usart3_rx_buffers[usart3_rx_active_index],
                                    USART3_RX_BUFFER_SIZE);
}

/* 从接收队列取出一段 USART3 数据，并通过 USART1 原样发出 */
/* 取出一段 USART3 数据：先给 USART1 调试显示，再喂给命令解析器。 */
static void USART3_ProcessReceivedData(void)
{
  uint8_t buffer_index = 0U;
  uint16_t size = 0U;
  uint8_t has_data = 0U;
  uint8_t has_overflow = 0U;
  uint32_t primask;
  static const uint8_t overflow_msg[] = "\r\n[USART3 RX overflow]\r\n";

  /* 关闭中断，防止主循环读队列时 USART3 回调同时修改队列 */
  primask = __get_PRIMASK();
  __disable_irq();

  /* 如果之前发生过队列溢出，先记录下来，稍后通过 USART1 提示 */
  if (usart3_rx_overflow != 0U)
  {
    usart3_rx_overflow = 0U;
    has_overflow = 1U;
  }

  /* 从队列头取出一块已经接收完成的数据 */
  if (usart3_rx_queue_count > 0U)
  {
    buffer_index = usart3_rx_queue[usart3_rx_queue_head];
    size = usart3_rx_sizes[usart3_rx_queue_head];
    usart3_rx_queue_head = (uint8_t)((usart3_rx_queue_head + 1U) % USART3_RX_QUEUE_SIZE);
    usart3_rx_queue_count--;
    has_data = 1U;
  }

  /* 恢复进入本函数前的中断状态 */
  if (primask == 0U)
  {
    __enable_irq();
  }

  /* 打印接收溢出提示，表示 USART3 来得太快或 USART1 输出太慢 */
  if (has_overflow != 0U)
  {
    (void)HAL_UART_Transmit(&huart1,
                            (uint8_t *)overflow_msg,
                            (uint16_t)(sizeof(overflow_msg) - 1U),
                            HAL_MAX_DELAY);
  }

  /* 把 USART3 收到的数据原样转发到 USART1 */
  if (has_data != 0U)
  {
    (void)HAL_UART_Transmit(&huart1,
                            usart3_rx_buffers[buffer_index],
                            size,
                            HAL_MAX_DELAY);
    PC_CommandFeedBytes(usart3_rx_buffers[buffer_index], size);
  }
}

/* 按换行符把 DTU 转发来的字节流拼成一条 PC 控制命令。 */
static void PC_CommandFeedBytes(const uint8_t *data, uint16_t size)
{
  uint16_t i;

  for (i = 0U; i < size; i++)
  {
    char ch = (char)data[i];

    if (ch == '\r')
    {
      continue;
    }

    if (ch == '\n')
    {
      /* 收到换行才执行命令，避免半包被误解析。 */
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
      /* 命令异常过长时丢弃当前半包，等待下一条换行重新同步。 */
      pc_cmd_line_len = 0U;
      Debug_WriteString("\r\n[PC CMD overflow]\r\n");
    }
  }
}

static void PC_CommandHandleLine(const char *line)
{
  pc_motor_command_t cmd;

  /* 解析和范围检查都通过后，才允许发送到底盘。 */
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

  /* 严格匹配五个字段，字段外多余字符也视为非法命令。 */
  if ((PC_ParseIntField(&cursor, &left_pwm) == 0U) || (PC_ParseComma(&cursor) == 0U) ||
      (PC_ParseIntField(&cursor, &right_pwm) == 0U) || (PC_ParseComma(&cursor) == 0U) ||
      (PC_ParseIntField(&cursor, &pulses) == 0U) || (PC_ParseComma(&cursor) == 0U) ||
      (PC_ParseFloatField(&cursor, &angle_left) == 0U) || (PC_ParseComma(&cursor) == 0U) ||
      (PC_ParseFloatField(&cursor, &angle_right) == 0U) || (PC_ParseEnd(&cursor) == 0U))
  {
    return 0U;
  }

  /* 范围与 pc_auto_sender.py 保持一致，防止异常值直接驱动电机。 */
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

/* 解析一个带可选正负号的整数字段，并把 cursor 推进到字段末尾。 */
static uint8_t PC_ParseIntField(const char **cursor, int32_t *out_value)
{
  int32_t sign = 1;
  int32_t value = 0;
  uint8_t has_digit = 0U;

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

/* 解析一个简单十进制浮点字段，避免在单片机侧引入 sscanf 浮点格式化开销。 */
static uint8_t PC_ParseFloatField(const char **cursor, float_t *out_value)
{
  int32_t sign = 1;
  uint32_t whole = 0U;
  uint32_t fraction = 0U;
  uint32_t scale = 1U;
  uint8_t has_digit = 0U;
  float_t value;

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
        /* 限制小数位，避免 fraction/scale 在长输入下持续增长。 */
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

/* 字段之间必须用逗号分隔，允许逗号两侧有空格。 */
static uint8_t PC_ParseComma(const char **cursor)
{
  PC_SkipSpaces(cursor);
  if (**cursor != ',')
  {
    return 0U;
  }
  (*cursor)++;
  PC_SkipSpaces(cursor);
  return 1U;
}

/* 最后一个字段后只能有空格或字符串结束。 */
static uint8_t PC_ParseEnd(const char **cursor)
{
  PC_SkipSpaces(cursor);
  return (**cursor == '\0') ? 1U : 0U;
}

/* 兼容 PC 端手动输入时可能带的空格和 TAB。 */
static void PC_SkipSpaces(const char **cursor)
{
  while ((**cursor == ' ') || (**cursor == '\t'))
  {
    (*cursor)++;
  }
}

/* 把 PC 命令转换为 AGV 控制帧，并通过 USART2 单向发给底盘。 */
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

  /* 当前只实现单向电机控制：PC 五参数映射到 AGV 控制帧。 */
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

  /* 使用 SDK 帧协议添加帧头、长度、类型、CRC 和帧尾。 */
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

/* USART1 调试输出，避免在主逻辑里重复计算字符串长度。 */
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
      /* 队列未满时，把当前缓冲区加入待打印队列 */
      if (usart3_rx_queue_count < USART3_RX_QUEUE_SIZE)
      {
        usart3_rx_queue[usart3_rx_queue_tail] = usart3_rx_active_index;
        usart3_rx_sizes[usart3_rx_queue_tail] = Size;
        usart3_rx_queue_tail = (uint8_t)((usart3_rx_queue_tail + 1U) % USART3_RX_QUEUE_SIZE);
        usart3_rx_queue_count++;

        /* 切换到下一块缓冲区，用于后续 USART3 接收 */
        usart3_rx_active_index++;
        if (usart3_rx_active_index >= USART3_RX_BUFFER_COUNT)
        {
          usart3_rx_active_index = 0U;
        }
      }
      else
      {
        /* 队列已满，本次数据无法保存，交给主循环打印溢出提示 */
        usart3_rx_overflow = 1U;
      }
    }

    /* 重新开启 USART3 接收，继续等待下一段数据 */
    USART3_StartReceiveToIdle();
  }
}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

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
