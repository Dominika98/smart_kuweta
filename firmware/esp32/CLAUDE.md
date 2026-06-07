# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Smart Kuweta** is an ESP32-CAM firmware for a smart cat litter box monitor. The AI-Thinker ESP32-CAM detects when a cat has used the litter box (duration signaled by STM32 over UART — currently hardcoded), takes a photo with LED ring illumination, uploads it to Firebase Storage, and logs the visit to Firestore.

## Build & Flash

All commands run from `smart-kuweta-esp32/` using ESP-IDF 5.5.x. The IDF environment must be sourced first.

```bash
# Source ESP-IDF (once per shell session)
. $IDF_PATH/export.sh

cd smart-kuweta-esp32

# Build
idf.py build

# Flash + monitor (replace /dev/ttyUSB0 with your port)
idf.py -p /dev/ttyUSB0 flash monitor

# Monitor only
idf.py -p /dev/ttyUSB0 monitor
```

The dev container (`.devcontainer/`) provides a Docker environment with IDF pre-installed. Open in VS Code Dev Containers and IDF will be available automatically.

There are no automated tests — this is bare-metal firmware validated by flashing and observing serial output.

## Architecture

All logic lives in a single file: `main/main.c`. The `app_main()` flow is:

1. NVS flash init → camera init (OV3660, VGA JPEG, DRAM framebuffer) → WS2812 ring init
2. WiFi STA connect (blocks up to 15 s) — auto-reconnects on disconnect
3. Fetch server time from Firestore via HTTP `Date` header (3 retries)
4. Wait 20 s for cat to leave litter box
5. WS2812 ring ON → 1 s delay → capture JPEG with GPIO4 flash → 1 s delay → ring OFF
6. Upload JPEG to Firebase Storage → log visit document to Firestore (PATCH)

**Server time**: no RTC or NTP — time is derived from the `Date` header of a GET to Firestore, parsed with `strptime`, stored as `time_t`.

**Visit document** (`visits/{visit_id}`): fields `startTime`, `endTime` (ISO 8601 timestamps), `duration` (int, seconds), `photoUrl` (Firebase Storage URL), `type` ("unknown" — Flutter app fills this in later).

**Visit ID**: milliseconds since boot (`esp_timer_get_time() / 1000`), not wall-clock time.

**LED ring**: 12× WS2812 on GPIO12, driven by ESP RMT peripheral via `espressif/led_strip`. During photo capture all 12 LEDs are set to `(128, 128, 128)` white.

**UART / STM32 link**: `uart_task` (priority 5, 4 KB stack) runs permanently on UART1, reading newline-terminated bytes one at a time into a line buffer. On `\n` it calls `cJSON_Parse`, checks `event == "visit_end"`, and pushes `duration_s` (`uint32_t`) onto `s_visit_queue` (depth 1). `app_main` blocks on `xQueueReceive(s_visit_queue, ..., portMAX_DELAY)` — nothing Firebase-related executes until STM32 sends the event. `\r` bytes are silently dropped; lines longer than 255 bytes are discarded with a warning.

## Hardware Pinout

AI-Thinker ESP32-CAM:
- Camera: standard OV2640/OV3660 pins (PWDN=32, XCLK=0, SDA=26, SCL=27, D0–D7, VSYNC=25, HREF=23, PCLK=22)
- Flash LED: GPIO4
- WS2812 ring: GPIO12
- UART0 (debug monitor): GPIO1 TX, GPIO3 RX — do not reassign
- UART1 (STM32 link): GPIO15 TX, GPIO14 RX, 115200 8N1

## Managed Components

Declared in `main/idf_component.yml`, auto-fetched by IDF:
- `espressif/esp32-camera` ≥ 2.0.0
- `espressif/led_strip` ≥ 2.0.0

Source is cached under `managed_components/` — do not edit.

## Custom Partition Table

`partitions.csv` allocates a 1.94 MB factory app partition (offset `0x10000`, size `0x1F0000`). Referenced in the root `CMakeLists.txt` via `PARTITION_TABLE_CSV_PATH`.

## Credentials

WiFi SSID/password and Firebase API key are hardcoded as `#define` constants near the top of `main.c`. These must be changed before building for a different environment.
