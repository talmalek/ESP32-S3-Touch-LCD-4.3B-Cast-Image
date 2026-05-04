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
3. **Web Server & Cropping Tool**: 
   - Access via `http://<device-ip>`
   - Upload JPG/PNG/BMP
   - **Built-in Cropper**: Precise 800x480 locked aspect ratio crop tool.
4. **Reliable Storage**: 
   - LittleFS for image persistence.
   - Automatic filesystem repair and reformatting if corruption is detected.
5. **Professional UI**:
   - **Centered Status**: Clean "No Image Loaded" screen with large fonts and clear connection instructions.
   - **Settings Menu**: Accessible via gear icon with professional flex-aligned layout.
   - **Navigation**: "Back to Frame" button for intuitive menu control.
6. **Robust Image Upload**:
   - Automatic LVGL task suspension during write to prevent flash/DMA bus contention.
   - Filesystem remounting to ensure data visibility.
7. **Clean Boot (Anti-Flash)**:
   - Synchronized backlight control to prevent the common "white screen" flash during startup.
   - Display remains dark until the first frame is ready to render.

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