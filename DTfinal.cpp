/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Pressure Color Mixing Game
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For rand()
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Pressure Thresholds (12-bit ADC: 0-4095)
#define THRESH_LOW   1300 // ~33%
#define THRESH_HIGH  2700 // ~66%

// Game Settings
#define WIN_HOLD_TIME 10 // Number of loop cycles to hold correct color to win (debounce)

// --- PIN DEFINITIONS (Added to fix 'undeclared' errors) ---
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC

#define TGT_B_Pin GPIO_PIN_1
#define TGT_B_GPIO_Port GPIOC

#define TGT_R_Pin GPIO_PIN_1
#define TGT_R_GPIO_Port GPIOA
#define TGT_G_Pin GPIO_PIN_4
#define TGT_G_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA

#define P1_R_Pin GPIO_PIN_10
#define P1_R_GPIO_Port GPIOA
#define P1_G_Pin GPIO_PIN_3
#define P1_G_GPIO_Port GPIOB
#define P1_B_Pin GPIO_PIN_5
#define P1_B_GPIO_Port GPIOB

#define P2_R_Pin GPIO_PIN_4
#define P2_R_GPIO_Port GPIOB
#define P2_G_Pin GPIO_PIN_10
#define P2_G_GPIO_Port GPIOB
#define P2_B_Pin GPIO_PIN_8
#define P2_B_GPIO_Port GPIOA
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
char msg[100];
int round_count = 1;
int hold_counter = 0; // To prevent instant accidental wins

// Color Definitions
const Color RED     = {1, 0, 0};
const Color GREEN   = {0, 1, 0};
const Color BLUE    = {0, 0, 1};
const Color YELLOW  = {1, 1, 0};
const Color MAGENTA = {1, 0, 1};
const Color OFF     = {0, 0, 0};

// Game State
Color target_color;
Color p2_color;
Color mix_color;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
uint16_t Read_Pressure(void);
void Set_Mixing_LED(Color c);
void Set_Static_LED(GPIO_TypeDef* portR, uint16_t pinR,
                    GPIO_TypeDef* portG, uint16_t pinG,
                    GPIO_TypeDef* portB, uint16_t pinB,
                    Color c);
void Pick_New_Target(void);
int Colors_Are_Equal(Color c1, Color c2);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint16_t Read_Pressure(void) {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        return HAL_ADC_GetValue(&hadc1);
    }
    return 0;
}

// Set Mixing LED (PWM) - 255 is max brightness
void Set_Mixing_LED(Color c) {
    // Scaling 1/0 logic to 255/0 for PWM
    // Assumes Common Anode (255 - Val)
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 255 - (c.r * 255)); // Red
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 255 - (c.g * 255)); // Green
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 255 - (c.b * 255)); // Blue
}

// Set Static LED (GPIO)
void Set_Static_LED(GPIO_TypeDef* portR, uint16_t pinR,
                    GPIO_TypeDef* portG, uint16_t pinG,
                    GPIO_TypeDef* portB, uint16_t pinB,
                    Color c) {
    // Common Anode: Reset = ON
    HAL_GPIO_WritePin(portR, pinR, c.r ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(portG, pinG, c.g ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(portB, pinB, c.b ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void Pick_New_Target(void) {
    int r = rand() % 3;
    if (r == 0) target_color = RED;
    else if (r == 1) target_color = MAGENTA;
    else target_color = YELLOW;

    // Update Target LED
    Set_Static_LED(TGT_R_GPIO_Port, TGT_R_Pin, TGT_G_GPIO_Port, TGT_G_Pin, TGT_B_GPIO_Port, TGT_B_Pin, target_color);
}

int Colors_Are_Equal(Color c1, Color c2) {
    return (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
  // Start PWM
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

  // Initial Setup
  Pick_New_Target();

  // Set Player 1 to PERMANENT RED
  Set_Static_LED(P1_R_GPIO_Port, P1_R_Pin, P1_G_GPIO_Port, P1_G_Pin, P1_B_GPIO_Port, P1_B_Pin, RED);

  sprintf(msg, "\r\n=== NEW GAME STARTED: ROUND %d ===\r\n", round_count);
  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 1. Read Input
      uint16_t p = Read_Pressure();

      // 2. Determine Player 2 Color (Pressure based)
      if (p < THRESH_LOW) {
          p2_color = RED;    // 0-33%
      } else if (p < THRESH_HIGH) {
          p2_color = BLUE;   // 34-66%
      } else {
          p2_color = GREEN;  // 67-100% (High Pressure is now GREEN)
      }

      // Update Player 2 LED
      Set_Static_LED(P2_R_GPIO_Port, P2_R_Pin, P2_G_GPIO_Port, P2_G_Pin, P2_B_GPIO_Port, P2_B_Pin, p2_color);

      // 3. Determine Mixing Color (P1 is always RED)
      // Logic: Mix = P1(Red) + P2(Variable)
      if (Colors_Are_Equal(p2_color, RED)) {
          mix_color = RED;      // Red + Red = Red
      } else if (Colors_Are_Equal(p2_color, BLUE)) {
          mix_color = MAGENTA;  // Red + Blue = Magenta
      } else {
          mix_color = YELLOW;   // Red + Green = Yellow
      }

      // Update Mixing LED
      Set_Mixing_LED(mix_color);

      // 4. Win Logic
      if (Colors_Are_Equal(mix_color, target_color)) {
          hold_counter++;

          // User must hold the color for ~1 second (10 * 100ms) to confirm win
          if (hold_counter >= WIN_HOLD_TIME) {
              round_count++;
              sprintf(msg, "\r\n!!! YOU WIN !!!\r\n");
              HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);

              sprintf(msg, "Moving to Round %d...\r\n", round_count);
              HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);

              // Flash Mixing LED to celebrate
              for(int i=0; i<3; i++) {
                  Set_Mixing_LED(OFF); HAL_Delay(100);
                  Set_Mixing_LED(mix_color); HAL_Delay(100);
              }

              Pick_New_Target();
              hold_counter = 0;
          }
      } else {
          hold_counter = 0; // Reset if they slip
      }

      // 5. Logging
      if (hold_counter == 0) { // Reduce spam, only log when searching
          char *t_str = Colors_Are_Equal(target_color, RED) ? "RED" : (Colors_Are_Equal(target_color, MAGENTA) ? "MAGENTA" : "YELLOW");
          char *m_str = Colors_Are_Equal(mix_color, RED) ? "RED" : (Colors_Are_Equal(mix_color, MAGENTA) ? "MAGENTA" : "YELLOW");

          sprintf(msg, "Round: %d | ADC: %4d | P2: %s | Mix: %s | TARGET: %s\r",
                  round_count, p,
                  (Colors_Are_Equal(p2_color, RED) ? "RED" : (Colors_Are_Equal(p2_color, BLUE) ? "BLUE" : "GREEN")),
                  m_str, t_str);
          HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
      }

      HAL_Delay(100);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/**
  * @brief ADC1 Initialization Function
  */
static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  HAL_ADC_Init(&hadc1);

  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/**
  * @brief TIM3 Initialization Function
  */
static void MX_TIM3_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 254;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  HAL_TIM_PWM_Init(&htim3);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig);

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3);
}

/**
  * @brief USART2 Initialization Function
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart2);
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TGT_B_GPIO_Port, TGT_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, TGT_R_Pin|TGT_G_Pin|LD2_Pin|P2_B_Pin
                          |P1_R_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, P2_G_Pin|P1_G_Pin|P2_R_Pin|P1_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TGT_B_Pin */
  GPIO_InitStruct.Pin = TGT_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TGT_B_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TGT_R_Pin TGT_G_Pin LD2_Pin P2_B_Pin
                           P1_R_Pin */
  GPIO_InitStruct.Pin = TGT_R_Pin|TGT_G_Pin|LD2_Pin|P2_B_Pin
                          |P1_R_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : P2_G_Pin P1_G_Pin P2_R_Pin P1_B_Pin */
  GPIO_InitStruct.Pin = P2_G_Pin|P1_G_Pin|P2_R_Pin|P1_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
