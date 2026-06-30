mount /dev/mmcblk0p4 /media
mount /dev/mmcblk0p1 /boot
mount /dev/mmcblk0p3 /media2
modprobe g_acm_ms luns=2 file=/dev/mmcblk0p4,/dev/mmcblk0p3 stall=0 removable=1
setsid getty -L -l /bin/sh -n 115200 /dev/ttyGS0

