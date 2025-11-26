# RGB Rush - Color Mix Pressure Game

RGB Rush is an interactive embedded-systems game where players recreate a randomly generated target color using pressure sensors and RGB LEDs. The system runs on a STM32-F446RE board using FreeRTOS.

---

## Overview
The game generates a target color and challenges the player to match it by applying pressure to two sensors. Each sensor controls the brightness of specific LED channels, allowing real-time color mixing. The game uses a timer, a round counter, and multiple visual indicators to guide the player.

### Features
- Real-time RGB color mixing  
- 0–60 second timer  
- Round counter display  
- LEDs for target, user input, and color mix  
- FreeRTOS task-based architecture  

---

## System Components

| Component | Purpose |
|----------|---------|
| RGB LEDs | Display target and mixed colors |
| Pressure Sensors | Map pressure → LED intensity |
| 7-Segment Displays | Show timer and round number |
| Microcontroller | Runs game logic and tasks |
| Serial Terminal | Debugging and logs |

---

## How the Game Works

### 1. Game Start
- Generate a random target color  
- Display on target LED  
- Timer resets and starts  
- Round counter shows current round  

### 2. User Input
- Player presses two sensors  
- Pressure intensity controls LED brightness  
- Mixed LED updates live  

### 3. Matching Process
- Mixed color is compared against target  
- Stops when matched or timer expires  

### 4. Round Completion
- LEDs blink on correct match  
- Timer stops  
- Round increments  
- New target color is generated  

---

## Tasks Structure

The system uses multiple FreeRTOS tasks:

- **Game Control Task** — Manages game flow and rounds  
- **Sensor Task** — Reads pressure sensors  
- **Mix Task** — Calculates color mix and checks for match  
- **LED Task** — Updates RGB LED outputs  
- **Timer Task** — Handles countdown (0–60s)  
- **7-Segment Task** — Updates timer and round displays  

Tasks communicate using **queues**, **semaphores**, and **mutexes**.

---

## Communication Overview

| Mechanism | Purpose |
|-----------|---------|
| Queues | Send sensor data, mix information, timer values |
| Semaphores | Signal events such as “color matched” or “new round” |
| Mutexes | Prevent display or LED conflicts |

---

## Hardware Integration

| Component | Interface |
|----------|-----------|
| RGB LEDs | PWM GPIO |
| Pressure Sensors | ADC input |
| 7-Segment Displays | GPIO / Shift Register |
| Serial Terminal | UART |

---

## Acceptance Criteria

To be considered complete, the system must:

1. Start at Round 1 successfully  
2. Count time accurately from 0–60 seconds  
3. Update LED colors based on user pressure  
4. Blink LEDs when target color is matched  
5. Transition between rounds within 2 seconds  
6. Communicate reliably between all tasks 
