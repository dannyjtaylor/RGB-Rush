/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Game + Score + 0-99 Timer + Animations (Multiplexed)
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
    char* name;
} Color;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// --- CONFIGURATION ---
// Set to 0 for Common Cathode (Common pin -> GND)
#define SEG_COMMON_ANODE 0

// Logic
#define THRESH_OFF   200
#define THRESH_LOW   1365
#define THRESH_HIGH  2730
#define WIN_HOLD_COUNT 10
#define STARTING_SCORE 5  // Lowered for quicker testing of win condition

// 7-Segment Bus (Shared)
#define SEG_A_Pin GPIO_PIN_10
#define SEG_A_Port GPIOA
#define SEG_B_Pin GPIO_PIN_3
#define SEG_B_Port GPIOB
#define SEG_C_Pin GPIO_PIN_5
#define SEG_C_Port GPIOB
#define SEG_D_Pin GPIO_PIN_4
#define SEG_D_Port GPIOB
#define SEG_E_Pin GPIO_PIN_10
#define SEG_E_Port GPIOB
#define SEG_F_Pin GPIO_PIN_8
#define SEG_F_Port GPIOA
#define SEG_G_Pin GPIO_PIN_9
#define SEG_G_Port GPIOA

// Digit Select Pins (Common Cathodes)
#define DIG1_COM_Pin GPIO_PIN_7 // Score
#define DIG1_COM_Port GPIOC
#define DIG2_COM_Pin GPIO_PIN_9 // Timer Tens
#define DIG2_COM_Port GPIOB
#define DIG3_COM_Pin GPIO_PIN_8 // Timer Ones
#define DIG3_COM_Port GPIOB

// Player 1 Pins (Port C)
#define P1_R_Pin GPIO_PIN_0
#define P1_R_Port GPIOC
#define P1_G_Pin GPIO_PIN_1
#define P1_G_Port GPIOC
#define P1_B_Pin GPIO_PIN_2
#define P1_B_Port GPIOC

// Player 2 Pins (Port B)
#define P2_R_Pin GPIO_PIN_0
#define P2_R_Port GPIOB
#define P2_G_Pin GPIO_PIN_1
#define P2_G_Port GPIOB
#define P2_B_Pin GPIO_PIN_2
#define P2_B_Port GPIOB

// Mixing LED Pins
#define MIX_R_Pin GPIO_PIN_6
#define MIX_R_Port GPIOB
#define MIX_G_Pin GPIO_PIN_6
#define MIX_G_Port GPIOA
#define MIX_B_Pin GPIO_PIN_7
#define MIX_B_Port GPIOA

// Target LED Pins (Port C)
#define TGT_R_Pin GPIO_PIN_5
#define TGT_R_Port GPIOC
#define TGT_G_Pin GPIO_PIN_6
#define TGT_G_Port GPIOC
#define TGT_B_Pin GPIO_PIN_8
#define TGT_B_Port GPIOC

// Onboard LED
#define LD2_Pin GPIO_PIN_5
#define LD2_Port GPIOA
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
TIM_HandleTypeDef htim10; // New Timer for Multiplexing
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
char msg[100];
int round_count = 1;
int score = STARTING_SCORE;
int win_timer = 0;
uint32_t last_log_time = 0;

// Game Timer Variables
int game_timer_val = 0;
uint32_t last_timer_tick = 0;

// Display Buffers (Updated by Main, Read by Interrupt)
volatile uint8_t disp_digit_1 = 0; // Score
volatile uint8_t disp_digit_2 = 0; // Timer Tens
volatile uint8_t disp_digit_3 = 0; // Timer Ones
volatile uint8_t current_digit_index = 0; // 0=Score, 1=Tens, 2=Ones

// Color Definitions
const Color OFF     = {0, 0, 0, "OFF"};
const Color BLUE    = {0, 0, 1, "BLUE"};
const Color RED     = {1, 0, 0, "RED"};
const Color GREEN   = {0, 1, 0, "GREEN"};
const Color MAGENTA = {1, 0, 1, "MAGENTA"};
const Color YELLOW  = {1, 1, 0, "YELLOW"};
const Color CYAN    = {0, 1, 1, "CYAN"};
const Color WHITE   = {1, 1, 1, "WHITE"};

Color target_color;

// 7-Segment Patterns (0-9) + Spin Patterns (10-15)
const uint8_t segment_map[16] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, // 0-4
    0x6D, 0x7D, 0x07, 0x7F, 0x6F, // 5-9
    0x01, // 10: A (Top)
    0x02, // 11: B (Top Right)
    0x04, // 12: C (Bot Right)
    0x08, // 13: D (Bottom)
    0x10, // 14: E (Bot Left)
    0x20  // 15: F (Top Left)
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void MX_TIM10_Init(void); // New Timer Init

/* USER CODE BEGIN PFP */
uint16_t Read_ADC1(void);
uint16_t Read_ADC2(void);
Color Get_Color(uint16_t val);
Color Mix_Colors(Color c1, Color c2);
void Set_LED_P1(Color c);
void Set_LED_P2(Color c);
void Set_LED_Mix(Color c);
void Set_LED_Target(Color c);
void Write_Segment_Bus(uint8_t num);
void Pick_New_Target(void);
void Game_Over_Sequence(void);
int Colors_Match(Color c1, Color c2);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

// --- INTERRUPT CALLBACK (Runs 500 times/sec) ---
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM10) {
        // 1. Turn OFF all digits (Prevent ghosting)
        // For Common Cathode, High = OFF (Disconnect from GND)
        HAL_GPIO_WritePin(DIG1_COM_Port, DIG1_COM_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(DIG2_COM_Port, DIG2_COM_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(DIG3_COM_Port, DIG3_COM_Pin, GPIO_PIN_SET);

        // 2. Select Value to Display
        uint8_t val_to_show = 0;
        if (current_digit_index == 0) val_to_show = disp_digit_1;
        else if (current_digit_index == 1) val_to_show = disp_digit_2;
        else val_to_show = disp_digit_3;

        // 3. Write Pattern to Bus
        Write_Segment_Bus(val_to_show);

        // 4. Turn ON specific digit
        // For Common Cathode, Low = ON (Connect to GND)
        if (current_digit_index == 0)      HAL_GPIO_WritePin(DIG1_COM_Port, DIG1_COM_Pin, GPIO_PIN_RESET);
        else if (current_digit_index == 1) HAL_GPIO_WritePin(DIG2_COM_Port, DIG2_COM_Pin, GPIO_PIN_RESET);
        else                               HAL_GPIO_WritePin(DIG3_COM_Port, DIG3_COM_Pin, GPIO_PIN_RESET);

        // 5. Cycle to next digit
        current_digit_index++;
        if (current_digit_index > 2) current_digit_index = 0;
    }
}

void Write_Segment_Bus(uint8_t num) {
    if(num > 15) num = 0; // Update cap for spin patterns
    uint8_t pattern = segment_map[num];

    // Common Cathode: 1 = Segment ON
    GPIO_PinState on = SEG_COMMON_ANODE ? GPIO_PIN_RESET : GPIO_PIN_SET;
    GPIO_PinState off = SEG_COMMON_ANODE ? GPIO_PIN_SET : GPIO_PIN_RESET;

    HAL_GPIO_WritePin(SEG_A_Port, SEG_A_Pin, (pattern & 0x01) ? on : off);
    HAL_GPIO_WritePin(SEG_B_Port, SEG_B_Pin, (pattern & 0x02) ? on : off);
    HAL_GPIO_WritePin(SEG_C_Port, SEG_C_Pin, (pattern & 0x04) ? on : off);
    HAL_GPIO_WritePin(SEG_D_Port, SEG_D_Pin, (pattern & 0x08) ? on : off);
    HAL_GPIO_WritePin(SEG_E_Port, SEG_E_Pin, (pattern & 0x10) ? on : off);
    HAL_GPIO_WritePin(SEG_F_Port, SEG_F_Pin, (pattern & 0x20) ? on : off);
    HAL_GPIO_WritePin(SEG_G_Port, SEG_G_Pin, (pattern & 0x40) ? on : off);
}

// ... (Existing Read_ADC and Color functions remain the same) ...
uint16_t Read_ADC1(void) {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 50) == HAL_OK) return HAL_ADC_GetValue(&hadc1);
    return 0;
}
uint16_t Read_ADC2(void) {
    HAL_ADC_Start(&hadc2);
    if (HAL_ADC_PollForConversion(&hadc2, 50) == HAL_OK) return HAL_ADC_GetValue(&hadc2);
    return 0;
}
Color Get_Color(uint16_t val) {
    if (val <= THRESH_OFF) return OFF;
    if (val <= THRESH_LOW) return BLUE;
    if (val <= THRESH_HIGH) return GREEN;
    return RED;
}
Color Mix_Colors(Color c1, Color c2) {
    Color result;
    result.r = c1.r | c2.r;
    result.g = c1.g | c2.g;
    result.b = c1.b | c2.b;
    if (result.r && result.g && result.b) result.name = "WHITE";
    else if (result.r && result.g)        result.name = "YELLOW";
    else if (result.r && result.b)        result.name = "MAGENTA";
    else if (result.g && result.b)        result.name = "CYAN";
    else if (result.r)                    result.name = "RED";
    else if (result.g)                    result.name = "GREEN";
    else if (result.b)                    result.name = "BLUE";
    else                                  result.name = "OFF";
    return result;
}
int Colors_Match(Color c1, Color c2) {
    return (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b);
}
void Pick_New_Target(void) {
    int r = (HAL_GetTick() + rand()) % 6;
    switch(r) {
        case 0: target_color = RED; break;
        case 1: target_color = GREEN; break;
        case 2: target_color = BLUE; break;
        case 3: target_color = YELLOW; break;
        case 4: target_color = CYAN; break;
        case 5: target_color = MAGENTA; break;
    }
    Set_LED_Target(target_color);
}
void Set_LED_P1(Color c) {
    HAL_GPIO_WritePin(P1_R_Port, P1_R_Pin, !c.r);
    HAL_GPIO_WritePin(P1_G_Port, P1_G_Pin, !c.g);
    HAL_GPIO_WritePin(P1_B_Port, P1_B_Pin, !c.b);
}
void Set_LED_P2(Color c) {
    HAL_GPIO_WritePin(P2_R_Port, P2_R_Pin, !c.r);
    HAL_GPIO_WritePin(P2_G_Port, P2_G_Pin, !c.g);
    HAL_GPIO_WritePin(P2_B_Port, P2_B_Pin, !c.b);
}
void Set_LED_Mix(Color c) {
    HAL_GPIO_WritePin(MIX_R_Port, MIX_R_Pin, !c.r);
    HAL_GPIO_WritePin(MIX_G_Port, MIX_G_Pin, !c.g);
    HAL_GPIO_WritePin(MIX_B_Port, MIX_B_Pin, !c.b);
}
void Set_LED_Target(Color c) {
    HAL_GPIO_WritePin(TGT_R_Port, TGT_R_Pin, !c.r);
    HAL_GPIO_WritePin(TGT_G_Port, TGT_G_Pin, !c.g);
    HAL_GPIO_WritePin(TGT_B_Port, TGT_B_Pin, !c.b);
}

void Game_Over_Sequence(void) {
    sprintf(msg, "\r\n!!! GAME COMPLETED in %d seconds !!!\r\n", game_timer_val);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);

    // Spin Animation for 3 seconds
    int spin_index = 10;
    for (int i = 0; i < 30; i++) {
        // Toggle RGBs randomly for disco effect
        Set_LED_P1((Color){rand()%2, rand()%2, rand()%2, ""});
        Set_LED_P2((Color){rand()%2, rand()%2, rand()%2, ""});
        Set_LED_Mix((Color){rand()%2, rand()%2, rand()%2, ""});
        Set_LED_Target((Color){rand()%2, rand()%2, rand()%2, ""});

        // Spin Segment Logic
        // Indices 10-15 correspond to Segments A-F
        disp_digit_1 = spin_index;
        disp_digit_2 = spin_index;
        disp_digit_3 = spin_index;

        spin_index++;
        if (spin_index > 15) spin_index = 10;

        HAL_Delay(100);
    }

    // --- STOP STATE ---
    Set_LED_P1(OFF);
    Set_LED_P2(OFF);
    Set_LED_Mix(OFF);
    Set_LED_Target(OFF);

    // Show Final Time
    disp_digit_1 = 0; // Score 0
    disp_digit_2 = game_timer_val / 10;
    disp_digit_3 = game_timer_val % 10;

    sprintf(msg, "Press Reset Button to Play Again.\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);

    // Freeze Here
    while(1) {
        HAL_GPIO_TogglePin(LD2_Port, LD2_Pin);
        HAL_Delay(500);
    }
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_TIM10_Init(); // Init Timer 10

  /* USER CODE BEGIN 2 */
  // Start the Multiplexing Interrupt
  HAL_TIM_Base_Start_IT(&htim10);

  Set_LED_P1(OFF);
  Set_LED_P2(OFF);
  Set_LED_Mix(OFF);

  score = STARTING_SCORE;
  disp_digit_1 = score; // Set display buffer

  Pick_New_Target();

  sprintf(msg, "\r\n=== ROUND %d START ===\r\n", round_count);
  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 50);
  /* USER CODE END 2 */

  while (1)
  {
    HAL_GPIO_TogglePin(LD2_Port, LD2_Pin);

    // --- GAME TIMER LOGIC (0-99) ---
    if (HAL_GetTick() - last_timer_tick > 1000) { // Every 1 second
        game_timer_val++;
        if (game_timer_val > 99) game_timer_val = 0;

        // Update Display Buffers for Timer
        disp_digit_2 = game_timer_val / 10; // Tens
        disp_digit_3 = game_timer_val % 10; // Ones

        last_timer_tick = HAL_GetTick();
    }

    // 1. Read & Process
    uint16_t s1 = Read_ADC1();
    uint16_t s2 = Read_ADC2();
    Color c1 = Get_Color(s1);
    Color c2 = Get_Color(s2);
    Color mix = Mix_Colors(c1, c2);

    // 2. Output to LEDs
    Set_LED_P1(c1);
    Set_LED_P2(c2);
    Set_LED_Mix(mix);

    // 3. Win Check
    if (Colors_Match(mix, target_color)) {
        win_timer++;

        if (win_timer >= WIN_HOLD_COUNT) {
            round_count++;

            // Decrement Score
            if (score > 0) {
                score--;
            }

            // Update Score Display Buffer
            disp_digit_1 = score;

            sprintf(msg, "\r\n*** WIN! Score: %d ***\r\n", score);
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 50);

            // If Game Over
            if (score == 0) {
                Game_Over_Sequence();
            } else {
                // Normal Round Victory Flash
                for(int i=0; i<3; i++) {
                    Set_LED_Target(OFF); Set_LED_Mix(OFF); HAL_Delay(100);
                    Set_LED_Target(target_color); Set_LED_Mix(mix); HAL_Delay(100);
                }
                win_timer = 0;
                Pick_New_Target();
            }
        }
    } else {
        win_timer = 0;
    }

    // 4. Non-Blocking Status Log
    if (HAL_GetTick() - last_log_time > 200) {
        if (win_timer == 0) {
            sprintf(msg, "P1:%s | P2:%s | MIX:%s | TGT:%s\r", c1.name, c2.name, mix.name, target_color.name);
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 20);
        }
        last_log_time = HAL_GetTick();
    }

    HAL_Delay(50);
  }
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

static void MX_ADC1_Init(void)
{
  __HAL_RCC_ADC1_CLK_ENABLE();
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
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

static void MX_ADC2_Init(void)
{
  __HAL_RCC_ADC2_CLK_ENABLE();
  ADC_ChannelConfTypeDef sConfig = {0};
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.ScanConvMode = DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  hadc2.Init.DMAContinuousRequests = DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  HAL_ADC_Init(&hadc2);
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  HAL_ADC_ConfigChannel(&hadc2, &sConfig);
}

// --- MISSING FUNCTION ADDED HERE ---
static void MX_TIM10_Init(void)
{
  // Enable TIM10 Clock
  __HAL_RCC_TIM10_CLK_ENABLE();

  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 83; // 84MHz -> 1MHz
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 1999;  // 1MHz / 2000 = 500Hz
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }

  // Enable Interrupt in NVIC
  HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
}

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

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // Initialize LED/DIGIT control pins to OFF (High)
  HAL_GPIO_WritePin(GPIOC, P1_R_Pin|P1_G_Pin|P1_B_Pin|TGT_R_Pin|TGT_G_Pin|TGT_B_Pin|DIG1_COM_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, P2_R_Pin|P2_G_Pin|P2_B_Pin|MIX_R_Pin|DIG2_COM_Pin|DIG3_COM_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, MIX_G_Pin|MIX_B_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LD2_Port, LD2_Pin, GPIO_PIN_RESET);

  // Initialize Segment Pins LOW
  HAL_GPIO_WritePin(SEG_A_Port, SEG_A_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SEG_B_Port, SEG_B_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SEG_C_Port, SEG_C_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SEG_D_Port, SEG_D_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SEG_E_Port, SEG_E_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SEG_F_Port, SEG_F_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SEG_G_Port, SEG_G_Pin, GPIO_PIN_RESET);

  // Player 1 + Target + Segment A,F,G + DIGIT 1 (Score)
  GPIO_InitStruct.Pin = P1_R_Pin|P1_G_Pin|P1_B_Pin|TGT_R_Pin|TGT_G_Pin|TGT_B_Pin|DIG1_COM_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  // Player 2 + Mix Red + Segment B,C,D,E + DIGIT 2,3 (Timer)
  GPIO_InitStruct.Pin = P2_R_Pin|P2_G_Pin|P2_B_Pin|MIX_R_Pin|SEG_B_Pin|SEG_C_Pin|SEG_D_Pin|SEG_E_Pin|DIG2_COM_Pin|DIG3_COM_Pin;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Mixing LED Green/Blue + Segment A,F,G
  GPIO_InitStruct.Pin = MIX_G_Pin|MIX_B_Pin|SEG_A_Pin|SEG_F_Pin|SEG_G_Pin;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // LD2
  GPIO_InitStruct.Pin = LD2_Pin;
  HAL_GPIO_Init(LD2_Port, &GPIO_InitStruct);

  // ADC Pins
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// Timer 10 Interrupt Vector (Must match IOC generated name if different)
// Usually put in stm32f4xx_it.c, but we rely on HAL_TIM_PeriodElapsedCallback here
void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
