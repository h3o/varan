echo 41 > /sys/class/gpio/export
#cat /sys/class/gpio/gpio41/direction
echo out > /sys/class/gpio/gpio41/direction
#cat /sys/class/gpio/gpio41/value
echo 1 > /sys/class/gpio/gpio41/value

