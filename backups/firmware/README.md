# Firmware Backup - V16 STABLE

**Date**: May 4, 2026
**Version**: V16 (Production Ready)

## Key Features in this Build:
1. **Persistence**: NVS storage for the 24H Clock setting (survives reboots).
2. **Display Stability**: 
   - LCD Clock lowered to 14MHz for maximum timing reliability.
   - "Sync & Fix Display" button in settings for manual re-synchronization.
3. **Optimized Transitions**:
   - Black-screen reveal during image updates to prevent ghosting.
   - PSRAM bus protection during Flash-to-Buffer transfers.
4. **UI Refinements**:
   - One-page Settings Menu (all 4 buttons visible without scrolling).
   - "Tap-to-close" settings overlay.
   - Touch pass-through for the Flip-Clock.

## Files
- `PictureFrame_V16_STABLE.bin`: The main application firmware.

## How to Flash
You can use `esptool.py` to flash this binary at offset `0x10000`:
```bash
esptool.py --chip esp32s3 write_flash 0x10000 PictureFrame_V16_STABLE.bin
```
