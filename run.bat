@echo off
title "Run QEMU with Kernel"

REM Create a test disk image if it doesn't exist (10MB)
if not exist test_disk.img (
    echo Creating 10MB test disk image...
    Z:/qemu/qemu-img.exe create -f raw test_disk.img 10M
)

REM Run QEMU with kernel and disk
Z:/qemu/qemu-system-i386.exe -kernel kernel -hda test_disk.img
pause
