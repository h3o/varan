echo 41 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio41/direction
echo 0 > /sys/class/gpio/gpio41/value
