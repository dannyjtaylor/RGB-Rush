# 🔴🟢🔵 RGB Rush 🔴🟢🔵

An embedded real-time reaction game built on the **STM32 Nucleo-F446RE**
platform using **FreeRTOS**.

This project demonstrates modern embedded firmware architecture by
decoupling **sensor acquisition**, **game logic**, and **display
rendering** into concurrent RTOS tasks. It features analog sensor
processing, additive color mixing, and high-speed hardware-driven
multiplexing.

## 📖 Table of Contents

1.  Project Overview
2.  Game Mechanics
3.  System Architecture
4.  Hardware Setup
5.  Software Implementation
6.  Getting Started
7.  Limitations and Next Steps

## 📽️ Project Overview

**RGB Rush** challenges players to match a random **Target
Color** by pressing two pressure sensors. Each sensor controls an RGB
LED, producing colors based on pressure intensity. These two colors are
then additively mixed to generate a **Mixing Color**.

When the Mixing Color matches the Target Color for a sustained duration,
the player wins the round and the score updates.

### Key Technical Features

-   **Real-Time OS:** FreeRTOS (CMSIS-V1)
-   **Concurrent Architecture:** Input, Logic, and Display tasks
-   **Inter-Process Communication:** Queues, Mutexes, Semaphores
-   **High-Speed Interrupts:** 500 Hz TIM10 interrupt for flicker-free
    display
-   **Analog Front-End:** Dual ADC channels with voltage-divider
    conditioning

## 🎮 Game Mechanics

### Objective

Match the **Mixing Color** (result of both sensors) to the **Target
Color**.

### Controls

-   **Left Sensor** Controls RGB LED #1
-   **Right Sensor** Controls RGB LED #2

### Pressure → Color

| Pressure Range | Output  |
|----------------|--------------|
| 11–33%         | **Blue**     |
| 34–66%         | **Green**    |
| 67–100%        | **Red**      |

Note that blue starts at 11% since 0-10 is expected noise from the sensor. This ensures that the LEDs are all **OFF** if nobody touches it.

### Color Mixing

Additive color mixing combines the two sensor colors to produce the final mixing color:

| Sensor 1 | Sensor 2 | Mixed Color |
|----------|----------|-------------|
| Blue     | Blue     | **Blue**    |
| Blue     | Green    | **Cyan**    |
| Blue     | Red      | **Magenta** |
| Green    | Blue     | **Cyan**    |
| Green    | Green    | **Green**   |
| Green    | Red      | **Yellow**  |
| Red      | Blue     | **Magenta** |
| Red      | Green    | **Yellow**  |
| Red      | Red      | **Red**     |

### Winning Condition

-   Hold the matching color for ~**0.5 seconds**
-   Score decrements from a set value on the Score Display
-   Game ends when the countdown score reaches **0**

## ⚙️ System Architecture

The software is built around **three primary RTOS tasks** & a **high-priority timer interrupt**.

### 1. Input Task --- "Producer"

-   Polls ADC sensors at 20 Hz
-   Converts raw readings to color states
-   Sends data to `xInputQueue`

### 2. Logic Task --- "Game Brain"

-   Consumes sensor data
-   Computes mixed color
-   Tracks win timer and rounds
-   Uses `xScoreMutex` for safe score updates
-   Signals `xWinSemaphore` when a round is won

### 3. Display Task --- "Renderer"

-   Updates RGB LEDs and 7-segment display
-   Reads score (protected by `xScoreMutex`)
-   Runs animations when `xWinSemaphore` is triggered

### 4. TIM10 ISR --- Multiplexing Interrupt (500 Hz)

-   Drives 3-digit 7-segment through time-division multiplexing
-   Provides flicker-free output

## 🔌 Hardware Setup

### Platform

-   STM32 Nucleo-F446RE
-   Morpho + Arduino headers

### Components

-   2× Force-Sensitive Resistors
-   4× Common-Anode RGB LEDs
-   1× 3-Digit Common-Cathode 7-Segment Display
-   2× 10 kΩ resistors
-   12× 220 Ω resistors

### Pinout

| Component      | Function        | STM32 Pin              | Notes      |
|----------------|-----------------|------------------------|------------|
| Sensor 1       | P1 Input        | PA0                    | ADC1_IN0   |
| Sensor 2       | P2 Input        | PA1                    | ADC2_IN1   |
| Player 1 LED   | R/G/B           | PC0, PC1, PC2          | —          |
| Player 2 LED   | R/G/B           | PB0, PB1, PB2          | —          |
| Mixing LED     | R/G/B           | PB6, PA6, PA7          | —          |
| Target LED     | R/G/B           | PC5, PC6, PC8          | —          |
| 7-Segment Bus  | Segments A–G    | PA10, PB3, PB5, PB4, PB10, PA8, PA9 | Ground Decimal Point |
| Digit Selects  | SCORE/TENS/ONES        | PC7, PC9, PC3          | —          |

## 💻 Software Implementation

### RTOS & IOC Configuration
-   Pins above set as GPIO_Output accordingly
-   SYS:
    -   TIM6 timebase
    -   Serial Wire
    -   Clock Speed: 84 MHz
-   Analog:
    - IN0 (ADC1)
    - IN1 (ADC2)
-   Timer:
    - TIM10
        - Prescaler: 83
        - Counter Period: 1999
-   USART2 Interrupt
-   TIM1, TIM10 Interrupt
-   ADC Interrupt
-   Everything else can be default

### Communication Objects

``` c
QueueHandle_t      xInputQueue;
SemaphoreHandle_t  xWinSemaphore;
SemaphoreHandle_t  xScoreMutex;
```

### Multiplexing ISR

Runs every **2 ms**: 1. Disable all digits\
2. Load next segment pattern\
3. Enable target digit\
4. Increment digit index

## 🚀 Getting Started

1. Download `finalProject.c` from the repository
2. Follow the .IOC and FreeRTOS setup
3. Build the RGB Rush Circuit

### Import

STM32CubeIDE → *Open Projects from File System...*

### Flash the Code

-   Connect Nucleo
-   Press **Run**
OR
- Debug (hammer Icon)
- Use STM32CubeProgrammer
  - Connect Board
  - Click "**Start Programming**"
### Monitor & Debugging

Use **PuTTY** as the terminal to monitor serial logs and debug the application:

- **Baud Rate:** 115200
- **Connection:** Serial COM port (check Device Manager for the correct port)
- **Purpose:** 
  - Monitor real-time game logs
  - Debug sensor readings and color states
  - Track RTOS task execution

## ⚠️ Limitations and Next Steps

### Current Limitations

1. **GPIO Resource Exhaustion:** Direct-drive architecture uses unique GPIO pins for each LED segment, exhausting I/O resources and preventing scalability. Future expansion requires shift registers or I/O expanders via SPI/I2C.

2. **Constrained Color Palette:** Two analog inputs limit color output to seven discrete combinations. Quantizing pressure into ON/OFF states prevents continuous PWM, blocking subtle shades and tertiary colors.

### Future Improvements

- **Scalable I/O:** Implement shift registers (74HC595) or I/O expanders (MCP23017) to manage multiple LEDs through serial communication, enabling expansion without exhausting GPIO pins.

- **Enhanced Color Mixing:** Replace discrete color mapping with continuous PWM control based on analog pressure values. This enables smooth color transitions, intermediate shades, and full RGB color space access.

