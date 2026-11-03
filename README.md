# 💣 STM32 拆彈專家資格挑戰模擬器 (Bomb Defuse Simulator)

> 一款基於 **STM32G474RE** 開發板的嵌入式模擬解謎遊戲。結合 **GPIO 輸入/輸出**、**UART 序列埠通訊解析** 與 **軟體/硬體 PWM 燈效控制**，打造具備倒數計時、自訂解謎關卡與燈光回饋的趣味實作專案。

> ⚠️ **聲明**：本專案為純粹由低電壓按鈕與 LED 組成的電子益智遊戲，不涉及任何真實爆裂物或危險裝置。

---

## 🌟 專案亮點 (Key Features)

- **動態關卡設定**：透過電腦 UART 終端機（Python / Serial Terminal）即時設定引線數量（1～4 條）、正確剪線順序與倒數時間。
- **即時遊戲狀態回饋**：
  - 遊戲進行中每秒透過 UART 自動回報剩餘時間與拆除進度。
  - 按下對應引線按鈕即時印出驗證結果（例如：`CUT YELLOW CORRECT (1/4)`）。
- **豐富的多型態 LED 燈效**：
  - **待機狀態 (IDLE)**：4 顆 LED 恆亮。
  - **遊戲進行中 (PLAYING)**：剪斷（按下）對應引線後，該顏色 LED 熄滅。
  - **挑戰成功 (SUCCESS)**：4 顆 LED 呈現流暢的**漸變呼吸燈**效果。
  - **挑戰失敗 (FAILED)**：4 顆 LED 呈現**高頻快速閃爍**效果。
- **穩定防護機制**：
  - **按鈕軟體防彈跳 (Debounce)** 與二次延遲確認，避免雜訊與突波誤觸發。
  - **輸入腳位內部上拉 (Pull-up)**，防止腳位懸空 (Floating) 導致遊戲自動判定失敗。

---

## 🛠️ 硬體與開發環境 (Hardware & Software)

### 硬體設備
- **開發板**：STMicroelectronics NUCLEO-G474RE (STM32G474RE, ARM Cortex-M4 @ 170 MHz)
- **外接週邊**：
  - 四色 LED（紅、黃、綠、藍）
  - 四個外接按鈕（代表紅、黃、綠、藍四條模擬引線）

### 開發環境與工具
- **IDE**：STM32CubeIDE (v1.10+ / v2.2.0)
- **程式語言**：C 語言
- **韌體框架**：STM32 HAL Library
- **電腦端測試介面**：Python 3.x (`pyserial` 模組)
- **通訊介面**：LPUART1 (115200 Baud, 8N1)

---

## 📌 腳位配置圖 (Pinout Configuration)

| 功能標示 | 代表顏色 / 引線 | STM32 腳位 | GPIO 模式 | 說明 |
| :--- | :--- | :--- | :--- | :--- |
| **LED 1** | 🔴 紅色 LED | `PA0` | GPIO Output | 高電位點亮 (Active High) |
| **LED 2** | 🟡 黃色 LED | `PA1` | GPIO Output | 高電位點亮 (Active High) |
| **LED 3** | 🟢 綠色 LED | `PC2` | GPIO Output | 高電位點亮 (Active High) |
| **LED 4** | 🔵 藍色 LED | `PC3` | GPIO Output | 高電位點亮 (Active High) |
| **BTN 1** | 🔴 紅引線按鈕 | `PA8` | Input with Pull-up | 低電位觸發 (Active Low) |
| **BTN 2** | 🟡 黃引線按鈕 | `PA9` | Input with Pull-up | 低電位觸發 (Active Low) |
| **BTN 3** | 🟢 綠引線按鈕 | `PA10` | Input with Pull-up | 低電位觸發 (Active Low) |
| **BTN 4** | 🔵 藍引線按鈕 | `PB4` | Input with Pull-up | 低電位觸發 (Active Low) |
| **UART** | ST-LINK 虛擬串口 | `PA2` (TX), `PA3` (RX) | LPUART1 | 115200 Baud / 8N1 |

---

## 💻 快速開始 (Quick Start)

### 1. 燒錄韌體至 STM32
1. 使用 **STM32CubeIDE** 開啟專案。
2. 將 NUCLEO-G474RE 連接至電腦。
3. 點擊 `Run` (或按 `Ctrl + F11`) 將韌體編譯並燒錄至開發板。

### 2. 執行電腦端 Python 控制腳本
1. 確保電腦已安裝 Python 與 `pyserial`：
   ```bash
   pip install pyserial
## 🎮 UART 指令操作手冊 (Command List)

通訊語法不區分大小寫，命令結尾請附帶 `\r\n`：

| 指令語法 | 說明 | 範例與回應 |
| :--- | :--- | :--- |
| `HELP` | 顯示所有可用指令選單 | 輸出指令說明與手冊 |
| `STATUS` | 查看當前設定與遊戲狀態 | 顯示 State, Wire Count, Sequence, Time |
| `SET <N> <SEQ>` | 設定引線數量 N (1~4) 與正確剪線順序 | `SET 4 BRYG` (設定順序：藍 -> 紅 -> 黃 -> 綠) |
| `TIME <SEC>` | 設定倒數時間 (秒) | `TIME 30` (設定倒數 30 秒) |
| `START` | 開始遊戲並啟動倒數 | 4 顆 LED 全亮，開始秒數回報與按鈕偵測 |
| `RESET` | 重置遊戲狀態至 IDLE | 終止當前遊戲，LED 恢復預設狀態 |

---

## 🔬 遊戲流程與邏輯 (Game Flow)

```text
       +-----------------------+
       |   IDLE 待機狀態       |
       |  (4 顆 LED 恆亮)      |
       +-----------+-----------+
                   |
             [輸入 START]
                   v
       +-----------------------+
       |  PLAYING 遊戲進行中   | <---+ 按下按鈕 (按對顏色)
       | (每秒 UART 自動回報)  | ----+ LED 熄滅 / 進度 +1
       +-----+-----------+-----+
             |           |
    [按錯順序/逾時]     [完成所有引線]
             |           |
             v           v
    +-----------------+ +-----------------+
    | FAILED 挑戰失敗 | | SUCCESS 挑戰成功|
    | (LED 快速閃爍)  | | (LED 漸變呼吸燈)|
    +-----------------+ +-----------------+

📂 專案目錄結構 (Directory Structure)
.
├── Core/
│   ├── Inc/
│   │   └── main.h             # 腳位與系統頭檔案
│   └── Src/
│       ├── main.c             # 遊戲主邏輯、狀態機與 UART 中斷處理
│       └── stm32g4xx_it.c     # 中斷服務常式
├── defuse_game.py             # 電腦端 Python UART 控制腳本
└── README.md                  # 專案說明文件
📝 授權條款 (License)
本專案採用 MIT License 授權開放，歡迎自由修補、改寫與作為教學範例使用。
