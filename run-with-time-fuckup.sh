#!/bin/bash

sudo timedatectl set-ntp false
sudo date -s "2020-11-27 03:11:00"
source $VIVADO_PATH/settings64.sh
$VIVADO_PATH/bin/vivado_hls -f run_all.tcl
sudo timedatectl set-ntp true
sudo systemctl restart systemd-timesyncd
