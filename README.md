
<div align="center">


<h1>🧰 System Resource Monitor & Process Scheduler Simulator</h1>
<p>
<img src="https://img.shields.io/badge/C-Resource%20Monitor-blue?logo=c" alt="C"/>
<img src="https://img.shields.io/badge/C++-Scheduler-green?logo=c%2B%2B" alt="C++"/>
<img src="https://img.shields.io/badge/Bash-Automation-black?logo=gnu-bash" alt="Bash"/>
</p>
<sub>🔬 Real-time OS Simulation • Multi-threaded C/C++ • IPC • Shell Automation</sub>

</div>

<hr/>

---

## 🧩 Project Overview <img src="https://img.icons8.com/color/32/overview-pages.png" width="24"/>
This project demonstrates core Operating Systems concepts through two practical modules:
- A real-time, multi-threaded Resource Monitor in C that reads Linux `/proc` data, logs metrics, and publishes summaries via POSIX message queues.
- A Process Scheduler Simulator in C++ implementing classic algorithms with metrics and Gantt chart output.

It’s ideal for students and practitioners who want a hands-on understanding of threading, synchronization, IPC, and CPU scheduling.

---

## ⚙️ Features <img src="https://img.icons8.com/color/32/settings.png" width="24"/>
- ✅ C-based Resource Monitor
  - CPU, Memory, Disk I/O, and Network monitoring (via `/proc`)
  - Multi-threaded producers + synchronized logger (producer–consumer pattern)
  - Alerts when CPU/MEM exceed thresholds
  - Periodic summaries via POSIX message queues (IPC)
  - Graceful shutdown (Ctrl+C)

- ✅ C++ Scheduler Simulator (STL-based)
  - Algorithms: FCFS, SJF, Round Robin (q=2), Priority, Multilevel Queue
  - Computes Waiting Time, Turnaround Time, Throughput
  - Prints Gantt chart slices for visualization
  - Appends results to `data/reports/scheduler_report.txt`

- ✅ IPC Consumer
  - Reads and prints live summaries from the POSIX message queue

- ✅ Automation Scripts
  - `cleanup_logs.sh` — cleanup and retention for logs
  - `health_check.sh` — simple CPU/MEM threshold alerting
  - `generate_report.sh` — collates recent logs and scheduler report

---

## 🧠 How It Works <img src="https://img.icons8.com/color/32/brain.png" width="24"/>
- Threaded Monitoring (C)
  - One thread each for CPU, Memory, Disk, Network → enqueues metrics
  - Logger thread dequeues metrics, writes to `data/logs/resource_log.txt`, and raises alerts
  - Every few seconds, a summary (e.g., `CPU=xx.x% MEM=yy.y%`) is sent to a POSIX message queue (default: `/sysmon_queue`)

- Scheduler Algorithms (C++)
  - Reads processes from CSV: `PID,Arrival,Burst,Priority`
  - Runs FCFS, SJF, RR(q=2), Priority, Multilevel Queue
  - Produces per-process waiting/turnaround, averages, throughput, and Gantt chart slices

- IPC Messaging
  - Separate consumer reads summaries from the message queue and prints them to stdout

---

## 🖥️ System Requirements <img src="https://img.icons8.com/color/32/laptop.png" width="24"/>
- Linux or Windows Subsystem for Linux (WSL)
- GCC, G++, Make, Bash
- Access to Linux `/proc` filesystem and POSIX message queues

> Note: On Windows, use WSL and run commands from a WSL terminal.

---

## 🏗️ Installation & Setup <img src="https://img.icons8.com/color/32/installation.png" width="24"/>

Clone the repository and build all targets:

```bash
git clone https://github.com/&lt;your-username&gt;/SystemResourceMonitor.git
cd SystemResourceMonitor
make all
```

<sub>If using WSL and your project is in a Windows folder, use:</sub>

```bash
cd "/mnt/c/Users/PC/OneDrive/Desktop/Maulishka's Projects/SystemResourceMonitor"
make all
```

> If needed, install build tools (Ubuntu): `sudo apt update && sudo apt install -y build-essential`

---

## ▶️ How to Run <img src="https://img.icons8.com/color/32/play.png" width="24"/>

<details>
<summary><strong>🚦 Quick Start: All-in-One</strong></summary>

```bash
make all && make run_monitor
```
<sub>Builds everything and starts the resource monitor. Press <kbd>Ctrl</kbd>+<kbd>C</kbd> to stop.</sub>

<pre>
Resource Monitor started. Press Ctrl+C to stop.
Logging to data/logs/resource_log.txt
Sending summaries to POSIX mq /sysmon_queue (if available)
</pre>
</details>

---

<details>
<summary><strong>🖥️ Resource Monitor (C)</strong></summary>

```bash
make run_monitor
```
<sub>Monitors CPU, memory, disk, and network. Logs to <code>data/logs/resource_log.txt</code>.</sub>

<pre>
1697654321000,CPU,42.35
1697654321500,MEM,61.20
1697654322000,DISK,128,64
1697654322500,NET,4096,2048
1697654323000,ALERT,CPU_HIGH,91.75
</pre>
</details>

---

<details>
<summary><strong>📡 IPC Consumer (POSIX MQ)</strong></summary>

```bash
./bin/ipc_consumer
```
<sub>Shows live summaries from the message queue.</sub>

<pre>
[Summary] CPU=37.4% MEM=58.2%
[Summary] CPU=41.9% MEM=60.1%
</pre>
</details>

---

<details>
<summary><strong>📊 Scheduler Simulator (C++)</strong></summary>

```bash
./bin/scheduler data/processes.csv
```
<sub>Runs all algorithms and prints Gantt chart. Appends to <code>data/reports/scheduler_report.txt</code>.</sub>

<pre>
Algorithm: FCFS
PID	Waiting	Turnaround
1	0	5
2	4	6
3	7	15
Avg Waiting: 3.7, Avg Turnaround: 8.7, Throughput: 0.3
Gantt: |P1(0-5)|P2(5-8)|P3(8-16)| end=16
</pre>
</details>

---

<details>
<summary><strong>📝 Generate a consolidated report</strong></summary>

```bash
bash scripts/generate_report.sh
```
<sub>The combined report is saved to <code>data/reports/final_summary.txt</code>.</sub>

<pre>
=== System Resource Monitor Summary ===
... (last 50 log lines) ...

=== Scheduler Latest Report ===
... (last 50 scheduler report lines) ...
</pre>
</details>

---

<details>
<summary><strong>🕹️ (Optional) Interactive Menu</strong></summary>

```bash
./bin/menu
```
<sub>Text-based menu to run all modules and scripts.</sub>
</details>

---

## 📂 Project Structure <img src="https://img.icons8.com/color/32/folder-invoices.png" width="24"/>
```
SystemResourceMonitor/
├── src/
│   ├── resource_monitor.c         # C monitor (threads, /proc, IPC, signals)
│   ├── ipc_consumer.c             # POSIX mqueue summaries reader
│   ├── scheduler_simulator.cpp    # C++ scheduling simulator
│   └── main.cpp                   # minimal interactive menu
├── include/
│   ├── monitor.h
│   └── scheduler.h
├── scripts/
│   ├── cleanup_logs.sh
│   ├── health_check.sh
│   └── generate_report.sh
├── data/
│   ├── processes.csv              # sample scheduler input
│   ├── logs/
│   │   └── resource_log.txt       # generated at runtime
│   └── reports/
│       └── scheduler_report.txt   # generated at runtime
├── bin/                           # built executables
├── Makefile
└── README.md
```

---

## 🧪 Sample Output <img src="https://img.icons8.com/color/32/test-tube.png" width="24"/>
- Gantt chart (console):
```
Algorithm: FCFS
PID	Waiting	Turnaround
1	0	5
2	4	6
3	7	15
Avg Waiting: 3.7, Avg Turnaround: 8.7, Throughput: 0.3
Gantt: |P1(0-5)|P2(5-8)|P3(8-16)| end=16
```

- Resource log snippet (`data/logs/resource_log.txt`):
```
1697654321000,CPU,42.35
1697654321500,MEM,61.20
1697654322000,DISK,128,64
1697654322500,NET,4096,2048
1697654323000,ALERT,CPU_HIGH,91.75
```

- IPC consumer (summaries):
```
[Summary] CPU=37.4% MEM=58.2%
[Summary] CPU=41.9% MEM=60.1%
```

---

## 🚀 Future Enhancements <img src="https://img.icons8.com/color/32/rocket--v1.png" width="24"/>
- Shared Memory or Pipes as an additional IPC demo
- Cron integration for `health_check.sh`
- Live statistics in the interactive menu (e.g., stream last summary)
- Configurable thresholds and sampling intervals via a config file

---

## 🤝 Contributing <img src="https://img.icons8.com/color/32/handshake.png" width="24"/>
Contributions are welcome! Feel free to open issues or submit PRs:
- Add new scheduling algorithms or visualizations
- Improve error handling, parsing, or portability
- Expand documentation and examples

Please follow conventional commit messages and include a brief description of changes.

---

## 📜 License <img src="https://img.icons8.com/color/32/certificate.png" width="24"/>
Specify your preferred license (e.g., MIT) and add a `LICENSE` file to the repository.

---

## ✨ Credits / Authors <img src="https://img.icons8.com/color/32/conference-call.png" width="24"/>
- Your Name (@yourhandle)
- Thanks to the Linux `/proc` documentation and the C/C++ standard libraries
