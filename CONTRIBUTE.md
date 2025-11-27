LOL Vivado use positive part of the 32-bit integer to represent timestamp and apparently it is not enough to store date after 2020,
so we temporary patch system date to run RTL synthesis.

```bash
sudo timedatectl set-ntp false
sudo date -s "2020-11-27 03:11:00"
source $VIVADO_PATH/settings64.sh
$VIVADO_PATH/bin/vivado_hls -f run_all.tcl
sudo timedatectl set-ntp true
sudo systemctl restart systemd-timesyncd
```

We also deleted built-in Vivado linker ($VIVADO_PATH/bin/ld) because it is not compatible with modern libraries format. Deleting it forces Vivado to use the system linker.
