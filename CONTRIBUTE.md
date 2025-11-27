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

### 4. IP Core Interface (Интерфейс IP-ядра)

Interface ports are listed in the synthesis report under **"Interface"** section:

```bash
cat mat_comparison_baseline/solution/syn/report/mat_compute_csynth.rpt | grep -A 30 "== Interface"
```

### 5. AXI-Lite Address Map (Карта адресов)

The register map is auto-generated in the driver header file:

```bash
cat mat_comparison_baseline/solution/impl/misc/drivers/mat_compute_v1_0/src/xmat_compute_hw.h
```

This file contains:

- Control register addresses (ap_start, ap_done, ap_idle)
- Memory addresses for matrices A, B, C
- Data packing format (4 bytes per 32-bit word)

### 6. IP Core Block Diagram

After `export_design`, the IP can be viewed in Vivado IP Catalog:

```bash
$VIVADO_PATH/bin/vivado
# Then: IP Catalog -> User Repository -> mat_compute
```

Or view the generated Verilog directly:

```bash
cat mat_comparison_baseline/solution/impl/verilog/mat_compute.v
```

### 7. Loop Structure and Optimization Details

Detailed loop analysis is in the synthesis report under **"Performance Estimates -> Detail -> Loop"**:

```bash
cat mat_comparison_baseline/solution/syn/report/mat_compute_csynth.rpt | grep -A 20 "Loop:"
```

## Glossary (Глоссарий терминов)

### Resource Metrics (Ресурсы FPGA)

| Термин       | Описание                                                                                                                                |
| ------------ | --------------------------------------------------------------------------------------------------------------------------------------- |
| **BRAM_18K** | Block RAM 18Kbit - встроенная память FPGA. Используется для хранения массивов, буферов. Один блок = 18 Кбит = 2048 байт                 |
| **DSP48E**   | Digital Signal Processing slice - аппаратный блок для умножения и сложения. Один DSP48E выполняет операцию `A*B+C` за 1 такт            |
| **FF**       | Flip-Flop (триггер) - базовый элемент хранения 1 бита. Используется для регистров, конвейерных стадий, FSM                              |
| **LUT**      | Look-Up Table - таблица истинности для реализации комбинационной логики. 6-входовая LUT может реализовать любую функцию от 6 переменных |
| **URAM**     | UltraRAM - большие блоки памяти (288 Кбит) в новых FPGA. В Artix-7 отсутствует                                                          |

### Timing Metrics (Временные характеристики)

| Термин              | Описание                                                                                                                      |
| ------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| **Target Clock**    | Целевой период тактового сигнала (задаётся в TCL: `create_clock -period 10`). 10 ns = 100 MHz                                 |
| **Estimated Clock** | Оценка реального периода после синтеза. Если > Target, timing constraints не выполнены                                        |
| **Uncertainty**     | Запас на джиттер и вариации. HLS вычитает его из Target при планировании                                                      |
| **Latency**         | Задержка в тактах от начала до завершения вычисления. Latency × Clock Period = время в секундах                               |
| **Interval (II)**   | Initiation Interval - через сколько тактов можно начать следующую операцию. II=1 означает максимальную пропускную способность |

### Loop Metrics (Характеристики циклов)

| Термин                | Описание                                                                  |
| --------------------- | ------------------------------------------------------------------------- |
| **Trip Count**        | Количество итераций цикла (для `for(i=0; i<7; i++)` Trip Count = 7)       |
| **Iteration Latency** | Задержка одной итерации цикла в тактах                                    |
| **Loop Latency**      | Общая задержка цикла = Iteration Latency × Trip Count (без pipeline)      |
| **Pipelined**         | Конвейеризирован ли цикл. Yes = итерации перекрываются во времени         |
| **II achieved**       | Реально достигнутый Initiation Interval для pipelined цикла               |
| **II target**         | Целевой II (обычно 1). Если achieved > target, есть зависимости по данным |

### HLS Directives (Директивы оптимизации)

| Директива               | Эффект                                                                                                            |
| ----------------------- | ----------------------------------------------------------------------------------------------------------------- |
| `#pragma HLS UNROLL`    | Развернуть цикл - создать N копий тела цикла для параллельного выполнения. Увеличивает ресурсы, уменьшает latency |
| `#pragma HLS PIPELINE`  | Конвейеризировать цикл - перекрыть итерации во времени. Увеличивает throughput при умеренном росте ресурсов       |
| `#pragma HLS INTERFACE` | Задать тип интерфейса порта (s_axilite, m_axi, ap_memory, etc.)                                                   |

### Interface Types (Типы интерфейсов)

| Тип            | Описание                                                                                |
| -------------- | --------------------------------------------------------------------------------------- |
| **s_axilite**  | AXI4-Lite Slave - регистровый доступ от CPU. Простой, для небольших данных и управления |
| **m_axi**      | AXI4 Master - DMA-доступ к памяти. Для больших массивов данных                          |
| **ap_memory**  | BRAM-интерфейс - прямое подключение к локальной памяти                                  |
| **ap_fifo**    | FIFO-интерфейс - для потоковой обработки данных                                         |
| **ap_ctrl_hs** | Handshake-управление (ap_start, ap_done, ap_idle, ap_ready)                             |

### Control Signals (Сигналы управления)

| Сигнал        | Описание                                     |
| ------------- | -------------------------------------------- |
| **ap_start**  | Запуск вычисления (CPU записывает 1)         |
| **ap_done**   | Вычисление завершено (однократный импульс)   |
| **ap_idle**   | IP простаивает и готов к новому запуску      |
| **ap_ready**  | Готов принять новые входные данные           |
| **interrupt** | Сигнал прерывания для CPU (обычно = ap_done) |
