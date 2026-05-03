# Project Configuration Summary: Cast Image

## Board: ESP32-S3-Touch-LCD-4.3B

### Hardware Specs
- **Display**: 800x480 RGB LCD (ST7262)
- **Touch**: GT911 (I2C)
- **MCU**: ESP32-S3 (16MB Flash, 8MB PSRAM)
- **IO Expander**: CH422G (Controls backlight)

### Core Components

**1. main.cpp**
- Entry point initializing storage, hardware, and UI.
- Manages WiFi lifecycle: attempts auto-connect, then falls back to WiFiManager AP.

**2. web_server.cpp**
- Serves an interactive HTML/JS UI with **Cropper.js**.
- Converts uploaded images to raw RGB565 Little Endian on the client side.
- **Upload Safety**: Suspend LVGL task (`lvgl_port_stop`) before flash writes to avoid DMA bus contention.

**3. storage_manager.cpp**
- Handles NVS (WiFi credentials) and LittleFS (Image data).
- **Self-Healing**: Reformats LittleFS automatically on mount failure (`LittleFS.begin(true)`).
- **Remount Logic**: Performs `LittleFS.end()`/`begin()` cycle before reading images to ensure data consistency between tasks.

**4. frame_ui.cpp**
- Renders the image from PSRAM using an `lv_img_dsc_t`.
- **Dynamic Scroll**: Detects images taller than 480px and applies an `lv_anim` vertical bouncing animation.
- **Thread Safety**: Uses `lvgl_port_lock()`/`unlock()` for all UI manipulations.

### Build Configuration (platformio.ini)
- **Board**: `esp32-s3-devkitc1-n16r8`
- **Partition Scheme**: `large_spiffs_16MB.csv` (customized for 16MB)
- **PSRAM**: Enabled with `qio_opi` memory type.

### Flash & Test Commands
```bash
# Upload Firmware
pio run -e release -t upload --upload-port /dev/cu.usbmodem101

# Clean Monitor
pio device monitor --port /dev/cu.usbmodem101 --baud 115200 --filter direct
```