# 🎮 RGB Rush — Color Mix Pressure Game
An interactive embedded-systems game where players match a randomly generated target color using pressure sensors and RGB LEDs. Built for a STM32-F446RE board using FreeRTOS.

---

## Table of Contents
- [Overview]
- [System Components]
- [Game Flow]
- [Software Architecture]
- [Inter-Task Communication]
- [Hardware Integration]
- [Game Logic Summary]
- [Timing and Performance]
- [Developer Responsibilities]
- [Inter-Developer Communication]
- [Acceptance Criteria]
- [Modular Independence]
- [Repository Structure]

---

## Overview
**RGB Rush** is an interactive embedded system game that challenges users to replicate a randomly generated target color by applying pressure to two sensors. Each sensor maps pressure intensity to LED brightness, creating dynamic color mixing in real-time.

## Key Features
- Real-time RGB mixing using pressure sensors  
- 0–60 second round timer  
- Round counter displayed on 7-segment display  
- LEDs for target color, user color, and mixed result  
- Modular, non-blocking architecture using FreeRTOS  
- Reliable inter-task communication and event handling  

---

## System Components

| Component | Function |
|----------|----------|
| **RGB LEDs (x4)** | Display target color, user intensities, and real-time mix |
| **Pressure Sensors (x2)** | Convert analog pressure → color intensity (light/medium/hard) |
| **7-Segment Display (Timer)** | Displays elapsed time (0–60 seconds) |
| **7-Segment Display (Round Counter)** | Displays the current game round |
| **Microcontroller** | Executes all game logic across FreeRTOS tasks |
| **Serial Terminal** | Provides debugging, game logs, and timer output |

---

## Game Flow

## **Step 1 — Game Start**
- Generate a **random target color** (purple, cyan, yellow, etc.)  
- Display the target LED  
- Timer resets to 0  
- Round display starts at **Round 1**  
- Timer begins counting  

## **Step 2 — User Input**
- User applies pressure to **two sensors**  
- Each sensor controls one primary color channel  
- Pressure intensity determines LED brightness (PWM)  
- Mixed LED updates live as the intensity changes

## **Step 3 — Matching Process**
- Compare current mix vs target continuously  
- Loop until match occurs or **60s timeout**  

## **Step 4 — Game Over / Round Completion**
- If matched → Target LED & Mix LED blink  
- Timer stops  
- Round increments  
- System resets and generates a **new target color**  

---
  
## Main FreeRTOS Tasks

| Task Name | Responsibility | Priority | Outputs |
|----------|----------------|----------|---------|
| **GameControlTask** | Manages game states (start, play, win), handles transitions | High | Round triggers |
| **SensorTask (Gaspar)** | Reads pressure sensors, maps analog input to intensity | Medium | `sensorQueue` |
| **MixTask (Danny)** | Combines intensities, computes mix, checks for match | Medium | `mixQueue` |
| **LEDDisplayTask (Danny)** | Updates all RGB LEDs via PWM | Medium | LED signals |
| **TimerTask (Ethan)** | Tracks 0–60s countdown, emits timeout events | Medium | `timerQueue`, `timerEventQueue` |
| **SevenSegmentTask (Ethan)** | Updates timer + round 7-segment displays | Low | Display output |

---

## Task Communication

## Queues
| Queue | From → To | Purpose |
|--------|-----------|----------|
| `sensorQueue` | Gaspar → Danny | User color intensities |
| `mixQueue` | Danny → GameControl | Mix + match status |
| `timerQueue` | Ethan → SevenSegmentTask | Timer seconds update |
| `roundQueue` | GameControl → Ethan | Round display update |

## Semaphores
| Semaphore | Direction | Purpose |
|-----------|------------|----------|
| `colorMatchSemaphore` | Danny → Ethan/GameControl | Notifies when target color is matched |
| `timerResetSemaphore` | Ethan ↔ GameControl | Resets timer each round |
| `newRoundSemaphore` | GameControl → Gaspar | Resets sensor baseline at new round |

## Mutexes
- `ledMutex` — protect LED GPIO updates  
- `displayMutex` — protect 7-segment display writes  
- `serialMutex` — orderly UART debugging  

---

## Hardware Integration

| Component | Interface | Notes |
|----------|-----------|-------|
| RGB LEDs | PWM-capable GPIOs | Target, mix, and user input LEDs |
| Pressure Sensors | ADC inputs | Map voltage → RGB intensity levels |
| 7-Segment Displays | GPIO / Shift Register | Timer + round counter |
| Serial Terminal | UART | Debug messages and logs |

---

## Game Logic Summary

| Event | System Response |
|-------|------------------|
| **Game Start** | Reset timer, increment round, display new target |
| **User Pressure** | Update LED intensity + mix color in real-time |
| **Color Match** | Blink LEDs, stop timer, signal round completion |
| **Timeout (60s)** | Reset round, optional retry |
| **Round Completion** | Increment round counter, generate new target |

---

## Timing and Performance

| Function | Requirement |
|----------|-------------|
| Sensor sampling | ≥ 20 Hz per sensor |
| LED update latency | ≤ 100 ms |
| Timer accuracy | ± 50 ms/sec |
| Display update | < 100 ms |
| Game loop | Continuous, non-blocking |
| Round transition | ≤ 2 seconds |

---

## Developer Responsibilities

### **Ethan — Timer & 7-Segment Displays**
- Handles: timer counting, timeout events, round display  
- Queues: `timerQueue`, `roundQueue`  
- Semaphores: `timerResetSemaphore`, listens to `colorMatchSemaphore`  
- Mutexes: `displayMutex`  

### **Danny — LED Control & Color Mixing**
- Handles: PWM LED output, color mixing, target comparison  
- Queues: receives `sensorQueue`, sends `mixQueue`  
- Semaphores: `colorMatchSemaphore`  
- Mutexes: `ledMutex`  

### **Gaspar — Pressure Sensors**
- Handles: ADC readings, intensity mapping, sensor reset  
- Queues: sends `sensorQueue`  
- Semaphores: `newRoundSemaphore`  

---

## Inter-Developer Communication

| From → To | Resource | Purpose |
|-----------|-----------|----------|
| Gaspar → Danny | `sensorQueue` | User pressure data |
| Danny → Ethan/GameControl | `colorMatchSemaphore` | Notify about color match |
| Ethan → Danny | `timerEventQueue` | Timeout / reset |
| GameControl → Gaspar | `newRoundSemaphore` | Reset sensor baselines |
| Ethan internal | `timerQueue`, `roundQueue` | Timer + round updates |

---

## Acceptance Criteria
1. System initializes at **Round = 1**  
2. Timer accurately counts **0–60 seconds**  
3. LED brightness reflects real-time pressure  
4. Mix + target LEDs blink on color match  
5. Timer resets and transitions occur within **2 seconds**  
6. All modules communicate reliably with no conflicts  

---

## Modular Independence

- **Ethan:** Can develop timer + display logic using mock events  
- **Danny:** Can develop LED mixing using simulated sensor values  
- **Gaspar:** Can develop pressure sensor reading with potentiometers or simulated input  


