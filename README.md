# STM32-Based Advanced Digital Lock System

## Project Description
This repository contains the design and implementation of a secure digital lock system using an STM32 microcontroller. The system authenticates a user-entered password via a matrix keypad, provides real-time visual feedback through an I2C OLED display, and allows for dynamic password configuration via UART communication on system startup. 

The project strictly adheres to a state-machine architecture and implements security protocols including a hardware-timer-based lockout after three consecutive failed attempts.

## Core Features
* **State Machine Architecture:** The system transitions between defined states (IDLE, CHECKING, GRANTED, WRONG, LOCKED) to ensure predictable and secure operation.
* **UART Password Configuration:** On system boot, the master password is set via a Serial Terminal. The input logic includes strict numeric validation and requires exactly 4 digits.
* **Hardware Timer Lockout:** Three consecutive incorrect attempts trigger a 30-second system freeze. This is handled using the HAL_GetTick() hardware timer. Keypad input is explicitly disabled during the lockout phase.
* **Dynamic OLED Interface:** Displays state-dependent messages:
  * "ENTER PASSWORD" (Waiting for input)
  * "ACCESS GRANTED" (Authentication successful)
  * "WRONG PASSWORD" (Incorrect attempt)
  * "SYSTEM LOCKED – TRY AFTER 30s" (Live countdown during lockout)
* **Debouncing & Indication:** Matrix keypad scanning incorporates software debouncing. Status is indicated via the OLED and two discrete LEDs (Green for success, Red for error/lockout).

---

## Hardware Configuration & Pin Mapping

### STM32 Connections
| Component | STM32 Pin | GPIO Configuration |
| :--- | :--- | :--- |
| Keypad Rows (1-4) | PA0, PA1, PA2, PA3 | Output (Push-Pull) |
| Keypad Cols (1-4) | PA4, PA5, PA6, PA7 | Input (Pull-Up) |
| Green LED | PB0 | Output (Push-Pull) |
| Red LED | PB1 | Output (Push-Pull) |
| OLED SDA | PB7 | I2C1 Alternate Function Open-Drain |
| OLED SCL | PB6 | I2C1 Alternate Function Open-Drain |
| UART TX (To PC) | PA9 | USART1 TX (9600 Baud) |
| UART RX (From PC)| PA10 | USART1 RX (9600 Baud) |

### Arduino Uno as USB-to-TTL Serial Bridge
To establish UART communication with a PC terminal without a dedicated FTDI/TTL adapter, an Arduino Uno is utilized as a serial bridge via a hardware bypass.

1. **Disable Arduino MCU:** Connect a jumper wire between the RESET pin and the GND pin on the Arduino Uno. This holds the ATmega328P in reset, providing direct access to the board's USB-to-Serial converter chip.
2. **Wiring the Bridge:** Connect the STM32 UART pins directly to the Arduino's digital RX/TX pins. Do not cross the lines; wire them straight-through:
   * STM32 PA10 (RX) -> Arduino Pin 0 (RX)
   * STM32 PA9 (TX) -> Arduino Pin 1 (TX)
   * STM32 GND -> Arduino GND (Required for common voltage reference)

---

## Software Integration: OLED Display
The project uses the `afiskon/stm32-ssd1306` library for I2C OLED control. 

**Integration Steps:**
1. In STM32CubeMX, configure I2C1 for Standard Mode (100kHz).
2. Clone or download the [afiskon/stm32-ssd1306](https://github.com/afiskon/stm32-ssd1306) repository.
3. Copy the required header files (`ssd1306.h` and `ssd1306_fonts.h`) into the `Core/Inc/` directory of your STM32CubeIDE project.
4. Copy the required source files (`ssd1306.c` and `ssd1306_fonts.c`) into the `Core/Src/` directory.
5. Include the headers in `main.c` (`#include "ssd1306.h"`, `#include "ssd1306_fonts.h"`).
6. Initialize the display by calling `ssd1306_Init();` prior to the main infinite loop.

---

## System Testing & Verification Guide
To verify the system meets all functional requirements, execute the following test sequence:

1. **Initial Configuration (UART)**
   * Open a Serial Terminal at 9600 Baud with "Both NL & CR" enabled.
   * Reset the STM32. The terminal will prompt: `Enter new 4-digit password:`.
   * Input a 4-digit numeric code (e.g., `1234`). The system will echo the input and confirm setup.
2. **Idle State Check**
   * Observe the OLED. It must display "ENTER PASSWORD". Both LEDs should be off.
3. **Failed Authentication & Lockout Sequence**
   * Enter an incorrect 4-digit password on the matrix keypad.
   * Verify the Red LED activates briefly and the OLED displays "WRONG PASSWORD".
   * Repeat the incorrect entry two more times.
   * Verify the system enters the lockout state: Red LED toggles, and the OLED displays "SYSTEM LOCKED – TRY AFTER 30s" with a live, decrementing timer. 
   * Attempt to press keypad buttons during this 30-second window to confirm inputs are ignored.
4. **Successful Authentication**
   * Allow the lockout timer to expire, returning the system to the Idle state.
   * Enter the correct password configured in Step 1.
   * Verify the Green LED activates and the OLED displays "ACCESS GRANTED".

---

## Recommended Resources
For a comprehensive visual walkthrough of integrating the SSD1306 OLED display with an STM32 microcontroller and configuring the `afiskon` library in STM32CubeIDE, refer to the following tutorial:
* [STM32 OLED TUTORIAL by hacksontable](https://youtu.be/z1Px6emHIeg?si=ds9WqE2OpMnEoBCm)

## Author
Nithin Sanjith
