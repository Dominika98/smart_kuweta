# 🐱 Smart Kuweta — IoT Cat Litter Monitor

> A production-grade IoT system combining **bare-metal FreeRTOS firmware** on STM32, **ESP32-CAM** media pipeline, and a **cross-platform Flutter application** with a real-time Firebase backend.

Designed and built end-to-end as a personal project — from hardware selection and PCB-level wiring, through RTOS task architecture and peripheral driver configuration, to a fully functional mobile application with cloud integration.

---

## 📐 System Architecture

```
┌─────────────────────────────────────────────────────┐
│                   Hardware Layer                     │
│                                                     │
│  STM32WB55 (FreeRTOS)        ESP32-CAM              │
│  ├── PIR HC-SR501            ├── OV2640 Camera      │
│  ├── GPIO interrupt ISR      ├── WS2812 LED Ring     │
│  ├── RTC timestamping        └── WiFi → Firebase    │
│  └── UART → ESP32-CAM                               │
└─────────────────────────────────────────────────────┘
                        │
                        │ WiFi / HTTPS
                        ▼
┌─────────────────────────────────────────────────────┐
│                  Firebase (GCP)                      │
│  ├── Firestore    ← structured visit logs           │
│  ├── Storage      ← photo assets (.jpg)             │
│  └── FCM          ← push notification delivery      │
└─────────────────────────────────────────────────────┘
                        │
          ┌─────────────┴─────────────┐
          ▼                           ▼
   Android App                   iOS App
   (Flutter)                     (Flutter)
```

---

## 🔧 Hardware

| Component | Model | Role |
|---|---|---|
| MCU | STM32WB55RG (P-NUCLEO-WB55) | Main controller, FreeRTOS, motion handling |
| Camera + WiFi | ESP32-CAM (AI-Thinker, OV2640) | Photo capture, cloud upload |
| Motion Sensor | HC-SR501 PIR | Passive infrared cat detection |
| Lighting | WS2812 RGB LED Ring (12x) | Addressable illumination, software-controlled brightness |
| Power | 5V/3A USB-C PSU | Mains powered, stable supply for camera peak current |

### Hardware Design Decisions
- **PIR over ultrasonic** — HC-SR04 operates at 40kHz, within cat hearing range; PIR is completely silent and passive
- **UART as inter-module bus** — STM32WB55 signals ESP32-CAM over UART; ESP32-CAM owns all WiFi/cloud communication, keeping the STM32 firmware lean and deterministic
- **Mains power** — ESP32-CAM draws up to 310mA peak during WiFi transmission; battery supply ruled out for reliability

---

## ⚙️ Firmware — STM32WB55 (FreeRTOS)

### RTOS Task Design

```
┌──────────────┐     GPIO ISR      ┌──────────────┐
│   PIRTask    │ ←── interrupt ─── │   HC-SR501   │
│              │                   └──────────────┘
│  entry/exit  │ ──→ xQueueSend
└──────────────┘         │
                         ▼
                  ┌──────────────┐
                  │  TimerTask   │  ← RTC-based visit duration
                  │              │
                  │  start/stop  │ ──→ xQueueSend
                  └──────────────┘         │
                                           ▼
                                    ┌──────────────┐
                                    │   UARTTask   │ ──→ ESP32-CAM trigger
                                    └──────────────┘
```

- **PIRTask** — handles GPIO interrupt, debounces signal, detects entry/exit events
- **TimerTask** — measures visit duration using RTC peripheral for accuracy independent of system load
- **UARTTask** — serializes visit data and signals ESP32-CAM to capture photo

### Peripheral Configuration
- GPIO input on PB2 with pull-down for PIR HC-SR501 (active-high output)
- RTC for accurate timestamping independent of RTOS tick
- UART for STM32 ↔ ESP32-CAM communication

### Stack
- **RTOS**: FreeRTOS (via STM32CubeMX)
- **HAL**: STM32CubeHAL
- **Toolchain**: arm-none-eabi-gcc, OpenOCD, CLion
- **Language**: C

---

## 📷 Camera Pipeline — ESP32-CAM

The ESP32-CAM operates as a dedicated media and connectivity node:

1. Receives UART trigger from STM32WB55
2. Activates WS2812 ring via single-wire protocol (NeoPixel)
3. Captures frame from OV2640 via DVP interface
4. Uploads JPEG to Firebase Storage over HTTPS
5. Writes structured visit document to Firestore
6. Dispatches FCM push notification to all registered devices

### Stack
- **Framework**: Arduino / ESP-IDF
- **Language**: C++
- **Key libraries**: Firebase ESP Client, Adafruit NeoPixel

---

## 📱 Mobile Application — Flutter

Cross-platform mobile app targeting **Android and iOS** with a shared codebase, backed by Firebase real-time infrastructure.

### Features
- 🏠 **Dashboard** — live-updating feed of visits via Firestore StreamBuilder
- 📋 **Calendar Logs** — visit history with per-day event markers and drill-down
- 📊 **Statistics** — visit frequency, average duration, 24h activity heatmap
- 🖼️ **Visit Detail** — photo, start/end time, duration, visit type classification
- 🔔 **Push Notifications** — FCM-delivered alerts on cat activity
- 🔐 **Authentication** — Google Sign-In via Firebase Auth

### Architecture
```
Flutter App
├── StreamBuilder        ← real-time Firestore listener
├── CatVisit model       ← Firestore document mapping
├── DashboardScreen      ← live visit feed
├── LogsScreen           ← TableCalendar + per-day drill-down
├── StatsScreen          ← fl_chart bar chart, computed metrics
└── VisitDetailScreen    ← photo + structured visit data
```

### Tech Stack
| | |
|---|---|
| Framework | Flutter 3.x (Dart) |
| Backend | Firebase (Firestore, Storage, FCM, Auth) |
| Charts | fl_chart |
| Calendar | table_calendar |
| Platforms | Android, iOS, Web |

---

## 🗂️ Repository Structure

```
smart-kuweta/
├── app/                    # Flutter cross-platform application
│   ├── lib/
│   │   ├── main.dart       # App entry, screens, Firebase integration
│   │   └── firebase_options.dart
│   └── pubspec.yaml
├── firmware/
│   ├── stm32/              # FreeRTOS firmware — STM32WB55
│   └── esp32/              # ESP32-CAM firmware
└── README.md
```

---

## 🚧 Project Status

| Module | Status |
|---|---|
| Flutter app UI | ✅ Complete |
| Firestore real-time integration | ✅ Complete |
| Google Sign-In authentication | ✅ Complete |
| STM32 FreeRTOS project setup | ✅ Complete |
| PIR motion detection (STM32) | ✅ Complete |
| Visit duration timing (STM32) | 🔧 In development |
| UART STM32 → ESP32-CAM | 🔧 In development |
| ESP32-CAM firmware | 🔧 In development |
| Firebase Storage (photos) | 🔧 In development |
| Push notifications (FCM) | 🔧 In development |

---

## 👩‍💻 About

Built end-to-end by a senior embedded developer with full-stack reach — covering hardware selection, RTOS architecture, peripheral driver integration, cloud backend design, and cross-platform mobile development.

**Core competencies demonstrated:**
- FreeRTOS task design, inter-task communication (queues, semaphores)
- STM32 peripheral configuration (GPIO EXTI, UART, RTC)
- ESP32 WiFi + camera pipeline
- Firebase real-time backend (Firestore, Storage, FCM, Auth)
- Flutter cross-platform mobile development (Android + iOS + Web)

---

## 📄 License

MIT
