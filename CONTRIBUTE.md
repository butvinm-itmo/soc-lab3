## Good to Read

https://www.zzzdavid.tech/loop_opt/

## Running Synthesis

LMAO Vivado use positive part of the 32-bit integer to represent timestamp and apparently it is not enough to store date after 2020,
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

## Подробное объяснение оптимизаций

### Как работает UNROLL

UNROLL **физически дублирует** аппаратуру тела цикла, позволяя выполнять все итерации параллельно.

**Исходный код:**
```c
for (k = 0; k < 7; k++) {
#pragma HLS UNROLL
    temp += B[i][k] * B[k][j];
}
```

**Что генерирует HLS:**
```
                    ┌─────────────────────────────────────────────────────┐
                    │            Комбинационная схема (1 такт)            │
                    │                                                     │
B[i][0], B[0][j] ──→│ MUL ──┐                                             │
B[i][1], B[1][j] ──→│ MUL ──┼──→ ADD ──┐                                  │
B[i][2], B[2][j] ──→│ MUL ──┘          │                                  │
B[i][3], B[3][j] ──→│ MUL ──┐          ├──→ ADD ──┐                       │──→ temp
B[i][4], B[4][j] ──→│ MUL ──┼──→ ADD ──┘          │                       │
B[i][5], B[5][j] ──→│ MUL ──┘                     ├──→ ADD ──────────────→│
B[i][6], B[6][j] ──→│ MUL ──────────────→ ADD ───┘                        │
                    │                                                     │
                    └─────────────────────────────────────────────────────┘
```

**Характеристики UNROLL:**
- Latency = 1 такт (все операции параллельно)
- Ресурсы: 7× MUL, 6× ADD (линейный рост от trip count)
- Критический путь: MUL + log₂(7) уровней ADD ≈ 11 ns

**Проблема:** Критический путь слишком длинный → timing violation

### Как работает PIPELINE

PIPELINE создаёт **конвейер**, где итерации цикла перекрываются во времени. Каждая итерация разбивается на стадии, и пока одна итерация на стадии 2, следующая уже на стадии 1.

**Исходный код:**
```c
for (j = 0; j < 7; j++) {
#pragma HLS PIPELINE
    temp[i][j] = 0;
    for (k = 0; k < 7; k++) {
        temp[i][j] += B[i][k] * B[k][j];
    }
}
```

**Конвейерное выполнение (II=1, идеальный случай):**
```
Такт:     1    2    3    4    5    6    7    8    9   ...
         ─────────────────────────────────────────────────
iter j=0: S1   S2   S3   S4   S5   S6   S7   done
iter j=1:      S1   S2   S3   S4   S5   S6   S7   done
iter j=2:           S1   S2   S3   S4   S5   S6   S7  ...
iter j=3:                S1   S2   S3   S4   S5   S6  ...
...

S1 = Load B[i][0], B[0][j]
S2 = MUL + Load B[i][1], B[1][j]
S3 = ADD + MUL + Load...
...
```

**Реальный случай с зависимостями (II=14):**

В нашем коде `temp[i][j]` накапливает сумму, поэтому следующая итерация внутреннего цикла зависит от предыдущей. HLS не может начинать новую итерацию каждый такт:

```
Такт:     1    2    3    ...   14   15   16   ...   28
         ──────────────────────────────────────────────────
iter j=0: S1   S2   S3   ...   S14  done
iter j=1:                      S1   S2   S3   ...   S14
iter j=2:                                          S1  ...

II = 14 тактов между началом итераций
```

**Почему II=14?** Внутренний цикл `for k` выполняет 7 умножений-сложений последовательно, каждая операция занимает ~2 такта (чтение из памяти + вычисление). 7 × 2 = 14 тактов.

### Сравнение подходов

```
                    Baseline          UNROLL            PIPELINE
                    ────────────────────────────────────────────────
Структура:          Последовательно   Параллельно       Конвейер

Итерации j=0..6:    ┌───┐             ┌───────────────┐  ┌─────────────────┐
                    │j=0│             │j=0,1,2,3,4,5,6│  │j=0──────────────│
                    └───┘             │  (все сразу)  │  │  j=1────────────│
                    ┌───┐             └───────────────┘  │    j=2──────────│
                    │j=1│                                │      ...        │
                    └───┘                                └─────────────────┘
                    ...

Latency:            7 × 30 = 210      1 × 10 = 10       14 + 6 = 20*
                    тактов            тактов            тактов

Ресурсы:            1× MUL            7× MUL            1× MUL + регистры
                    1× ADD            6× ADD            1× ADD + контроллер

Timing:             OK (7 ns)         FAIL (11 ns)      OK (10.8 ns)

* Pipeline latency = II × (trip_count - 1) + iteration_latency
```

### Почему UNROLL нарушает timing

**Критический путь** — самый длинный путь комбинационной логики между двумя регистрами.

**Baseline:**
```
Регистр ──→ [MUL] ──→ [ADD] ──→ Регистр
            └─── ~7 ns ───┘
```

**UNROLL:**
```
            ┌──→ [MUL] ──┐
            ├──→ [MUL] ──┤
Регистры ──→├──→ [MUL] ──┼──→ [ADD] ──→ [ADD] ──→ [ADD] ──→ Регистр
            ├──→ [MUL] ──┤      │          │          │
            ├──→ [MUL] ──┤      │          │          │
            ├──→ [MUL] ──┘      │          │          │
            └──→ [MUL] ────────┘          │          │
                                          │          │
            └───────────── ~11 ns ────────┴──────────┘
```

Target = 10 ns, Estimated = 11 ns → **Timing violation!**

**PIPELINE:**
```
Такт N:   Регистр ──→ [MUL] ──→ Регистр (промежуточный)
Такт N+1: Регистр ──→ [ADD] ──→ Регистр

Критический путь = max(MUL, ADD) ≈ 5-6 ns ✓
```

### Как исправить timing violation в UNROLL

1. **Снизить частоту:**
   ```tcl
   create_clock -period 12 -name default  # 83 MHz вместо 100 MHz
   ```

2. **Частичный UNROLL:**
   ```c
   #pragma HLS UNROLL factor=2  # Развернуть только 2 итерации
   ```
   Критический путь: 2× MUL + 1× ADD ≈ 7 ns ✓

3. **Комбинация UNROLL + PIPELINE:**
   ```c
   for (k = 0; k < 7; k++) {
   #pragma HLS UNROLL factor=2
   #pragma HLS PIPELINE II=2
       temp += B[i][k] * B[k][j];
   }
   ```
   HLS вставит регистры между стадиями.

4. **ARRAY_PARTITION для увеличения пропускной способности памяти:**
   ```c
   #pragma HLS ARRAY_PARTITION variable=B complete dim=2
   ```
   Позволяет читать всю строку/столбец за один такт.

## Вопросы к защите лабораторной работы

### 1. Какова основная цель использования HLS по сравнению с VHDL/Verilog? Какие преимущества получены в этой работе?

**Цель HLS:** Автоматическое преобразование алгоритма на языке C/C++ в RTL-описание (Verilog/VHDL), что позволяет:
- Сократить время разработки в 5-10 раз
- Использовать привычный язык программирования вместо HDL
- Быстро экспериментировать с разными архитектурами через директивы

**Преимущества в нашей работе:**
- Написали 30 строк C-кода вместо сотен строк Verilog
- За минуты сравнили 4 варианта оптимизации (baseline, unroll, partial_unroll, pipeline)
- Получили готовый IP-блок с AXI-Lite интерфейсом без ручного проектирования протокола
- Автоматическая генерация драйверов для CPU

### 2. Прокомментируйте метрики базовой версии (без директив). Соответствует ли результат ожиданиям?

**Метрики baseline:**
- Latency: 1598 cycles = 15.98 мкс @ 100 MHz
- Ресурсы: 1 DSP, 450 FF, 602 LUT
- Estimated clock: 7.18 ns (с запасом от target 10 ns)

**Анализ:**
- Latency соответствует ожиданиям: 3 вложенных цикла 7×7×7 = 343 итерации × ~4 такта/итерация ≈ 1400 тактов
- Используется только 1 DSP — все умножения выполняются последовательно на одном блоке
- Минимальное использование ресурсов (~0% от доступных)
- Timing выполнен с большим запасом — можно повысить частоту или добавить оптимизации

**Вывод:** Результат соответствует ожиданиям для последовательной реализации без оптимизаций.

### 3. Как директива UNROLL повлияла на структуру аппаратного обеспечения?

**Структурные изменения:**
- Внутренний цикл `for k` (7 итераций) развёрнут в 7 параллельных умножителей
- Результаты умножений суммируются через дерево сложений (3 уровня для 7 элементов)
- Вместо 1 DSP используется 4 DSP (HLS оптимизировал дерево)

**Влияние на площадь:**
- DSP: 1 → 4 (×4)
- FF: 450 → 932 (×2) — для хранения промежуточных результатов
- LUT: 602 → 1134 (×1.9) — для дополнительной логики и мультиплексоров

**Влияние на производительность:**
- Latency: 1598 → 611 cycles (ускорение 2.6×)
- Но: Estimated clock = 11 ns > 10 ns target (**timing violation!**)

**Причина timing violation:** Критический путь включает 7 параллельных умножений + 3 уровня сложений, что превышает 10 ns.

### 4. В чём состоит принцип конвейеризации (PIPELINE)?

**Принцип:** PIPELINE разбивает тело цикла на стадии и позволяет нескольким итерациям выполняться одновременно на разных стадиях — аналогично конвейеру в CPU.

**Ключевые характеристики:**
- **Initiation Interval (II)** — через сколько тактов начинается следующая итерация
- II=1 — идеальный случай: новая итерация каждый такт
- II>1 — есть зависимости по данным или ресурсам

**В нашем случае:**
- Loop 1 (умножение): II achieved = 14, II target = 1
- Loop 2 (сложение): II achieved = 1, II target = 1

**Почему II=14 в Loop 1?**
Внутренний цикл `temp[i][j] += B[i][k] * B[k][j]` имеет зависимость по данным: каждая итерация читает и пишет `temp[i][j]`. HLS должен завершить все 7 умножений-сложений (7 × 2 такта = 14) прежде чем начать следующую итерацию внешнего цикла.

**Результат:**
- Latency: 741 cycles (ускорение 2.2× от baseline)
- Timing: 10.86 ns — близко к target, но выполнен
- Ресурсы: умеренный рост (4 DSP, 563 FF, 1081 LUT)

### 5. Сравнительный анализ: производительность vs ресурсоёмкость

| Метрика        | Baseline | Unroll | Partial Unroll | Pipeline |
|----------------|----------|--------|----------------|----------|
| Latency        | 1598     | 611    | 1374           | 741      |
| Ускорение      | 1.0×     | 2.6×   | 1.16×          | 2.2×     |
| DSP            | 1        | 4      | 2              | 4        |
| LUT            | 602      | 1134   | 714            | 1081     |
| Timing Met     | Yes      | **No** | Yes            | Marginal |

**Рекомендация для реального проекта: PIPELINE**

Причины:
1. Лучшее соотношение ускорение/ресурсы (2.2× при умеренном росте)
2. Timing выполнен (в отличие от полного UNROLL)
3. Масштабируемость — работает для больших матриц без timing violation
4. Предсказуемое поведение — II фиксирован

**Альтернатива — Partial Unroll (factor=2):**
- Если нужна гарантия timing с запасом (9.33 ns vs 10 ns target)
- Если важна минимизация ресурсов

### 6. Ограничения синтезируемого подмножества языка C

**Запрещено:**
1. **Динамическое выделение памяти** (`malloc`, `new`) — FPGA требует статического размещения
2. **Рекурсия** — невозможно развернуть в hardware без известной глубины
3. **Указатели на функции** — нельзя определить hardware во время компиляции
4. **Системные вызовы** (`printf`, `fopen`) — нет ОС на FPGA
5. **Переменные размеры массивов** — размер должен быть известен на этапе синтеза

**Ограничения:**
1. **Циклы должны иметь известные границы** — иначе невозможно определить latency
2. **Глобальные переменные** — ограниченная поддержка, лучше избегать
3. **Операции с плавающей точкой** — поддерживаются, но требуют много ресурсов
4. **Большие массивы** — ограничены объёмом BRAM/URAM

**Почему эти ограничения?**
FPGA — статическая архитектура. Все ресурсы должны быть определены на этапе синтеза. Динамическое поведение (runtime allocation, variable bounds) невозможно реализовать в фиксированной схеме.

### 7. Что САПР выводит в отчёте синтеза?

Отчёт `*_csynth.rpt` содержит:

**1. Performance Estimates:**
```
+ Timing (ns):
  Target: 10.00, Estimated: 7.18, Uncertainty: 1.25

+ Latency (clock cycles):
  min: 1598, max: 1598, Interval: 1598
```
- Target — заданный период тактового сигнала
- Estimated — реальный критический путь после синтеза
- Latency — количество тактов от начала до конца
- Interval — через сколько тактов можно запустить следующее вычисление

**2. Utilization Estimates:**
```
+---------+-------+--------+-------+
|  Name   | DSP48E|   FF   |  LUT  |
+---------+-------+--------+-------+
| Total   |   1   |  450   |  602  |
|Available|  240  | 126800 | 63400 |
+---------+-------+--------+-------+
```
- Сколько ресурсов использует дизайн
- Процент от доступных на целевом FPGA

**3. Loop Analysis:**
```
| Loop Name  | Latency | Trip Count | Pipelined |
|------------|---------|------------|-----------|
| Loop 1     |  1484   |     7      |    no     |
| Loop 1.1   |   210   |     7      |    no     |
```
- Детализация по каждому циклу
- Iteration Latency, II achieved/target для pipelined циклов

**4. Interface:**
```
| RTL Ports              | Dir | Bits | Protocol |
|------------------------|-----|------|----------|
| s_axi_AXILiteS_AWADDR  | in  |  8   |  s_axi   |
| s_axi_AXILiteS_WDATA   | in  | 32   |  s_axi   |
```
- Список портов сгенерированного IP
- Протокол и направление каждого порта

### 8. Раздел HLS анализ — для чего нужен и что показывает?

**HLS Analysis** (в Vivado HLS GUI: Solution → Open Schedule Viewer) показывает:

**1. Schedule View — расписание операций по тактам:**
```
Такт 1: load B[i][0], load B[0][j]
Такт 2: mul, load B[i][1], load B[1][j]
Такт 3: add, mul, load...
```
Позволяет увидеть:
- Какие операции выполняются параллельно
- Где возникают зависимости по данным
- Почему II > 1

**2. Resource View — использование ресурсов во времени:**
- Загрузка DSP/BRAM по тактам
- Узкие места (bottleneck) в ресурсах

**3. Пример анализа для нашего PIPELINE:**

```
Такт:  1   2   3   4   5   6   7   8   9  10  11  12  13  14
      ─────────────────────────────────────────────────────────
DSP1: MUL MUL MUL MUL MUL MUL MUL  -   -   -   -   -   -   -
ADD:   -  ADD ADD ADD ADD ADD ADD  -   -   -   -   -   -   -
BRAM:  R   R   R   R   R   R   R   R   -   -   -   -   -   W
                                                          ↑
                                              Запись temp[i][j]
                                              только на такте 14
```

**Почему это полезно:**
- Понять, почему HLS не достиг II=1
- Найти bottleneck (память, DSP, зависимости)
- Оптимизировать дизайн (добавить ARRAY_PARTITION, изменить алгоритм)
