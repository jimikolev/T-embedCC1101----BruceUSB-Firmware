@echo off
echo ========================================
echo  Bruce USB Firmware Flasher for T-Embed CC1101
echo ========================================
echo.
echo This will flash Bruce firmware to your LilyGO T-Embed CC1101 device.
echo Make sure your device is connected via USB.
echo.
pause

echo Detecting device...
python -m esptool --chip esp32s3 --port COM3 flash_id
if %errorlevel% neq 0 (
    echo.
    echo Error: Could not detect device on COM3.
    echo Please check:
    echo - Device is connected via USB
    echo - Correct COM port (you may need to change COM3 to your port)
    echo - Device drivers are installed
    echo.
    pause
    exit /b 1
)

echo.
echo Flashing firmware...
python -m esptool --chip esp32s3 --port COM3 --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin

if %errorlevel% eq 0 (
    echo.
    echo ========================================
    echo  SUCCESS! Bruce firmware has been flashed!
    echo ========================================
    echo.
    echo Your T-Embed CC1101 should now boot with Bruce firmware.
    echo The device will restart automatically.
    echo.
) else (
    echo.
    echo ========================================
    echo  ERROR: Flashing failed!
    echo ========================================
    echo.
    echo Please check your connection and try again.
)

pause
