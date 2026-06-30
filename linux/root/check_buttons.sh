#!/bin/sh

  #check if user is holding a button:

  echo 35 > /sys/class/gpio/export	#export GPIOs and set direction
  echo in > /sys/class/gpio/gpio35/direction
  echo 197 > /sys/class/gpio/export
  echo in > /sys/class/gpio/gpio197/direction
 
  devmem 0x01c208f4 32 0x00000540 	#enable pull-ups
  devmem 0x01c20840 32 0x00000054

  r1=$(cat /sys/class/gpio/gpio35/value)
  r2=$(cat /sys/class/gpio/gpio197/value)

  #if [ "$r1" == "0" ]
  if [[ "$r1" == "0" && "$r2" == "0" ]]
  then
  
  #echo 40 >/sys/class/gpio/export
  #echo out >/sys/class/gpio/gpio40/direction

  for i in `seq 1 10`
  do	
    #echo "Blink $i"
    echo 0 >/sys/class/gpio/gpio40/value
    usleep 80000
    echo 1 >/sys/class/gpio/gpio40/value
    usleep 80000
  done

  fi

  echo 35 > /sys/class/gpio/unexport	#unexport GPIOs
  echo 197 > /sys/class/gpio/unexport
