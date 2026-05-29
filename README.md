![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue)
![License](https://img.shields.io/badge/License-GPLv3-green)
![Status](https://img.shields.io/badge/Status-Working-success)
![Project](https://img.shields.io/badge/Category-Healthcare-red)

# AlzGuard

**AlzGuard** is an intelligent healthcare and safety ecosystem specifically designed for Alzheimer patients. The system combines a custom-designed smartwatch, real-time health monitoring, smart medication reminders, intelligent fall detection, and a caregiver companion device to improve patient safety and independence.

By integrating wearable technology, IoT communication, emergency response mechanisms, and caregiver monitoring, AlzGuard provides a practical solution to address the daily challenges faced by Alzheimer's patients and their families.

---

## Problem Statement

Alzheimer patients frequently experience:

- Memory loss
- Missed medications
- Difficulty following daily routines
- Increased risk of falls
- Delayed emergency response
- Reduced independence

Caregivers often struggle to continuously monitor patients while managing their own responsibilities.

AlzGuard was developed to bridge this gap by creating a wearable healthcare and safety ecosystem that actively assists patients while keeping caregivers informed.

---

## Key Features

### Health Monitoring

- Real-time Heart Rate Monitoring
- Blood Oxygen (SpO₂) Monitoring
- Temperature Monitoring
- Humidity Monitoring
- Battery Status Monitoring

### Safety Features

- Intelligent Fall Detection
- Multi-Stage Motion Analysis
- Emergency Alert Generation
- "I AM SAFE" Confirmation System
- Caregiver Notifications

### Smart Assistance

- Medication Reminders
- Water Intake Reminders
- Meal Reminders
- Exercise Reminders
- Sleep Reminders
- Custom Reminders

### Connectivity

- Bluetooth Low Energy (BLE)
- Wi-Fi Communication
- Mobile Dashboard Integration
- Companion Monitoring Device

### User Experience

- Touchscreen Interface
- Elderly-Friendly UI
- Large Readable Fonts
- Simple Navigation
- Real-Time Dashboard

---

## System Architecture

### Smartwatch Unit

The wearable smartwatch is used directly by the patient and provides:

- Vital Monitoring
- Reminder Notifications
- Fall Detection
- Emergency Alerts
- Wireless Connectivity
- Touchscreen Interaction

### Caregiver Companion Device

The ESP32-S3 Companion Device acts as a monitoring hub and provides:

- Live Health Monitoring
- Emergency Notifications
- Reminder Notifications
- Connectivity Monitoring
- Continuous Caregiver Support

---

## Hardware Components

### Smartwatch

#### Waveshare ESP32-S3 1.69" Touch Display

Main controller responsible for:

- Sensor Processing
- Display Management
- BLE Communication
- Wi-Fi Communication
- User Interface Rendering

#### MAX30100

- Heart Rate Monitoring
- SpO₂ Measurement

#### AHT21B

- Temperature Monitoring
- Humidity Monitoring

#### 3.7V 950mAh Li-Ion Battery

Provides portable operation and wearable usability.

### Companion Device

#### ESP32-S3-BOX-3

Functions:

- Receives Alerts
- Displays Patient Data
- Caregiver Monitoring
- Communication Bridge

## License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0).

You are free to use, modify, and distribute this software under the terms of the GPL-3.0 license. Any derivative work must also be distributed under the same license.

For more information, see the LICENSE file or visit:
https://www.gnu.org/licenses/gpl-3.0.en.html
