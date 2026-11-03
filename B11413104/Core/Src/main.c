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
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
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

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
typedef enum {
    STATE_IDLE,       // 起始狀態：LED 恆亮
    STATE_PLAYING,    // 遊戲進行中：按下的 LED 熄滅
    STATE_SUCCESS,    // 成功：呼吸燈效果
    STATE_FAILED      // 失敗：四顆燈快速閃爍
} GameState_t;

GameState_t gameState = STATE_IDLE;

// 遊戲參數
uint8_t wireCount = 4;                     // 引線數量 (1~4)
char wireSeq[5] = "BRYG";                  // 引線正確順序字串 (R, Y, G, B)
uint32_t totalTimeSec = 30;                // 倒數時間 (秒)

// 遊戲即時狀態
uint8_t currentStep = 0;                   // 當前已剪斷正確數
uint32_t gameStartTime = 0;                // 開始時間 (ms)
uint32_t lastReportTime = 0;               // 上次 UART 自動回報時間 (ms)
uint32_t breathTimer = 0;                  // 呼吸燈計時 (ms)
uint32_t blinkTimer = 0;                   // 閃爍計時 (ms)
bool ledBlinkState = false;

// LED 個別點亮/熄滅狀態 (1:亮, 0:滅)
uint8_t ledStatus[4] = {1, 1, 1, 1}; // 0:Red, 1:Yellow, 2:Green, 3:Blue

// 按鈕引線當前狀態 (true: 已剪斷/按過)
bool wireCut[4] = {false, false, false, false};

// LPUART1 緩衝區
#define RX_BUFF_SIZE 64
uint8_t rxBuffer[RX_BUFF_SIZE];
uint8_t rxChar;
uint8_t rxIndex = 0;

// 按鈕防彈跳時間
uint32_t lastBtnTime[4] = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void SendUART(const char *str);
void ProcessCommand(char *cmd);
void UpdateLEDs(void);
void SetLEDs(uint8_t r, uint8_t y, uint8_t g, uint8_t b); // <-- 補上這行宣告
void ReadButtons(void);
void EndGame(bool success, const char *reason);
void PrintHelp(void);
void PrintStatus(void);
uint8_t ColorToIdx(char c);
char IdxToColorChar(uint8_t idx);
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
  MX_LPUART1_UART_Init();
  // 啟動 UART 中斷接收
    HAL_UART_Receive_IT(&hlpuart1, &rxChar, 1);

    SendUART("\r\n========================================\r\n");
    SendUART(" [Bomb Defuse Simulator] System Ready.\r\n");
    SendUART(" Type 'HELP' to show available commands.\r\n");
    SendUART("========================================\r\n");
  MX_TIM1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  // 重新將 PA8, PA9, PA10, PB4 配置為 GPIO Input + 上拉電阻 (防雜訊與防誤觸)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct); // PA8 (RED), PA9 (YELLOW), PA10 (GREEN)

    GPIO_InitStruct.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct); // PB4 (BLUE)

    // 將 PA0, PA1, PC2, PC3 設定為 GPIO 數位輸出 (控制 4 顆 LED)
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct); // PA0 (RED LED), PA1 (YELLOW LED)

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct); // PC2 (GREEN LED), PC3 (BLUE LED)

    // 啟動 LPUART1 中斷接收
    HAL_UART_Receive_IT(&hlpuart1, &rxChar, 1);

    SendUART("\r\n========================================\r\n");
    SendUART(" [Bomb Defuse Simulator] System Ready.\r\n");
    SendUART(" Type 'HELP' to show available commands.\r\n");
    SendUART("========================================\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  uint32_t now = HAL_GetTick();

	        // ----------------------------------------------------
	        // 1. 狀態機與 LED 燈效處理
	        // ----------------------------------------------------
	        switch (gameState) {
	            case STATE_IDLE:
	                // 起始狀態：4 顆 LED 恆亮
	                SetLEDs(1, 1, 1, 1);
	                break;

	            case STATE_PLAYING:
	                // 遊戲倒數計時
	                {
	                    uint32_t elapsedSec = (now - gameStartTime) / 1000;
	                    if (elapsedSec >= totalTimeSec) {
	                        EndGame(false, "TIME EXPIRED! BOMB EXPLODED!");
	                    } else {
	                        // 每秒透過 UART 自動顯示倒數與進度
	                        if (now - lastReportTime >= 1000) {
	                            lastReportTime = now;
	                            char msg[64];
	                            uint32_t remain = totalTimeSec - elapsedSec;
	                            snprintf(msg, sizeof(msg), "[TIMER] Remaining: %lu s | Defused: %d/%d\r\n",
	                                     remain, currentStep, wireCount);
	                            SendUART(msg);
	                        }
	                    }
	                }
	                UpdateLEDs();
	                ReadButtons();
	                break;

	            case STATE_SUCCESS:
	                // 【通關：呼吸燈效果】利用高頻 PWM 模擬流暢呼吸燈 (週期約 1.5 秒)
	                if (now - breathTimer >= 15) {
	                    breathTimer = now;

	                    static int brightness = 0;   // 亮度 (0 ~ 100)
	                    static int fadeDir = 1;      // 漸變方向 (1:變亮, -1:變暗)

	                    brightness += fadeDir * 3;
	                    if (brightness >= 100) {
	                        brightness = 100;
	                        fadeDir = -1;
	                    } else if (brightness <= 0) {
	                        brightness = 0;
	                        fadeDir = 1;
	                    }

	                    // 軟體 PWM 脈衝寬度調變
	                    for (int i = 0; i < 10; i++) {
	                        if (i < (brightness / 10)) {
	                            SetLEDs(1, 1, 1, 1);
	                        } else {
	                            SetLEDs(0, 0, 0, 0);
	                        }
	                        // 產生 100us 極微小延遲形成 PWM 視覺效果
	                        for (volatile int delay = 0; delay < 200; delay++);
	                    }
	                }
	                break;

	            case STATE_FAILED:
	                // 【失敗：四顆燈快速閃爍】(每 100ms 切換全亮 / 全滅)
	                if (now - blinkTimer >= 100) {
	                    blinkTimer = now;
	                    ledBlinkState = !ledBlinkState;
	                    if (ledBlinkState) {
	                        SetLEDs(1, 1, 1, 1); // 4 顆全亮
	                    } else {
	                        SetLEDs(0, 0, 0, 0); // 4 顆全滅
	                    }
	                }
	                break;
	      }
	    }
	  }

	  // ----------------------------------------------------
	  // 工具函式
	  // ----------------------------------------------------
	  void SendUART(const char *str) {
	      HAL_UART_Transmit(&hlpuart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
	  }

	  // 轉換顏色字元與索引 (0:R, 1:Y, 2:G, 3:B)
	  uint8_t ColorToIdx(char c) {
	      switch (toupper(c)) {
	          case 'R': return 0;
	          case 'Y': return 1;
	          case 'G': return 2;
	          case 'B': return 3;
	          default:  return 255;
	      }
	  }

	  char IdxToColorChar(uint8_t idx) {
	      switch(idx) {
	          case 0: return 'R';
	          case 1: return 'Y';
	          case 2: return 'G';
	          case 3: return 'B';
	          default: return '?';
	      }
	  }

	  // 更新物理 GPIO 點亮或熄滅
	  void UpdateLEDs(void) {
	      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, ledStatus[0] ? GPIO_PIN_SET : GPIO_PIN_RESET); // RED
	      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, ledStatus[1] ? GPIO_PIN_SET : GPIO_PIN_RESET); // YELLOW
	      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, ledStatus[2] ? GPIO_PIN_SET : GPIO_PIN_RESET); // GREEN
	      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, ledStatus[3] ? GPIO_PIN_SET : GPIO_PIN_RESET); // BLUE
	  }

	  // 印出 HELP 指令資訊
	  void PrintHelp(void) {
	      SendUART("\r\n=== COMMAND LIST ===\r\n");
	      SendUART(" HELP           : Show command manual\r\n");
	      SendUART(" SET <N> <SEQ>  : Set wire count N (1-4) & sequence string (e.g. SET 4 BRYG)\r\n");
	      SendUART(" TIME <SEC>     : Set countdown timer in seconds (e.g. TIME 30)\r\n");
	      SendUART(" START          : Start game and countdown\r\n");
	      SendUART(" STATUS         : View current settings and game status\r\n");
	      SendUART(" RESET          : Reset back to IDLE state\r\n");
	      SendUART("====================\r\n\r\n");
	  }

	  // 印出 STATUS 目前設定與狀態
	  void PrintStatus(void) {
	      char msg[128];
	      snprintf(msg, sizeof(msg),
	               "\r\n=== GAME STATUS ===\r\n State: %s\r\n Target Wires: %d\r\n Sequence: %s\r\n Time: %lu s\r\n===================\r\n\r\n",
	               (gameState == STATE_IDLE) ? "IDLE" :
	               (gameState == STATE_PLAYING) ? "PLAYING" :
	               (gameState == STATE_SUCCESS) ? "SUCCESS" : "FAILED",
	               wireCount, wireSeq, totalTimeSec);
	      SendUART(msg);
	  }

	  // 讀取按鈕與剪線 Logic (加入防懸空與去彈跳機制)
	  void ReadButtons(void) {
		  if (gameState != STATE_PLAYING) return;

		      GPIO_TypeDef* ports[4] = {GPIOA, GPIOA, GPIOA, GPIOB};
		      uint16_t pins[4] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_4};
		      uint32_t now = HAL_GetTick();

		      // 遊戲開始前 500ms 忽略按鈕，避免啟動電位突波
		      if (now - gameStartTime < 500) return;

		      for (int i = 0; i < 4; i++) {
		          // 第一次判斷：腳位是否為 Low (Active Low)
		          if (HAL_GPIO_ReadPin(ports[i], pins[i]) == GPIO_PIN_RESET && !wireCut[i]) {
		              if (now - lastBtnTime[i] > 300) {

		                  // 二次延遲確認：防止懸空靜電或雜訊突波誤判
		                  HAL_Delay(5);
		                  if (HAL_GPIO_ReadPin(ports[i], pins[i]) != GPIO_PIN_RESET) {
		                      continue; // 屬雜訊突波，忽略
		                  }

		                  lastBtnTime[i] = now;
		                  wireCut[i] = true;
		                  ledStatus[i] = 0; // 按下後對應 LED 熄滅

		                  char pressedColor = IdxToColorChar(i);
		                  char expectedColor = wireSeq[currentStep];

		                  if (pressedColor == expectedColor) {
		                      currentStep++;

		                      char msg[64];
		                      const char* colorNames[] = {"RED", "YELLOW", "GREEN", "BLUE"};
		                      snprintf(msg, sizeof(msg), "CUT %s CORRECT (%d/%d)\r\n",
		                               colorNames[i], currentStep, wireCount);
		                      SendUART(msg);

		                      if (currentStep >= wireCount) {
		                          EndGame(true, "ALL WIRES DEFUSED PROPERLY!");
		                          return;
		                      }
		                  } else {
		                      // 按錯引線
		                      char msg[64];
		                      const char* colorNames[] = {"RED", "YELLOW", "GREEN", "BLUE"};
		                      snprintf(msg, sizeof(msg), "CUT %s WRONG! EXPECTED '%c'\r\n",
		                               colorNames[i], expectedColor);
		                      SendUART(msg);
		                      EndGame(false, "WRONG WIRE ORDER CUT!");
		                      return;
		                  }
		              }
		          }
		      }
	  }
	  // 結束遊戲與結算成績
	  void EndGame(bool success, const char *reason) {
	      uint32_t usedTime = (HAL_GetTick() - gameStartTime) / 1000;

	      if (success) {
	          gameState = STATE_SUCCESS;
	          SendUART("\r\n========================================\r\n");
	          SendUART(" *** BOMB DEFUSED SUCCESSFULLY! ***\r\n");
	          char scoreMsg[64];
	          snprintf(scoreMsg, sizeof(scoreMsg), " SCORE: SUCCESS | Time Spent: %lu s / %lu s\r\n",
	                   usedTime, totalTimeSec);
	          SendUART(scoreMsg);
	          SendUART("========================================\r\n");
	      } else {
	          gameState = STATE_FAILED;
	          SendUART("\r\n========================================\r\n");
	          SendUART(" !!! GAME OVER - BOMB EXPLODED !!!\r\n");
	          char failMsg[96];
	          snprintf(failMsg, sizeof(failMsg), " REASON: %s\r\n SCORE: FAILED | Completed: %d/%d Wires\r\n",
	                   reason, currentStep, wireCount);
	          SendUART(failMsg);
	          SendUART("========================================\r\n");
	      }
	  }

	  // UART 指令解析器
	  void ProcessCommand(char *cmd) {
	      // 轉成大寫方便比對
	      for (int i = 0; cmd[i]; i++) cmd[i] = toupper(cmd[i]);

	      if (strcmp(cmd, "HELP") == 0) {
	          PrintHelp();
	      }
	      else if (strcmp(cmd, "STATUS") == 0) {
	          PrintStatus();
	      }
	      else if (strncmp(cmd, "SET ", 4) == 0) {
	          // 解析 SET 4 BRYG
	          int num = 0;
	          char seqBuf[16] = {0};
	          if (sscanf(cmd + 4, "%d %15s", &num, seqBuf) == 2) {
	              if (num >= 1 && num <= 4 && strlen(seqBuf) == num) {
	                  wireCount = num;
	                  strncpy(wireSeq, seqBuf, num);
	                  wireSeq[num] = '\0';
	                  SendUART("[OK] Target wires and sequence updated.\r\n");
	              } else {
	                  SendUART("[ERROR] Invalid parameters. Usage: SET 4 BRYG (Length 1-4)\r\n");
	              }
	          }
	      }
	      else if (strncmp(cmd, "TIME ", 5) == 0) {
	          int sec = atoi(cmd + 5);
	          if (sec > 0) {
	              totalTimeSec = sec;
	              char msg[48];
	              snprintf(msg, sizeof(msg), "[OK] Countdown time set to %d seconds.\r\n", totalTimeSec);
	              SendUART(msg);
	          } else {
	              SendUART("[ERROR] Invalid time value.\r\n");
	          }
	      }
	      else if (strcmp(cmd, "START") == 0) {
	              if (gameState == STATE_PLAYING) {
	                  SendUART("[WARN] Game is already in progress!\r\n");
	                  return;
	              }
	          gameState = STATE_PLAYING;
	          currentStep = 0;
	          gameStartTime = HAL_GetTick();
	          lastReportTime = gameStartTime;

	          // 開始時徹底重置按鈕與 LED 狀態
	                  currentStep = 0;
	                  for (int i = 0; i < 4; i++) {
	                      wireCut[i] = false;
	                      ledStatus[i] = 1; // 4 顆 LED 恆亮
	                      lastBtnTime[i] = HAL_GetTick();
	                  }

	                  gameStartTime = HAL_GetTick();
	                  lastReportTime = gameStartTime;
	                  gameState = STATE_PLAYING;

	                  SendUART("\r\n[GAME] Start! Defuse the bomb now!\r\n");
	      }
	      else if (strcmp(cmd, "RESET") == 0) {
	          gameState = STATE_IDLE;
	          currentStep = 0;
	          for (int i = 0; i < 4; i++) {
	              wireCut[i] = false;
	              ledStatus[i] = 1;
	          }
	          SendUART("\r\n[GAME] Reset to IDLE.\r\n");
	      }
	      else {
	          SendUART("Unknown command. Type 'HELP' for options.\r\n");
	      }
	  }

	  // UART Rx 接收中斷回呼
	  void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	      if (huart->Instance == LPUART1) {
	          if (rxChar == '\r' || rxChar == '\n') {
	              if (rxIndex > 0) {
	                  rxBuffer[rxIndex] = '\0';
	                  ProcessCommand((char*)rxBuffer);
	                  rxIndex = 0;
	              }
	          } else {
	              if (rxIndex < RX_BUFF_SIZE - 1) {
	                  rxBuffer[rxIndex++] = rxChar;
	              }
	          }
	          HAL_UART_Receive_IT(&hlpuart1, &rxChar, 1);
	      }
	  }

  /* USER CODE END 3 */


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
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 169;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 169;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : GREEN_Pin BLUE_Pin */
  GPIO_InitStruct.Pin = GREEN_Pin|BLUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : RED_Pin YELLOW_Pin */
  GPIO_InitStruct.Pin = RED_Pin|YELLOW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// 控制 4 顆 LED 開關 (1:點亮, 0:熄滅)
void SetLEDs(uint8_t r, uint8_t y, uint8_t g, uint8_t b) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, r ? GPIO_PIN_SET : GPIO_PIN_RESET); // RED (PA0)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, y ? GPIO_PIN_SET : GPIO_PIN_RESET); // YELLOW (PA1)
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, g ? GPIO_PIN_SET : GPIO_PIN_RESET); // GREEN (PC2)
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, b ? GPIO_PIN_SET : GPIO_PIN_RESET); // BLUE (PC3)
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
