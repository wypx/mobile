#!/bin/sh

echo "init 4G...."
#insmod /dav/usbserial.ko
#insmod /dav/usb_wwan.ko
#insmod /dav/option.ko
mkdir -p /home/peers
mv /home/DIALD /home/peers
#mv /home/chat /bin/
cp chat /sbin/
cp chat /bin/
cp DIALD /home/peers/
cp chat-connect /home/
pppd call pppd_param &