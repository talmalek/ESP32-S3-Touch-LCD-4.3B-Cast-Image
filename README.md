# Digital Picture Frame - ESP32-S3-Touch-LCD-4.3B

## Overview
A WiFi-enabled digital picture frame project based on the Waveshare ESP32-S3-Touch-LCD-4.3B display (800x480 resolution). This project features a built-in web-based cropping tool and a clean, professional UI optimized for high-resolution static images.

## Hardware
- **Board**: Waveshare ESP32-S3-Touch-LCD-4.3B
- **Display**: 800x480 RGB LCD (ST7262 controller)
- **Touch**: GT911 capacitive touch (I2C)
- **MCU**: ESP32-S3 (16MB Flash, 8MB PSRAM)
- **IO Expander**: CH422G

## Features
1. **Splash Screen**: Professional UI showing network status and instructions.
2. **WiFi Setup**: WiFiManager portal for easy network configuration.
3. **Web Server & Advanced Client**: 
   - Access via `http://<device-ip>`
   - **Photo Studio**: Upload JPG/PNG/BMP with a built-in 800x480 precise crop tool.
   - **Solid Backgrounds**: Apply flat color backgrounds directly from a color picker.
4. **Persistent Settings**: 
   - Uses `Preferences.h` (NVS) to save your preferences (like Clock ON/OFF) across reboots.
   - LittleFS for image persistence.
5. **Production UI**:
   - **Centered Status**: Clean "No Image Loaded" screen with connection instructions.
   - **Compact Settings**: Redesigned one-page menu for easy control.
   - **24H Flip-Clock**: Premium animated clock overlay with touch pass-through.
6. **Hardware-Level Stability**:
   - **Optimized Timing**: LCD PCLK tuned to 12.5MHz for artifact-free rendering.
   - **Atomic Transitions**: Backlight-managed "clean reveal" during image updates to prevent flickering.
   - **Bus Protection**: Automatic LVGL task suspension during Flash I/O to prevent PSRAM contention.
7. **Clean Boot**:
   - Synchronized backlight control prevents the "white flash" during startup.
   - NTP time synchronization with automatic DST support for Israel.

## Build & Flash

### Requirements
- PlatformIO
- Python (for serial monitoring)

### Build
```bash
pio run -e release
```

### Flash
```bash
pio run -e release -t upload --upload-port /dev/cu.usbmodem101
```

## Project Structure

```
ESP32-S3-Touch-LCD-4.3B - Cast Image/
├── platformio.ini          # PlatformIO configuration
├── lib/
│   ├── ESP32_Display_Panel/  # Display driver
│   ├── ESP32_IO_Expander/   # IO expander driver
│   └── lvgl/               # LVGL v8
└── src/
    ├── main.cpp                # App entry & lifecycle
    ├── lvgl_v8_port.h/cpp      # LVGL integration
    ├── models/
    │   └── storage_manager.h/cpp  # NVS + LittleFS manager
    ├── ui/
    │   ├── splash.h/cpp       # Splash UI
    │   └── frame_ui.h/cpp     # Main UI & Animations
    └── net/
        └── web_server.cpp     # Web UI with Cropper.js integration
```

## How It Works
1. **Initialization**: Mounts storage and initializes the 800x480 display using RGB interface.
2. **Network**: If no WiFi is saved, creates an AP named "PictureFrame" (pass: 12345678).
3. **Display**: Once connected, it shows the local IP and setup instructions.
4. **Upload**: Use any device on the same network to crop and upload your image to the frame.

## Troubleshooting
- **LittleFS Failures**: The system now automatically reboots and reformats the LittleFS partition if a critical corruption occurs.
- **Image Blank**: Ensure you are using the provided web cropper. The firmware expects an 800px wide RGB565 stream.
- **Serial Monitoring**:
  ```bash
  pio device monitor --port /dev/cu.usbmodem101 --baud 115200
  ```

## License
Apache 2.0