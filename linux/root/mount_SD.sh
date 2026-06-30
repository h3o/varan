mount /dev/mmcblk0p1 /mnt/SD/
echo "" > /sys/kernel/config/usb_gadget/g1/functions/mass_storage.GS0/lun.0/file
cat /sys/kernel/config/usb_gadget/g1/functions/mass_storage.GS0/lun.0/file
echo /dev/mmcblk0p1 > /sys/kernel/config/usb_gadget/g1/functions/mass_storage.GS0/lun.0/file
cat /sys/kernel/config/usb_gadget/g1/functions/mass_storage.GS0/lun.0/file
