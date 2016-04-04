#!/bin/bash

lspci

echo "PCI devices are identified via VendorIDs, DeviceIDs, and Class Codes:"
lspci -v -m -n -s 00:04.0
echo "The VendorID, DeviceID, Class Code, Subsystem VendorID (i.e. SVendor) and SubsystemID (i.e. SDevice) values are embedded in the PCI configuration space:"
hexdump /sys/devices/pci0000\:00/0000\:00\:04.0/config
lspci -v -s 00:04.0
