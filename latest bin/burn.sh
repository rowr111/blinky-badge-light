#!/bin/sh

./esptool --chip esp32s3 --port /dev/ttyUSB4 -b 460800 write_flash 0x0 bootloader/bootloader.bin 0x8000 partition_table/partition-table.bin 0x10000 blink-badge-light.bin
