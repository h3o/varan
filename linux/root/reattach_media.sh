umount /dev/mmcblk0p4
echo "" > /sys/kernel/config/usb_gadget/g1/functions/mass_storage.GS0/lun.0/file
sleep 1
echo /dev/mmcblk0p4 > /sys/kernel/config/usb_gadget/g1/functions/mass_storage.GS0/lun.0/file
mount /dev/mmcblk0p4 /media
/root/autolink_tmp.sh &
