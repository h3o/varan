kill `pidof getty`
/root/uart_prescaler.sh
printf '\x90\x45\x64' >/dev/ttyS0
sleep 1
printf '\x80\x45\x64' >/dev/ttyS0
/root/uart_prescaler.sh
