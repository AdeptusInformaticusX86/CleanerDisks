#!/bin/bash

# Identify USB flash drive controller
# Usage: sudo ./identify_controller.sh /dev/sdg

if [ -z "$1" ]; then
    echo "Usage: $0 <device>"
    echo "Example: $0 /dev/sdg"
    exit 1
fi

DEVICE=$1
DEVICE_BASE=$(echo $DEVICE | sed 's/[0-9]*$//')

echo "=== USB Flash Drive Controller Identification ==="
echo

# Get USB IDs
VENDOR_ID=$(udevadm info --query=all --name=$DEVICE | grep "ID_VENDOR_ID=" | cut -d= -f2)
MODEL_ID=$(udevadm info --query=all --name=$DEVICE | grep "ID_MODEL_ID=" | cut -d= -f2)
VENDOR=$(udevadm info --query=all --name=$DEVICE | grep "ID_VENDOR=" | head -1 | cut -d= -f2)
MODEL=$(udevadm info --query=all --name=$DEVICE | grep "ID_MODEL=" | head -1 | cut -d= -f2)

echo "Vendor: $VENDOR"
echo "Model: $MODEL"
echo "Vendor ID: $VENDOR_ID"
echo "Model ID: $MODEL_ID"
echo

# Identify controller based on VID:PID
echo "=== Controller Detection ==="
case "$VENDOR_ID:$MODEL_ID" in
    "0951:1666")
        echo "Controller: Likely Phison PS2251 or similar"
        echo "Tools: Phison MPALL, Phison UP (Windows only)"
        echo "Note: Kingston DataTraveler G3/G4 typically use Phison controllers"
        ;;
    "0951:"*)
        echo "Controller: Kingston device, likely Phison-based"
        echo "Tools: Kingston Format Utility, Phison tools"
        ;;
    *)
        echo "Controller: Unknown or generic"
        echo "VID:PID = $VENDOR_ID:$MODEL_ID"
        ;;
esac
echo

# Check for USB quirks
echo "=== USB Quirks ==="
if [ -f "/sys/block/$(basename $DEVICE_BASE)/device/quirks" ]; then
    QUIRKS=$(cat /sys/block/$(basename $DEVICE_BASE)/device/quirks)
    echo "USB Quirks: $QUIRKS"
else
    echo "No quirks file found"
fi
echo

# Try to read more device info with sg_inq if available
if command -v sg_inq &> /dev/null; then
    echo "=== SCSI Inquiry Data ==="
    sg_inq $DEVICE 2>/dev/null || echo "SCSI inquiry failed"
    echo
fi

# Check for chip info using lsusb
echo "=== Detailed USB Information ==="
if command -v lsusb &> /dev/null; then
    BUS_DEV=$(udevadm info --query=all --name=$DEVICE | grep "BUSNUM\|DEVNUM" | cut -d= -f2 | tr '\n' '/' | sed 's/\//:/1' | sed 's/\/$//')
    if [ -n "$BUS_DEV" ]; then
        echo "USB Bus:Device = $BUS_DEV"
        lsusb -v -s $BUS_DEV 2>/dev/null | head -50
    else
        echo "Bus/Device info not found"
    fi
else
    echo "lsusb not available"
fi
echo

echo "=== Recommended Actions ==="
echo
echo "For Kingston DataTraveler with Phison controller:"
echo "1. Physical check: Look for a write-protect switch on the USB drive"
echo "2. Windows tools (if available):"
echo "   - Kingston Format Utility: https://www.kingston.com/en/support/technical/downloads"
echo "   - Phison MPALL (Mass Production Tool)"
echo "   - USB Disk Storage Format Tool"
echo "3. Linux alternatives:"
echo "   - Try formatting with: sudo mkfs.vfat -F 32 -I $DEVICE"
echo "   - Try dd with force: sudo dd if=/dev/zero of=$DEVICE bs=1M count=1"
echo "4. If all else fails:"
echo "   - Controller may be in read-only failure mode (permanent)"
echo "   - Contact Kingston support if under warranty"
echo
