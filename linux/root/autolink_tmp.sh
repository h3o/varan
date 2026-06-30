#!/bin/sh

#mount -r -o remount /media
#umount /media
#mount -r /dev/mmcblk0p3 /media

last_compiled="000"

while :
do

res="."$(ls /media/dev/ | grep compiled)

#echo "result = $res"


if [[ $res == "." ]]
then
   echo -e "nothing new compiled\n"
   break
fi


ts=${res:10:19}

#echo $ts

if [[ ${res:1:8} == "compiled" ]] && [[ $last_compiled != $ts ]]; then
        echo -e "New binary compiled at $ts\n"

#       umount /media
#       mount /dev/mmcblk0p3 /media
#       cp -R /media/dev/main.$ts/* /tmp/
        rm /tmp/main
        ln -s /media/dev/main.$ts/main /tmp/main
        last_compiled=$ts

#else
#       echo -e "Not updated.\n"


fi

sleep 1

#read -n 1 -t 0.1 input
#
#if [[ .$input = ".q" ]] || [[ .$input = ".Q" ]]
#then
#   echo # to get a newline after quitting
#   break
#fi

done

echo -e "Done\n"
