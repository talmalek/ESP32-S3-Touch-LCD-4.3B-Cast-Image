# Firmware Backup - V17 STABLE

**Date**: May 4, 2026
**Version**: V17 (Final Production)

## Key Features in this Build:
1. **Solid Color Support**: Web client now supports applying flat background colors.
2. **Persistence**: NVS storage for the 24H Clock setting.
3. **Maximum Stability**: 
   - LCD Clock optimized to 12MHz for artifact-free rendering.
   - Atomic "clean reveal" loading sequence.
4. **Clean UI**:
   - Compact Settings Menu (3 core buttons).
   - Touch pass-through for the Flip-Clock.
   - Tap-to-close overlay.

## Files
- `PictureFrame_V17_FINAL.bin`: The main application firmware.

## How to Flash
You can use `esptool.py` to flash this binary at offset `0x10000`:
```bash
esptool.py --chip esp32s3 write_flash 0x10000 PictureFrame_V17_FINAL.bin
```
