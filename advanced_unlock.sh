#!/bin/bash

# Advanced USB unlock script for stubborn write-protected drives
# Usage: sudo ./advanced_unlock.sh /dev/sdg

if [ "$EUID" -ne 0 ]; then
   echo "This script requires root privileges (sudo)"
   exit 1
fi

if [ -z "$1" ]; then
    echo "Usage: sudo $0 <device>"
    echo "Example: sudo $0 /dev/sdg"
    exit 1
fi

DEVICE=$1
DEVICE_BASE=$(echo $DEVICE | sed 's/[0-9]*$//')

echo "=== Advanced Unlock Methods for $DEVICE ==="
echo
echo "WARNING: These methods may cause data loss!"
echo "Press Enter to continue or Ctrl+C to cancel..."
read

# Check if sg3_utils is installed
if ! command -v sg_scan &> /dev/null; then
    echo "Installing sg3-utils (required for low-level commands)..."
    apt-get update -qq && apt-get install -y sg3-utils 2>&1 | grep -v "^Reading\|^Building"
fi

# Method 1: SCSI mode page modification
echo "=== Method 1: SCSI Mode Page (Write Protect) ==="
if command -v sg_wr_mode &> /dev/null; then
    echo "Attempting to clear write-protect via SCSI mode page..."
    # Try to disable write-protect using mode page
    sg_wr_mode --page=0 --mask=0,0,0,0x80 --clear=0,0,0,0x80 $DEVICE 2>&1

    # Verify
    RO=$(blockdev --getro $DEVICE)
    if [ "$RO" = "0" ]; then
        echo "✅ SUCCESS via SCSI mode page!"
        exit 0
    fi
else
    echo "⚠️  sg_wr_mode not available"
fi
echo

# Method 2: SCSI START STOP UNIT command
echo "=== Method 2: SCSI Start/Stop Unit ==="
if command -v sg_start &> /dev/null; then
    echo "Stopping device..."
    sg_start --stop $DEVICE 2>&1
    sleep 2
    echo "Starting device..."
    sg_start --start $DEVICE 2>&1
    sleep 2

    # Verify
    RO=$(blockdev --getro $DEVICE)
    if [ "$RO" = "0" ]; then
        echo "✅ SUCCESS via Start/Stop!"
        exit 0
    fi
else
    echo "⚠️  sg_start not available"
fi
echo

# Method 3: USB Mass Storage reset via sysfs
echo "=== Method 3: USB Mass Storage Reset ==="
# Find the USB device path
USB_DEV=$(udevadm info --query=path --name=$DEVICE | sed 's|/block/.*||')
if [ -n "$USB_DEV" ]; then
    SCSI_DEV=$(echo $USB_DEV | grep -o "[0-9]*:[0-9]*:[0-9]*:[0-9]*")
    if [ -n "$SCSI_DEV" ]; then
        echo "Deleting SCSI device..."
        echo 1 > /sys/block/$(basename $DEVICE_BASE)/device/delete 2>/dev/null
        sleep 2

        # Rescan SCSI bus
        HOST=$(echo $SCSI_DEV | cut -d: -f1)
        echo "Rescanning SCSI host $HOST..."
        echo "- - -" > /sys/class/scsi_host/host$HOST/scan 2>/dev/null
        sleep 3

        # Check if device is back
        if [ -b "$DEVICE" ]; then
            RO=$(blockdev --getro $DEVICE)
            if [ "$RO" = "0" ]; then
                echo "✅ SUCCESS via SCSI rescan!"
                exit 0
            fi
        else
            echo "⚠️  Device not found after rescan, unplug and replug the USB drive"
            exit 1
        fi
    fi
else
    echo "⚠️  USB device path not found"
fi
echo

# Method 4: Try to remove and re-add the device via USB
echo "=== Method 4: USB Unbind/Rebind ==="
USB_PATH=$(udevadm info --query=path --name=$DEVICE)
USB_DRIVER_PATH=$(dirname $USB_PATH | sed 's|/block.*||')

if [ -d "/sys$USB_DRIVER_PATH" ]; then
    USB_DEVICE=$(basename $USB_DRIVER_PATH)
    DRIVER_PATH="/sys$USB_DRIVER_PATH/driver"

    if [ -d "$DRIVER_PATH" ]; then
        DRIVER=$(basename $(readlink -f $DRIVER_PATH))
        echo "Unbinding from driver: $DRIVER"
        echo "$USB_DEVICE" > /sys/bus/usb/drivers/$DRIVER/unbind 2>/dev/null
        sleep 2

        echo "Rebinding to driver: $DRIVER"
        echo "$USB_DEVICE" > /sys/bus/usb/drivers/$DRIVER/bind 2>/dev/null
        sleep 3

        # Check if it worked
        if [ -b "$DEVICE" ]; then
            RO=$(blockdev --getro $DEVICE)
            if [ "$RO" = "0" ]; then
                echo "✅ SUCCESS via USB rebind!"
                exit 0
            fi
        fi
    fi
else
    echo "⚠️  USB driver path not found"
fi
echo

# Method 5: Direct attribute manipulation
echo "=== Method 5: Direct Sysfs Attribute ==="
if [ -f "/sys/block/$(basename $DEVICE_BASE)/ro" ]; then
    echo "Attempting to write directly to sysfs ro attribute..."
    echo 0 > /sys/block/$(basename $DEVICE_BASE)/ro 2>&1

    RO=$(cat /sys/block/$(basename $DEVICE_BASE)/ro)
    if [ "$RO" = "0" ]; then
        echo "✅ SUCCESS via sysfs!"
        exit 0
    fi
else
    echo "⚠️  Sysfs ro attribute not found"
fi
echo

# Final check
echo "=== Final Status ==="
RO=$(blockdev --getro $DEVICE 2>/dev/null || echo "1")
if [ "$RO" = "0" ]; then
    echo "✅ Device is now unlocked!"
    echo
    echo "Testing write capability..."
    if dd if=/dev/zero of=$DEVICE bs=512 count=1 conv=notrunc 2>/dev/null; then
        echo "✅ Write test successful!"
    else
        echo "❌ Write test failed - device may still be protected"
    fi
else
    echo "❌ All advanced methods failed"
    echo
    echo "=== Diagnosis ==="
    echo "Your Kingston DataTraveler appears to have:"
    echo "  • Firmware-level write protection, OR"
    echo "  • Hardware defect in the controller, OR"
    echo "  • Physical write-protect switch (check the USB drive)"
    echo
    echo "=== Possible Next Steps ==="
    echo "1. Check for a physical write-protect switch on the USB drive"
    echo "2. Try Kingston Format Utility (Windows-only): https://www.kingston.com/en/support/technical/downloads"
    echo "3. Try USB Mass Storage Format Tool"
    echo "4. Search for Phison controller tools (many Kingston drives use Phison chips)"
    echo "   - Tools like 'Phison MPALL' or 'Phison UP' may work"
    echo "5. If under warranty, contact Kingston support"
    echo "6. Last resort: The controller may be permanently failed"
fi
