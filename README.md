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

## SAADC
- Sampling rate 1 kHz
- Resolution 10 bit
- Uses timer interrupt (31.25 kHz) based continuous sampling

## Two wire interface (TWI)/ I2C Instance 1
- Communicates with MPU 6050 sensor
- Stores 3-axis accelerometer data (gyroscope data can also be saved, if required)
- Communication speed 100 kHz
- Automatically clear bus: True

## TWI/ I2C Instance 2
- Communicates sequentially with MAX30101 and Si7021 sensor respectively
- Stores infrared (IR) and red LED data and temperature-humidity data
- Communication speed 100 kHz
- Automatically clear bus: True

## Important Function definitions
- saadc_init(): Initializes ADC unit
- saadc_callback(): At every SAADC event, converts and save (using DMA) 48 analog samples
- twi_master_init(): Initializes TWI instance 1 for MPU6050 communication
- MPU6050_ReadAcc(&AccValue[0], &AccValue[1], &AccValue[2]): Reads accelerometer data
- max30100_init(): Initializes TWI instance 2 for MAX30101 and Si7021 communication
- max30100_read_sensor_data(&ir_data, &red_data): Reads IR and red LED data
- si7021_read_sensor_data(&temp_data, &humd_data): Reads temperature and humidity data

## BLE Specifications
- Advertising name: EX_CPS_EIT1
- Speed: 1 Mbps
- Baud rate: 115200 kbps
- Advertising interval: 40 ms
- Minimum connection interval: 20 ms
- Maximum connection interval: 50 ms
- Advertising duration: 180 s (goes to sleep after 3 minutes)
- BLE packet size: 116 bytes
- BLE packet organization: ECG samples (96 bytes) | Start byte, Respiration data, End byte (8 bytes) | Start byte, SpO2 data, End byte (6 bytes) | Start byte, Temperature_Humidity data, End byte (6 bytes)
