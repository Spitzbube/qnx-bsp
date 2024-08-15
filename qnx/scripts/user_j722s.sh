# Copyright (c) 2024, Texas Instruments Incorporated
# SPDX License Identifier: MIT

echo "user.sh called..."

echo Setting additional environment variables...
export PS1='J722S-EVM@QNX:$(pwd)# '
export PATH=:/proc/boot:/bin:/sbin:/usr/bin:/usr/sbin:/opt/bin:/ti_fs:/ti_fs/bin:/ti_fs/sbin:/ti_fs/usr/bin:/ti_fs/usr/sbin:/ti_fs/tibin:/ti_fs/scripts
export LD_LIBRARY_PATH=:/proc/boot:/lib:/usr/lib:/lib/dll:/opt/lib:/ti_fs/lib:/ti_fs/usr/lib:/ti_fs/lib/dll/mmedia:/ti_fs/lib/dll:/ti_fs/tilib
export OMXIL_COMPONENT_PATH=/ti_fs/tilib

GCOV_ENABLED=/ti_fs/gcov_enabled

if [[ -e "$GCOV_ENABLED" ]]; then
    echo "Gcov setting enabled "
    export GCOV_PREFIX=/ti_fs/
fi

echo "Starting tisci-mgr.."
tisci-mgr
waitfor /dev/tisci 2

echo "Starting shmemallocator.."
shmemallocator

echo "Starting tiudma-mgr.."
tiudma-mgr

echo "Starting tiipc-mgr.."
tiipc-mgr

echo "Starting ti-vpu-codec-mgr"
ti-vpu-codec-mgr

if [[ -e /ti_fs/usr/lib/graphics/j722s/graphics-virtual-dpy.conf ]]; then
echo "Start screen.."
screen -c /ti_fs/usr/lib/graphics/j722s/graphics-virtual-dpy.conf
fi

#echo "Starting sshd"
#/usr/sbin/sshd
