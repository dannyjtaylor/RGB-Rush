# STM32 FreeRTOS Color Match Game

An embedded real-time reaction game built on the **STM32 Nucleo-F446RE**
platform using **FreeRTOS**.

This project demonstrates modern embedded firmware architecture by
decoupling **sensor acquisition**, **game logic**, and **display
rendering** into concurrent RTOS tasks. It features analog sensor
processing, additive color mixing, and high-speed hardware-driven
multiplexing.

## 📖 Table of Contents

1.  Project Overview\
2.  Game Mechanics\
3.  System Architecture\
4.  Hardware Setup\
5.  Software Implementation\
6.  Getting Started\
7.  License

## 📽️ Project Overview

The **Color Match Game** challenges players to match a random **Target
Color** by pressing two pressure sensors. Each sensor controls an RGB
LED (Player 1 and Player 2), producing colors based on pressure
intensity. These two colors are then additively mixed to generate a
**Mixing Color**.

When the Mixing Color matches the Target Color for a sustained duration,
the player wins the round and the score updates.

### Key Technical Features

-   **Real-Time OS:** FreeRTOS (CMSIS-V1)\
-   **Concurrent Architecture:** Input, Logic, and Display tasks\
-   **Inter-Process Communication:** Queues, Mutexes, Semaphores\
-   **High-Speed Interrupts:** 500 Hz TIM10 interrupt for flicker-free
    display\
-   **Analog Front-End:** Dual ADC channels with voltage-divider
    conditioning

## 🎮 Game Mechanics

### Objective

Match the **Mixing Color** (result of both sensors) to the **Target
Color**.

### Controls

-   **Player 1 (Left Sensor):** Controls RGB LED #1\
-   **Player 2 (Right Sensor):** Controls RGB LED #2

### Pressure → Color Mapping

  Pressure Range   Output Color
  ---------------- --------------
  11--33%          **Blue**
  34--66%          **Green**
  67--100%         **Red**

### Color Mixing

Additive mixing (e.g., Red + Blue → Magenta).

### Winning Condition

-   Hold the matching color for **0.5 seconds**\
-   Score increments on the 7-segment display\
-   Game ends when the countdown score reaches **0**

## ⚙️ System Architecture

The software is built around **three primary RTOS tasks** and a
**high-priority timer interrupt**.

### 1. Input Task --- "Producer"

-   Polls ADC sensors at 20 Hz\
-   Converts raw readings to color states\
-   Sends data to `xInputQueue`

### 2. Logic Task --- "Game Brain"

-   Consumes sensor data\
-   Computes mixed color\
-   Tracks win timer and rounds\
-   Uses `xScoreMutex` for safe score updates\
-   Signals `xWinSemaphore` when a round is won

### 3. Display Task --- "Renderer"

-   Updates RGB LEDs and 7-segment display\
-   Reads score (protected by `xScoreMutex`)\
-   Runs animations when `xWinSemaphore` is triggered

### 4. TIM10 ISR --- Multiplexing Interrupt (500 Hz)

-   Drives 3-digit 7-segment through time-division multiplexing\
-   Provides flicker-free output

## 🔌 Hardware Setup

### Platform

-   STM32 Nucleo-F446RE\
-   Morpho + Arduino headers

### Components

-   2× Force-Sensitive Resistors\
-   4× Common-Anode RGB LEDs\
-   1× 3-Digit Common-Cathode 7-Segment Display\
-   2× 10 kΩ resistors\
-   12× 220 Ω resistors

### Pinout

  ------------------------------------------------------------------------
  Component          Function           STM32 Pin            Notes
  ------------------ ------------------ -------------------- -------------
  Sensor 1           P1 Input           PA0                  ADC1_IN0

  Sensor 2           P2 Input           PA1                  ADC2_IN1

  Player 1 LED       R/G/B              PC0, PC1, PC2        ---

  Player 2 LED       R/G/B              PB0, PB1, PB2        ---

  Mixing LED         R/G/B              PB6, PA6, PA7        ---

  Target LED         R/G/B              PC5, PC6, PC8        ---

  7-Segment Bus      Segments A--G      PA10, PB3, PB5, PB4, ---
                                        PB10, PA8, PA9       

  Digit Selects      D1/D2/D3           PC7, PB9, PB8        ---
  ------------------------------------------------------------------------

## 💻 Software Implementation

### RTOS Configuration

-   TIM6 timebase\
-   Heap: 15,360 bytes\
-   Stack: 512 words

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

### Clone

``` bash
git clone https://github.com/your-username/stm32-freertos-game.git
```

### Import

STM32CubeIDE → *Open Projects from File System...*

### Flash

-   Connect Nucleo\
-   Press **Run**

### Monitor

115200 baud, PuTTY/TeraTerm

## 📄 License

MIT License
