#!/bin/bash

echo "========================================"
echo " Bruce USB Firmware Flasher for T-Embed CC1101"
echo "========================================"
echo
echo "This will flash Bruce firmware to your LilyGO T-Embed CC1101 device."
echo "Make sure your device is connected via USB."
echo

# Detect potential device ports
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PORTS=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null)
    DEFAULT_PORT="/dev/ttyUSB0"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PORTS=$(ls /dev/cu.usbserial-* /dev/cu.usbmodem* 2>/dev/null)
    DEFAULT_PORT=$(echo $PORTS | cut -d' ' -f1)
else
    echo "Unsupported OS. Please use Windows batch file or manual esptool command."
    exit 1
fi

if [ -z "$PORTS" ]; then
    echo "No USB serial devices found. Please check your connection."
    echo "Expected ports: /dev/ttyUSB* (Linux) or /dev/cu.usbserial-* (macOS)"
    exit 1
fi

echo "Detected ports: $PORTS"
echo

# Allow user to specify port or use default
if [ "$1" != "" ]; then
    PORT="$1"
else
    read -p "Enter port [$DEFAULT_PORT]: " PORT
    PORT=${PORT:-$DEFAULT_PORT}
fi

echo "Using port: $PORT"
echo

# Check if esptool is installed
if ! command -v esptool.py &> /dev/null; then
    echo "esptool.py not found. Installing..."
    pip3 install esptool
    if [ $? -ne 0 ]; then
        echo "Failed to install esptool. Please install it manually:"
        echo "pip3 install esptool"
        exit 1
    fi
fi

echo "Detecting device..."
python3 -m esptool --chip esp32s3 --port "$PORT" flash_id
if [ $? -ne 0 ]; then
    echo
    echo "Error: Could not detect device on $PORT."
    echo "Please check:"
    echo "- Device is connected via USB"
    echo "- Correct port (try: ls /dev/tty*)"
    echo "- Device permissions (try: sudo chmod 666 $PORT)"
    echo "- Device drivers are installed"
    echo
    exit 1
fi

echo
echo "Flashing firmware..."
python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin

if [ $? -eq 0 ]; then
    echo
    echo "========================================"
    echo " SUCCESS! Bruce firmware has been flashed!"
    echo "========================================"
    echo
    echo "Your T-Embed CC1101 should now boot with Bruce firmware."
    echo "The device will restart automatically."
    echo
else
    echo
    echo "========================================"
    echo " ERROR: Flashing failed!"
    echo "========================================"
    echo
    echo "Please check your connection and try again."
    echo "You can also try with sudo or a lower baud rate (115200)."
fi
