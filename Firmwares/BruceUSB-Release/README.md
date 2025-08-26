# Bruce USB Firmware for LilyGO T-Embed CC1101

## What is this?

This is a complete Bruce firmware package for the LilyGO T-Embed CC1101 device. Bruce is a powerful multi-tool firmware that provides:

- 📡 **RF Tools**: Transmit, receive, and analyze sub-GHz signals using the CC1101 radio
- 📱 **RFID/NFC**: Read, write, and emulate various RFID/NFC tags
- 🔵 **Bluetooth**: BLE spam, device enumeration, and HID attacks
- 📶 **Wi-Fi**: Deauth attacks, evil portals, packet sniffing, and more
- ⌨️ **BadUSB**: Rubber ducky script execution via USB/Bluetooth
- 🔴 **Infrared**: TV remote control, AC control, and IR signal analysis
- 🎵 **Audio**: Text-to-speech, tone generation, and audio recording
- 🖥️ **Display**: Full-featured menu system with themes
- 💾 **Storage**: MicroSD card support for storing captures and scripts

## Hardware Requirements

- **LilyGO T-Embed CC1101** (ESP32-S3 based device with CC1101 radio)
- **USB-C cable** for programming and power
- **MicroSD card** (optional but recommended for full functionality)

## Installation Instructions

### Method 1: Easy Flash (Windows)

1. **Install Python** if you don't have it:
   - Download from https://python.org
   - Make sure to check "Add Python to PATH" during installation

2. **Install esptool**:
   ```
   pip install esptool
   ```

3. **Connect your device**:
   - Connect T-Embed CC1101 to your computer via USB-C
   - Note which COM port it uses (usually COM3, but may vary)

4. **Run the flash script**:
   - Double-click `flash_bruce_firmware.bat`
   - Follow the on-screen instructions
   - If your device is on a different COM port, edit the .bat file and change COM3 to your port

### Method 2: Manual Flash (Cross-platform)

Use esptool directly:

```bash
python -m esptool --chip esp32s3 --port COM3 --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

Replace `COM3` with your device's port (`/dev/ttyUSB0` on Linux, `/dev/cu.usbserial-*` on macOS).

### Method 3: ESP Flash Download Tool (Windows GUI)

1. Download ESP Flash Download Tool from Espressif
2. Select ESP32-S3
3. Load the files at these addresses:
   - `0x0` → `bootloader.bin`
   - `0x8000` → `partitions.bin` 
   - `0x10000` → `firmware.bin`
4. Set SPI Speed: 80MHz, SPI Mode: DIO, Flash Size: 16MB
5. Click START

## First Boot

After flashing:

1. **Restart** your device (press reset button or unplug/replug USB)
2. You should see the **Bruce startup screen**
3. Use the **buttons** on the device to navigate the menu system
4. **Insert a MicroSD card** for full functionality (storing captures, scripts, etc.)

## Usage

- **Navigation**: Use the hardware buttons to navigate menus
- **Serial Console**: Connect at 115200 baud for command-line access
- **Wi-Fi Setup**: Use the Wi-Fi menu to connect to networks
- **File Management**: Use MicroSD card for storing data, scripts, and configurations

## Features Overview

### RF Module (CC1101)
- Signal transmission and reception (300-928 MHz)
- Protocol analysis (OOK, ASK, FSK, GFSK, MSK)
- Frequency spectrum analysis
- Jamming capabilities
- Custom signal recording and replay

### RFID/NFC Module
- Multiple card types: Mifare Classic, Ultralight, NTAG, etc.
- Card cloning and emulation
- Chameleon Ultra integration
- PN532 support

### BadUSB Module
- Rubber Ducky script execution
- Multiple keyboard layouts
- HID device emulation
- Bluetooth BadUSB support

### Wi-Fi Module
- Deauth attacks
- Evil portal creation
- Packet capture and analysis
- Wi-Fi reconnaissance
- WPS attacks

## Troubleshooting

**Device not detected:**
- Check USB cable (data cable, not charge-only)
- Install CH341 or CP210x drivers
- Try different USB port
- Check Device Manager (Windows) or dmesg (Linux)

**Flash fails:**
- Try lower baud rate: change `460800` to `115200`
- Hold BOOT button during flashing
- Try different USB cable or port
- Ensure no other software is using the serial port

**Device boots to blank screen:**
- Press reset button
- Check if MicroSD card is properly inserted
- Try reflashing the firmware

## Building from Source

If you want to build the firmware yourself:

1. Install PlatformIO
2. Clone the repository
3. Run: `python -m platformio run --environment lilygo-t-embed-cc1101 --target upload`

## Firmware Details

- **Version**: Development build
- **Target**: LilyGO T-Embed CC1101 (ESP32-S3)
- **Flash Size**: 16MB
- **PSRAM**: 8MB
- **Framework**: Arduino ESP32 v2.0.17
- **Build Date**: August 27, 2025

## Legal Notice

This firmware is for educational and research purposes only. Users are responsible for complying with local laws and regulations regarding radio frequency transmission, wireless security testing, and device usage.

## Support

For issues and questions:
- Original Bruce project: https://github.com/pr3y/Bruce
- Hardware specs: LilyGO T-Embed CC1101 documentation

---

**Happy Hacking!** 🚀
