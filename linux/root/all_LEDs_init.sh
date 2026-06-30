#PE12 (POWER_SW_SIGNAL = Red LED) -> 4*32+12=140

echo 140 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio140/direction
echo 1 > /sys/class/gpio/gpio140/value

#PF6 (USB_ID = Yellow LED) -> 5*32+6=166

echo 166 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio166/direction

#PB4 (LED_START = Green LED) -> 1*32+4=36

echo 36 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio36/direction

#PG0 (LED_STOP = Orange LED) -> 6*32+0=192

echo 192 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio192/direction

#PE22 (SIG_LED = Blue LED) -> 4*32+22=150

echo 150 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio150/direction

#PB5 (PWM1, central LED, Orange) -> 1*32+5=37

echo 37 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio37/direction

#PE16 (12 o'clock outer Blue) -> 4*32+16=144

echo 144 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio144/direction

#PE17 (12 o'clock inner Green) -> 4*32+17=145

echo 145 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio145/direction

#PE0 (11 o'clock outer Blue) -> 4*32+0=128

echo 128 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio128/direction

#PE5 (11 o'clock inner Green) -> 4*32+5=133

echo 133 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio133/direction

#PB1 (10 o'clock outer Blue) -> 1*32+1=33

echo 33 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio33/direction

#PE1 (10 o'clock inner Green) -> 4*32+1=129

echo 129 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio129/direction

#PB0 (8 o'clock outer Blue) -> 1*32+0=32

echo 32 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio32/direction

#PE2 (8 o'clock inner Green) -> 4*32+2=130

echo 130 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio130/direction

#PE3 (7 o'clock outer Blue) -> 4*32+3=131

echo 131 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio131/direction

#PE4 (7 o'clock inner Green) -> 4*32+4=132

echo 132 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio132/direction

#PE7 (5 o'clock outer Blue) -> 4*32+7=135

echo 135 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio135/direction

#PE6 (5 o'clock inner Green) -> 4*32+6=134

echo 134 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio134/direction

#PE9 (4 o'clock outer Blue) -> 4*32+9=137

echo 137 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio137/direction

#PE8 (4 o'clock inner Green) -> 4*32+8=136

echo 136 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio136/direction

#PG3 (2 o'clock outer Blue) -> 6*32+3=195

echo 195 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio195/direction

#PE10 (2 o'clock inner Green) -> 4*32+10=138

echo 138 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio138/direction

#PG4 (1 o'clock outer Blue) -> 6*32+4=196

echo 196 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio196/direction

#PE15 (1 o'clock inner Green) -> 4*32+15=143

echo 143 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio143/direction
