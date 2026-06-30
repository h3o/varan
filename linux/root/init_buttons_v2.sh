#PB3 (PWR_BUTTON) -> 1*32+3=35

echo 35 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio35/direction
#cat /sys/class/gpio/gpio35/value

#in v1 prototype #2 pull up is needed
#devmem 0x01C20840 32 0x00040040

#v1: PB5 (center button) -> 1*32+5=37
#PE19 (center button) -> 4*32+19=147

echo 147 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio147/direction
#cat /sys/class/gpio/gpio147/value

#v1: #enable pull-down (keeping previous settings)
#devmem 0x01C20840 32 0x00040840

#v2: looks like no pull-down needed

#PB2 (top left button) -> 1*32+2=34

echo 34 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio34/direction
#cat /sys/class/gpio/gpio34/value

#enable pull-up (keeping previous settings)
#v1: #devmem 0x01C20840 32 0x00040850
devmem 0x01C20840 32 0x00040050

#v1: #PE16 (top middle button) -> 4*32+16=144
#PE23 (top middle button) -> 4*32+23=151

echo 151 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio151/direction
#cat /sys/class/gpio/gpio151/value

#enable pull-up
#devmem 0x01C208B0 32 0x00004000 #PE23 pull-up
devmem 0x01C208B0 32 0x00004080 #PE19 pull-down and PE23 pull-up

#PG1 (top right button) -> 6*32+1=193

echo 193 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio193/direction
#cat /sys/class/gpio/gpio193/value

#enable pull-up
#v1: #devmem 0x01C208F4 32 0x00040004 ???
devmem 0x01C208F4 32 0x00000004
