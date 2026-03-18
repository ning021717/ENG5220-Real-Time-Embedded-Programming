#!/bin/bash

echo "========================================"
echo " 🛠️ Camera Hot-Reload Sequence Initiated "
echo "========================================"

# 1. Kill any hung vision processes (silencing errors if none exist)
echo "[1/3] Terminating zombie MainApp processes..."
sudo killall MainApp 2>/dev/null

# 2. Forcefully rip the driver out of the Linux kernel
echo "[2/3] Unloading imx219 kernel module..."
sudo rmmod imx219

# Give the hardware bus 1 second to physically power down and reset
sleep 1

# 3. Inject the driver back into the kernel to force a new handshake
echo "[3/3] Reloading imx219 kernel module..."
sudo modprobe imx219

# Wait for the system to mount the /dev/video nodes
sleep 1

echo "========================================"
echo " ✅ Camera Module Successfully Reset! "
echo " You may now run: libcamerify ./MainApp "
echo "========================================"
