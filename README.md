# STM32 Advanced Digital Lock System

A secure digital lock implementation using an **STM32F103 (Blue Pill)**. This project features a multi-state machine, I2C OLED display, and dynamic password configuration via UART interrupts.

## 🚀 Key Features
- **Interrupt-Driven UART:** Configure the master password at startup via a Serial Terminal.
- **State Machine Logic:** 5 distinct states (`IDLE`, `CHECKING`, `GRANTED`, `WRONG`, `LOCKED`).
- **Security Lockout:** 3 failed attempts trigger a 30-second hardware-timer lockout with a live countdown on the OLED.
- **Visual Feedback:** SSD1306 OLED displays real-time status and masked password entry (`****`).

## 🔌 Hardware Setup: The Arduino UART Bridge
Instead of a standard USB-to-TTL adapter, this project utilizes an **Arduino Uno** as a serial bridge. This is achieved through a hardware bypass:



### Wiring Diagram
| Component | Arduino Uno Pin | STM32 Pin | Purpose |
| :--- | :--- | :--- | :--- |
| **Bypass** | RESET | GND | Disables ATmega328P to use USB-Serial chip only |
| **Ground** | GND | GND | Common Ground (Required) |
| **UART RX**| RX (0) | PA10 (RX1) | Data from PC to STM32 |
| **UART TX**| TX (1) | PA9 (TX1) | Data from STM32 to PC |



> **Note:** When using the Arduino as a bridge, connect RX-to-RX and TX-to-TX. This is because we are bypassing the onboard MCU to speak directly to the USB controller.

## 🛠️ Software Details
- **Environment:** STM32CubeIDE
- **Library:** HAL (Hardware Abstraction Layer)
- **Baud Rate:** 9600 (Optimized for stability over jumper wires)

## 📖 How to Use
1. Connect the hardware and open the Serial Monitor (9600 Baud).
2. Reset the STM32. 
3. Follow the UART prompt: `Enter 4-digit password:`.
4. Use the 4x3 Keypad to enter the PIN. Press `#` to confirm.
