#!/bin/bash

# Diagnostic script for write-protected USB drives
# Usage: sudo ./diagnose_usb.sh /dev/sdg

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

echo "=== Diagnostics for $DEVICE ==="
echo

# 1. Check device exists
if [ ! -b "$DEVICE" ]; then
    echo "❌ Error: $DEVICE does not exist or is not a block device"
    exit 1
fi
echo "✓ Device exists"

# 2. Basic information
echo
echo "=== Device Information ==="
lsblk -o NAME,SIZE,RO,TYPE,FSTYPE,MOUNTPOINT $DEVICE 2>/dev/null || lsblk $DEVICE
echo

# 3. Check read-only status
RO_STATUS=$(blockdev --getro $DEVICE)
echo "Read-only status (blockdev): $RO_STATUS"
if [ -f "/sys/block/$(basename $DEVICE_BASE)/ro" ]; then
    SYS_RO=$(cat /sys/block/$(basename $DEVICE_BASE)/ro)
    echo "Read-only status (sysfs): $SYS_RO"
fi
echo

# 4. USB information
echo "=== USB Information ==="
udevadm info --query=all --name=$DEVICE | grep -E "(ID_MODEL|ID_VENDOR|ID_SERIAL|ID_BUS)" || echo "No USB information available"
echo

# 5. Check physical write-protect switch
echo "=== Physical Write-Protect Switch Check ==="
if [ -f "/sys/block/$(basename $DEVICE_BASE)/device/wp" ]; then
    WP=$(cat /sys/block/$(basename $DEVICE_BASE)/device/wp 2>/dev/null)
    if [ "$WP" = "1" ]; then
        echo "⚠️  PHYSICAL SWITCH DETECTED: The device has hardware write-protection enabled!"
        echo "   Check if there is a physical switch on your USB drive."
    else
        echo "✓ No physical switch detected"
    fi
else
    echo "ℹ  Unable to check physical switch (not supported)"
fi
echo

# 6. Software unlock attempts
echo "=== Attempt 1: BLKROSET unlock ==="
hdparm -r0 $DEVICE 2>&1
echo "hdparm result: $?"

echo
echo "=== Attempt 2: blockdev --setrw ==="
blockdev --setrw $DEVICE 2>&1
echo "blockdev result: $?"

echo
echo "=== Attempt 3: USB Reset ==="
# Find USB device and reset it
USB_PATH=$(udevadm info --query=path --name=$DEVICE | sed 's|/block/.*||')
if [ -n "$USB_PATH" ]; then
    if [ -f "/sys$USB_PATH/authorized" ]; then
        echo "Unauthorizing device..."
        echo 0 > /sys$USB_PATH/authorized
        sleep 1
        echo "Re-authorizing device..."
        echo 1 > /sys$USB_PATH/authorized
        sleep 2
        echo "✓ USB device reset"
    else
        echo "⚠️  Unable to reset USB device"
    fi
else
    echo "⚠️  USB path not found"
fi
echo

# 7. Check final status
echo "=== Final Status ==="
RO_FINAL=$(blockdev --getro $DEVICE)
if [ "$RO_FINAL" = "0" ]; then
    echo "✅ SUCCESS: Device is now read-write!"
else
    echo "❌ FAILURE: Device is still read-only"
    echo
    echo "=== Possible Solutions ==="
    echo "1. Check if there is a physical write-protect switch on the USB drive"
    echo "2. Protection may be firmware-level (requires specialized tools)"
    echo "3. USB controller may be defective"
    echo "4. Some Kingston drives require manufacturer-specific tools"
    echo
    echo "For Kingston DataTraveler, try:"
    echo "  - Kingston Format Utility"
    echo "  - USB Disk Storage Format Tool"
fi
echo

# 8. Write test (if unlocked)
if [ "$RO_FINAL" = "0" ]; then
    echo "=== Write Test ==="
    echo "Attempting to write one byte at the beginning of the disk..."
    if dd if=/dev/zero of=$DEVICE bs=1 count=1 seek=0 conv=notrunc 2>/dev/null; then
        echo "✅ Write successful!"
    else
        echo "❌ Write failed (possible firmware protection)"
    fi
fi
