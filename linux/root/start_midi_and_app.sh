#!/bin/sh
kill `pidof getty`
/root/uart_prescaler.sh
printf '\x90\x45\x64' >/dev/ttyS0
#sleep 1
printf '\x80\x45\x64' >/dev/ttyS0
sleep 1
/root/uart_prescaler.sh
sleep 1
#nice -n -19 /root/app/main > null &


#check if this is the first run:

if ! test -f /data/already_ran; then

  id_str=$(/root/app/main id)
  #echo id_str = $id_str

  sn_hash=${id_str:81:2}${id_str:122:2}
  #echo sn_hash = $sn_hash

  echo $id_str >/media/system/id$sn_hash

  /root/app/main

  touch /data/already_ran
  rm /data/fw_backup/main

  sync

  umount /dev/mmcblk0p4
  echo "" > /sys/kernel/config/usb_gadget/g1/functions/mass_storage.GS0/lun.0/file
  sleep 1
  echo /dev/mmcblk0p4 > /sys/kernel/config/usb_gadget/g1/functions/mass_storage.GS0/lun.0/file
  mount /dev/mmcblk0p4 /media

  #sleep 1
  #umount /dev/mmcblk0p3
  #umount /dev/mmcblk0p4
  #reboot

  exit

fi


while true
do
  nice -n -19 /root/app/main > /data/ls_app.log
  echo Restarting...
done
