/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Digital Lock - Clean Serial Output (No Key Echo)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// --- Keypad & LED Definitions ---
#define R1_PORT GPIOA
#define R1_PIN  GPIO_PIN_0
#define R2_PORT GPIOA
#define R2_PIN  GPIO_PIN_1
#define R3_PORT GPIOA
#define R3_PIN  GPIO_PIN_2
#define R4_PORT GPIOA
#define R4_PIN  GPIO_PIN_3

#define C1_PORT GPIOA
#define C1_PIN  GPIO_PIN_4
#define C2_PORT GPIOA
#define C2_PIN  GPIO_PIN_5
#define C3_PORT GPIOA
#define C3_PIN  GPIO_PIN_6

#define LED_GREEN_PORT GPIOB
#define LED_GREEN_PIN  GPIO_PIN_0
#define LED_RED_PORT   GPIOB
#define LED_RED_PIN    GPIO_PIN_1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
// --- Password Logic Variables ---
uint8_t stored_password[4] = {1, 2, 3, 4};
uint8_t input_buffer[4] = {0};
uint8_t input_count = 0;
uint8_t wrong_attempts = 0;

// --- UART Interrupt Variables ---
uint8_t rx_byte;
uint8_t setup_buffer[4];
volatile uint8_t setup_count = 0;
volatile uint8_t config_mode = 1;

// --- System States ---
typedef enum {
    STATE_IDLE,
    STATE_CHECKING,
    STATE_GRANTED,
    STATE_WRONG,
    STATE_LOCKED
} SystemState;

SystemState current_state = STATE_IDLE;
uint32_t lockout_timer = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// --- 4x3 Keypad Scanning ---
uint8_t Keypad_GetBinary(void) {
    uint8_t keys[4][3] = {
        {1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 0, 11}
    };

    for (int row = 0; row < 4; row++) {
        HAL_GPIO_WritePin(R1_PORT, R1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(R2_PORT, R2_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(R3_PORT, R3_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(R4_PORT, R4_PIN, GPIO_PIN_SET);

        switch(row) {
            case 0: HAL_GPIO_WritePin(R1_PORT, R1_PIN, GPIO_PIN_RESET); break;
            case 1: HAL_GPIO_WritePin(R2_PORT, R2_PIN, GPIO_PIN_RESET); break;
            case 2: HAL_GPIO_WritePin(R3_PORT, R3_PIN, GPIO_PIN_RESET); break;
            case 3: HAL_GPIO_WritePin(R4_PORT, R4_PIN, GPIO_PIN_RESET); break;
        }

        if (HAL_GPIO_ReadPin(C1_PORT, C1_PIN) == GPIO_PIN_RESET) { HAL_Delay(200); return keys[row][0]; }
        if (HAL_GPIO_ReadPin(C2_PORT, C2_PIN) == GPIO_PIN_RESET) { HAL_Delay(200); return keys[row][1]; }
        if (HAL_GPIO_ReadPin(C3_PORT, C3_PIN) == GPIO_PIN_RESET) { HAL_Delay(200); return keys[row][2]; }
    }
    return 255;
}

// --- UART INTERRUPT CALLBACK ---
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1 && config_mode == 1)
    {
        // Filter: Only accept numbers 0-9
        if (rx_byte >= '0' && rx_byte <= '9') {
            setup_buffer[setup_count] = rx_byte - '0';
            setup_count++;

            // ECHO: Send digit back to PC
            HAL_UART_Transmit(&huart1, &rx_byte, 1, 100);

            if (setup_count >= 4) {
                stored_password[0] = setup_buffer[0];
                stored_password[1] = setup_buffer[1];
                stored_password[2] = setup_buffer[2];
                stored_password[3] = setup_buffer[3];
                config_mode = 0; // Exit loop
            }
        }
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
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
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  // --- Init OLED ---
  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(10, 10);
  ssd1306_WriteString("SYSTEM BOOT", Font_7x10, White);
  ssd1306_UpdateScreen();

  HAL_Delay(1000);

  // --- CONFIGURATION MODE ---
  ssd1306_Fill(Black);
  ssd1306_SetCursor(5, 20);
  ssd1306_WriteString("SET PASSWORD", Font_7x10, White);
  ssd1306_SetCursor(5, 40);
  ssd1306_WriteString("KEYPAD OR UART", Font_7x10, White);
  ssd1306_UpdateScreen();

  char msg[] = "\r\n\r\n=== SYSTEM STARTUP ===\r\nEnter 4-digit password:\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)msg, sizeof(msg), 1000);

  // Start UART Listening
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

  // --- SETUP LOOP: LISTENS TO BOTH KEYPAD AND UART ---
  while (config_mode == 1) {

      // 1. Check Keypad
      uint8_t key = Keypad_GetBinary();

      // If a valid number key (0-9) is pressed
      if (key <= 9) {
          setup_buffer[setup_count] = key;
          setup_count++;

          // Echo only during setup so user knows it registered
          char k_msg[2];
          sprintf(k_msg, "%d", key);
          HAL_UART_Transmit(&huart1, (uint8_t*)k_msg, 1, 100);

          // Check if done
          if (setup_count >= 4) {
                stored_password[0] = setup_buffer[0];
                stored_password[1] = setup_buffer[1];
                stored_password[2] = setup_buffer[2];
                stored_password[3] = setup_buffer[3];
                config_mode = 0; // Exit loop
          }
      }
      HAL_Delay(50);
  }

  char done_msg[] = "\r\nPassword Saved! System Ready.\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)done_msg, sizeof(done_msg), 1000);

  // --- IDLE MODE ---
  ssd1306_Fill(Black);
  ssd1306_SetCursor(5, 20);
  ssd1306_WriteString("ENTER PASSWORD", Font_7x10, White);
  ssd1306_UpdateScreen();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // ---------------------------------------------------------
      // 1. STATE: LOCKED
      // ---------------------------------------------------------
      if (current_state == STATE_LOCKED) {
          uint32_t elapsed_time = HAL_GetTick() - lockout_timer;
          int16_t seconds_left = 30 - (elapsed_time / 1000);

          if (seconds_left < 0) {
              current_state = STATE_IDLE;
              wrong_attempts = 0;
              ssd1306_Fill(Black);
              ssd1306_SetCursor(5, 20);
              ssd1306_WriteString("ENTER PASSWORD", Font_7x10, White);
              ssd1306_UpdateScreen();
              HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);

              HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n[SYSTEM] Unlocked via Timer\r\n", 29, 100);
          }
          else {
              static uint8_t last_disp = 255;
              if (seconds_left != last_disp) {
                  last_disp = seconds_left;
                  ssd1306_Fill(Black);
                  ssd1306_SetCursor(10, 5);
                  ssd1306_WriteString("SYSTEM LOCKED", Font_7x10, White);
                  char t_msg[20];
                  sprintf(t_msg, "TRY AFTER %ds", seconds_left);
                  ssd1306_SetCursor(10, 25);
                  ssd1306_WriteString(t_msg, Font_7x10, White);
                  ssd1306_UpdateScreen();
                  HAL_GPIO_TogglePin(LED_RED_PORT, LED_RED_PIN);
              }
          }
          continue;
      }

      // ---------------------------------------------------------
      // 2. STATE: IDLE (Keypad Logic)
      // ---------------------------------------------------------
      uint8_t key = Keypad_GetBinary();

      if (key != 255) {
          if (key <= 9) {
              if (input_count < 4) {
                  input_buffer[input_count] = key;
                  input_count++;

                  // Update OLED with '*'
                  ssd1306_SetCursor(10 + (input_count * 10), 40);
                  ssd1306_WriteString("*", Font_11x18, White);
                  ssd1306_UpdateScreen();

                  // REMOVED SERIAL ECHO HERE
              }
          }
          else if (key == 11) { // Enter (#)
              // REMOVED SERIAL ECHO HERE
              if (input_count == 4) {
                  current_state = STATE_CHECKING;
              }
          }
          else if (key == 10) { // Clear (*)
              input_count = 0;
              // REMOVED SERIAL ECHO HERE
              ssd1306_Fill(Black);
              ssd1306_SetCursor(5, 20);
              ssd1306_WriteString("ENTER PASSWORD", Font_7x10, White);
              ssd1306_UpdateScreen();
          }
      }

      // ---------------------------------------------------------
      // 3. CHECK PASSWORD
      // ---------------------------------------------------------
      if (current_state == STATE_CHECKING) {
          int match = 1;
          for (int i = 0; i < 4; i++) {
              if (input_buffer[i] != stored_password[i]) { match = 0; break; }
          }

          if (match) current_state = STATE_GRANTED;
          else current_state = STATE_WRONG;
          input_count = 0;
      }

      // ---------------------------------------------------------
      // 4. ACCESS GRANTED
      // ---------------------------------------------------------
      if (current_state == STATE_GRANTED) {
          ssd1306_Fill(Black);
          ssd1306_SetCursor(10, 20);
          ssd1306_WriteString("ACCESS GRANTED", Font_7x10, White);
          ssd1306_UpdateScreen();
          HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET);

          HAL_UART_Transmit(&huart1, (uint8_t*)"[STATUS] Access Granted\r\n", 25, 100);

          HAL_Delay(3000);
          HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
          wrong_attempts = 0;
          current_state = STATE_IDLE;
          ssd1306_Fill(Black);
          ssd1306_SetCursor(5, 20);
          ssd1306_WriteString("ENTER PASSWORD", Font_7x10, White);
          ssd1306_UpdateScreen();
      }

      // ---------------------------------------------------------
      // 5. WRONG PASSWORD
      // ---------------------------------------------------------
      if (current_state == STATE_WRONG) {
          wrong_attempts++;
          ssd1306_Fill(Black);
          ssd1306_SetCursor(10, 20);
          ssd1306_WriteString("WRONG PASSWORD", Font_7x10, White);
          ssd1306_UpdateScreen();
          HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);

          HAL_UART_Transmit(&huart1, (uint8_t*)"[STATUS] Wrong Password\r\n", 25, 100);

          HAL_Delay(1000);
          HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);

          if (wrong_attempts >= 3) {
              current_state = STATE_LOCKED;
              lockout_timer = HAL_GetTick();

              HAL_UART_Transmit(&huart1, (uint8_t*)"[STATUS] SYSTEM LOCKED\r\n", 24, 100);

              ssd1306_Fill(Black);
              ssd1306_SetCursor(10, 10);
              ssd1306_WriteString("SYSTEM LOCKED", Font_7x10, White);
              ssd1306_UpdateScreen();
          } else {
              current_state = STATE_IDLE;
              ssd1306_Fill(Black);
              ssd1306_SetCursor(5, 20);
              ssd1306_WriteString("ENTER PASSWORD", Font_7x10, White);
              ssd1306_UpdateScreen();
          }
      }
    /* USER CODE END WHILE */
  }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
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
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
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
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_RESET);

  // Keypad Rows (Output)
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Keypad Cols (Input with Pull-Up)
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // LEDs (Output)
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) { }
#endif
