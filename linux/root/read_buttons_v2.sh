#PB3 (PWR_BUTTON) -> 1*32+3=35
cat /sys/class/gpio/gpio35/value

#PE19 (center button) -> 4*32+19=147
cat /sys/class/gpio/gpio147/value

#PE23 (top middle button) -> 4*32+23=151
cat /sys/class/gpio/gpio151/value

#PB2 (top left button) -> 1*32+2=34
cat /sys/class/gpio/gpio34/value

#PG1 (top right button) -> 6*32+1=193
cat /sys/class/gpio/gpio193/value
