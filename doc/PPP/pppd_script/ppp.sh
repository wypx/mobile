#!/bin/sh
# An unforunate wrapper script 
# so that the exit code of pppd may be retrieved


# this is a workaround for issue #651747
#trap "/system/bin/sleep 1;exit 0" TERM


PPPD_PID=

#/system/bin/setprop "net.gprs.ppp-exit" ""

#/system/bin/log -t pppd "Starting pppd"

#/system/xbin/pppd $*
# pppd was put into /system/bin instead of /system/xbin after SDK1.6
/bin/pppd connect 'chat -v -s -r "/var/log/chat.log" -f "/etc/ppp/pppdata_call.conf"' disconnect 'chat -r "/var/log/chat.log" -t 30 -e -v "" +++ATH "NO CARRIER"' /dev/ttyUSB2 115200 mru 1280 mtu 1280 nodetach debug dump defaultroute usepeerdns novj novjccomp noipdefault ipcp-accept-local ipcp-accept-remote connect-delay 5000 linkname ppp0

PPPD_EXIT=$?
PPPD_PID=$!

#/bin/log -t pppd "pppd exited with $PPPD_EXIT"

#/system/bin/setprop "net.gprs.ppp-exit" "$PPPD_EXIT"
