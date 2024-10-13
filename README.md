# nRF52--Real-time-Ambulatory-Monitoring

# Multi-Sensor Biosignal Acquisition Firmware

## Overview
This repository contains the firmware for a multi-sensor biosignal acquisition system. It utilizes Nordic Semiconductor's nRF52832/nRF52840 chipset for wireless communication via BLE (Bluetooth Low Energy) and data transmission through the Nordic UART Service (NUS). The firmware supports multiple sensors for biosignal monitoring, including:

- **ECG data acquisition** using the SAADC (Successive Approximation Analog-to-Digital Converter)
- **Environmental data** (temperature and humidity) from the Si7021 sensor
- **Motion data** from the MPU6050 accelerometer and gyroscope
- **Pulse oximetry data** from the MAX30100 sensor

The firmware is built using the Nordic nRF5 SDK (version 15.2.0) and integrates BLE functionality with the SoftDevice S132 v6.1.0/S140 v6.1.0.

## Features
- **ECG Signal Processing**: Continuous ECG data acquisition with real-time peak detection and heart rate calculation.
- **Environmental Monitoring**: Acquires temperature and humidity data from the Si7021 sensor.
- **Motion Detection**: Captures accelerometer data using the MPU6050 sensor.
- **Pulse Oximetry**: Measures SpO2 levels and heart rate using the MAX30100 sensor.
- **BLE Communication**: Transmits sensor data over BLE via the Nordic UART Service (NUS) for remote monitoring.

## Setup

### Prerequisites

**Hardware**:
- nRF52832/nRF52840 development board
- Sensors: Si7021 (temperature and humidity), MPU6050 (motion), MAX30100 (pulse oximetry)

**Software**:
- nRF5 SDK v15.2.0
- Segger Embedded Studio (SES) or any compatible IDE for Nordic SDK
- SoftDevice S132 v6.1.0 or S140 v6.1.0 (for BLE)

### Dependencies
- **BLE SDK Components**: Nordic UART Service (NUS), BLE advertising, BLE connection parameters
- **SAADC Module**: For ECG data acquisition
- **TWI (I2C) Interface**: For communicating with Si7021 and MPU6050 sensors
- **Timers and PPI**: For handling timed SAADC sampling
- **nRF Logging**: For debugging and logging over UART or RTT
