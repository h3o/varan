until read -s -n 1 -t 0.01; do 
    devmem 0x01c2280c
    sleep 1
    done 
echo 

#while :
#do
#  devmem 0x01c2280c
#  sleep 1
#done
