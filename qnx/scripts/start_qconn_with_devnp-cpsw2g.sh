# Copyright (c) 2024, Texas Instruments Incorporated
# SPDX License Identifier: MIT

echo "starting devnp cpsw2g and enabling qconn"

io-pkt-v6-hc -d cpsw2g
if_up -p am0
ifconfig am0 up
dhclient -nw am0

echo "starting qconn.."
sleep 5
devc-pty
waitfor /dev/ptyp0 4
waitfor /dev/socket 4
PATH=$PATH:/usr/bin qconn port=8000
inetd &
