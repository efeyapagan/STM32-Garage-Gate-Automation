# STM32-Garage-Gate-Automation
# Garage Gate Automation System (STM32 Nucleo-64)

This project implements an automated garage gate control system using Mbed OS on an STM32 Nucleo-64 (NUCLEO-C031C6) development board. The system detects vehicle entry and exit using ultrasonic sensors and controls a servo motor to open and close the gate accordingly.

---

## Hardware Platform

This project was implemented and tested using:

- Development Board: STM32 Nucleo-64 (NUCLEO-C031C6)
- Microcontroller: STM32C031C6
- 2x Ultrasonic distance sensors (HC-SR04 compatible)
- 1x Servo motor (50 Hz PWM controlled)
- External power supply for stable servo operation (recommended)

---

## Software Environment

- Mbed OS
- C++
- PWM control via Mbed abstraction layer
- Timer-based pulse measurement for ultrasonic sensing

---

## System Logic

1. The entrance ultrasonic sensor continuously measures distance.
2. If a vehicle is detected within 30 cm, the gate opens (servo rotates from 0° to 180°).
3. The system waits for the exit sensor to detect the vehicle.
4. After confirmation and a short delay, the gate closes smoothly (180° to 0°).

---

## Features

- Dual ultrasonic sensor validation (entry + exit)
- Timeout-protected echo measurement (prevents system lock-up)
- Smooth servo sweep motion
- Configurable detection threshold
- Clean modular embedded structure

---

## Pin Configuration

- Entrance Trigger: D6  
- Entrance Echo: D7  
- Exit Trigger: D8  
- Exit Echo: D10  
- Servo PWM Output: D9  

---

## Notes

- Servo operates at 50 Hz (20 ms period).
- Distance is calculated from echo pulse duration using speed-of-sound approximation.
- Designed for educational and embedded systems experimentation purposes.
