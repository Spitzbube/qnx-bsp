# Copyright (c) 2024, Texas Instruments Incorporated
# SPDX License Identifier: MIT

echo "starting usb-asix devnp and enabling qconn"

io-pkt-v6-hc -d asix
if_up -p ax0
ifconfig ax0 up
dhclient -nw ax0

echo "starting qconn.."
sleep 5
devc-pty
waitfor /dev/ptyp0 4
waitfor /dev/socket 4
PATH=$PATH:/usr/bin qconn port=8000
inetd &
