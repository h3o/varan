devmem 0x01c2280c
devmem 0x01c22800
devmem 0x01c22800 32 0x02000141
devmem 0x01c22800
echo 193 > /sys/class/gpio/export
cat /sys/class/gpio/gpio193/direction
cat /sys/class/gpio/gpio193/value
echo out > /sys/class/gpio/gpio193/direction
cat /sys/class/gpio/gpio193/direction
cat /sys/class/gpio/gpio193/value
devmem 0x01c22800
devmem 0x01c2280c
echo 1 > /sys/class/gpio/gpio193/value
devmem 0x01c2280c
