# nRF52--Real-time-Ambulatory-Monitoring


## Overview
This repository contains the firmware for a multi-sensor biosignal acquisition system. It utilizes Nordic Semiconductor's nRF52840 chipset for wireless communication via BLE (Bluetooth Low Energy) and data transmission through the Nordic UART Service (NUS). The firmware supports multiple sensors for biosignal monitoring, including:

- **ECG data acquisition** using the SAADC (Successive Approximation Analog-to-Digital Converter)
- **Core body temperature and humidity** from the Si7021 sensor
- **Breathing motion data** from the MPU6050 accelerometer and gyroscope
- **Pulse oximetry data** from the MAX30101 sensor

The firmware is built using the Nordic nRF5 SDK (version 17.0.2) and integrates BLE functionality with the SoftDevice S140 v6.1.0.

## Features
- **ECG Signal Processing**: Continuous ECG data acquisition with real-time peak detection and heart rate calculation.
- **Body Temperature and Humidity**: Acquires temperature and humidity data from the Si7021 sensor.
- **Motion Detection**: Captures accelerometer data from breathing movement using the MPU6050 sensor.
- **Pulse Oximetry**: Measures SpO2 levels and heart rate using the MAX30101 sensor.
- **BLE Communication**: Transmits sensor data over BLE via the Nordic UART Service (NUS) for remote monitoring.

## Setup

### Prerequisites

**Hardware**:
- nRF52840 development board
- Custom PCB: analog front end (AFE) using AD8232, MAX30101, MPU6050, Si7021
  
**Software**:
- nRF5 SDK v17.0.2
- VS Code and/or Segger Embedded Studio (SES)
- SoftDevice S140 v6.1.0 (for BLE)

### Dependencies
- **BLE SDK Components**: Nordic UART Service (NUS), BLE advertising, BLE connection parameters
- **SAADC Module**: For ECG data acquisition
- **TWI (I2C) Interface**: For communicating with Si7021, MAX30101, and MPU6050 sensors
- **Timers and Programmable Peripheral Interface (PPI)**: For handling timed SAADC sampling
- **Direct Memory Access (DMA)**: For storing sensor data efficiently
- **nRF Logging**: For debugging and logging over UART or RTT
