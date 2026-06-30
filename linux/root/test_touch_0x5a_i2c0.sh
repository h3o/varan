while :
do

b1=$(i2cget -y 0 0x5a 0x00)
b2=$(i2cget -y 0 0x5a 0x01)

#b1=$(($b1))
#b2=$(($b2))

echo $b1 $b2

sleep 0.02
done

