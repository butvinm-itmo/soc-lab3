## Running Synthesis

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

## Getting Metrics for Report

### 1. Resource Utilization (Утилизация ресурсов FPGA)

From synthesis reports:
```bash
cat mat_comparison_baseline/solution/syn/report/mat_compute_csynth.rpt
cat mat_comparison_unroll/solution/syn/report/mat_compute_csynth.rpt
cat mat_comparison_pipeline/solution/syn/report/mat_compute_csynth.rpt
```

Look for **"Utilization Estimates"** section:
- BRAM_18K
- DSP48E
- FF (Flip-Flops)
- LUT

### 2. Computation Time at 100MHz (Время вычисления)

From the same synthesis report, look for **"Performance Estimates"**:
- **Latency (cycles)** - number of clock cycles
- **Computation time** = Latency × Clock Period = Latency × 10ns

Example: If latency = 500 cycles → Time = 500 × 10ns = 5000ns = 5µs

### 3. Time Diagram (Временная диаграмма)

Waveforms are generated during co-simulation in:
```
mat_comparison_*/solution/sim/verilog/*.wdb
```

To view waveforms:
```bash
# Open .wdb file in xsim GUI
$VIVADO_PATH/bin/xsim mat_comparison_baseline/solution/sim/verilog/*.wdb -gui

# Or open Vivado and use File -> Open Waveform
$VIVADO_PATH/bin/vivado
```
