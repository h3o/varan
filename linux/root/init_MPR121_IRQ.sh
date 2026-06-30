echo 142 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio142/direction
#enable pull-up
devmem 0x01C208AC 32 0x10000000
