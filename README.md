# 🧰 System Resource Monitor & Process Scheduler Simulator

<p align="center">
  <img src="https://img.shields.io/badge/C-Resource%20Monitor-blue?logo=c" alt="C"/>
  <img src="https://img.shields.io/badge/C++-Scheduler-green?logo=c%2B%2B" alt="C++"/>
  <img src="https://img.shields.io/badge/Bash-Automation-black?logo=gnu-bash" alt="Bash"/>
</p>

<p align="center"><em>🔬 Real-time OS Simulation • Multi-threaded C/C++ • IPC • Shell Automation</em></p>

---

## 📑 Table of Contents

1. [Project Overview](#project-overview)
2. [Features](#features)
3. [How It Works](#how-it-works)
4. [System Requirements](#system-requirements)
5. [Installation & Setup](#installation--setup)
6. [How to Run](#how-to-run)
7. [Project Structure](#project-structure)
8. [Sample Output](#sample-output)
9. [Future Enhancements](#future-enhancements)
10. [Contributing](#contributing)
11. [License](#license)
12. [Credits / Authors](#credits--authors)

---

## 🧩 Project Overview

**System Resource Monitor & Process Scheduler Simulator** demonstrates core Operating Systems concepts through two practical modules:

- **Resource Monitor (C):** Real-time, multi-threaded monitor that reads Linux `/proc` data, logs metrics, and publishes summaries via POSIX message queues.
- **Process Scheduler Simulator (C++):** Implements classic scheduling algorithms with metrics and Gantt chart output.

<details>
<summary><strong>Who is this for?</strong></summary>

Ideal for students and practitioners seeking hands-on experience with threading, synchronization, IPC, and CPU scheduling.

</details>

---


## ⚙️ Features

| Module                | Key Features |
|-----------------------|--------------|
| **Resource Monitor (C)** | - Monitors CPU, Memory, Disk I/O, Network (via `/proc`)<br>- Multi-threaded (producer–consumer)<br>- Alerts on CPU/MEM thresholds<br>- Periodic summaries via POSIX message queues<br>- Graceful shutdown (Ctrl+C) |
| **Scheduler Simulator (C++)** | - Algorithms: FCFS, SJF, RR (q=2), Priority, Multilevel Queue<br>- Computes waiting/turnaround/throughput<br>- Gantt chart visualization<br>- Appends results to `data/reports/scheduler_report.txt` |
| **IPC Consumer**      | - Reads and prints live summaries from POSIX message queue |
| **Automation Scripts**| - `cleanup_logs.sh`: log cleanup<br>- `health_check.sh`: threshold alerting<br>- `generate_report.sh`: collates logs and reports |

---

---


## 🧠 How It Works

<details>
<summary><strong>Threaded Monitoring (C)</strong></summary>

- Dedicated threads for CPU, Memory, Disk, Network (enqueue metrics)
- Logger thread dequeues, writes to `data/logs/resource_log.txt`, raises alerts
- Periodic summary (e.g., `CPU=xx.x% MEM=yy.y%`) sent to POSIX message queue (`/sysmon_queue`)

</details>

<details>
<summary><strong>Scheduler Algorithms (C++)</strong></summary>

- Reads processes from CSV: `PID,Arrival,Burst,Priority`
- Runs FCFS, SJF, RR(q=2), Priority, Multilevel Queue
- Outputs per-process waiting/turnaround, averages, throughput, Gantt chart

</details>

<details>
<summary><strong>IPC Messaging</strong></summary>

- Separate consumer reads summaries from the message queue and prints to stdout

</details>

---

---


## 🖥️ System Requirements

- Linux or Windows Subsystem for Linux (WSL)
- GCC, G++, Make, Bash
- Access to Linux `/proc` filesystem and POSIX message queues

> **Note:** On Windows, use WSL and run commands from a WSL terminal.

---

---


## 🏗️ Installation & Setup

**Clone and build:**

```bash
git clone https://github.com/<your-username>/SystemResourceMonitor.git
cd SystemResourceMonitor
make all
```

<details>
<summary>If using WSL and your project is in a Windows folder:</summary>

```bash
cd "/path/to/SystemResourceMonitor"
make all
```
</details>

> If needed, install build tools (Ubuntu): `sudo apt update && sudo apt install -y build-essential`

---

---


## ▶️ How to Run

<ul>
<li><strong>🚦 Quick Start (All-in-One):</strong>

  <pre><code>make all && make run_monitor</code></pre>
  <sub>Builds everything and starts the resource monitor. Press <kbd>Ctrl</kbd>+<kbd>C</kbd> to stop.</sub>
  <br>
  <pre>
Resource Monitor started. Press Ctrl+C to stop.
Logging to data/logs/resource_log.txt
Sending summaries to POSIX mq /sysmon_queue (if available)
  </pre>
</li>

<li><strong>🖥️ Resource Monitor (C):</strong>
  <pre><code>make run_monitor</code></pre>
  <sub>Monitors CPU, memory, disk, and network. Logs to <code>data/logs/resource_log.txt</code>.</sub>
  <pre>
1697654321000,CPU,42.35
1697654321500,MEM,61.20
1697654322000,DISK,128,64
1697654322500,NET,4096,2048
1697654323000,ALERT,CPU_HIGH,91.75
  </pre>
</li>

<li><strong>📡 IPC Consumer (POSIX MQ):</strong>
  <pre><code>./bin/ipc_consumer</code></pre>
  <sub>Shows live summaries from the message queue.</sub>
  <pre>
[Summary] CPU=37.4% MEM=58.2%
[Summary] CPU=41.9% MEM=60.1%
  </pre>
</li>

<li><strong>📊 Scheduler Simulator (C++):</strong>
  <pre><code>./bin/scheduler data/processes.csv</code></pre>
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
</li>

<li><strong>📝 Generate a consolidated report:</strong>
  <pre><code>bash scripts/generate_report.sh</code></pre>
  <sub>The combined report is saved to <code>data/reports/final_summary.txt</code>.</sub>
  <pre>
=== System Resource Monitor Summary ===
... (last 50 log lines) ...

=== Scheduler Latest Report ===
... (last 50 scheduler report lines) ...
  </pre>
</li>

<li><strong>🕹️ (Optional) Interactive Menu:</strong>
  <pre><code>./bin/menu</code></pre>
  <sub>Text-based menu to run all modules and scripts.</sub>
</li>
</ul>

---


## 📂 Project Structure

```text
SystemResourceMonitor/
├── src/                # Source code (C/C++)
│   ├── resource_monitor.c
│   ├── ipc_consumer.c
│   ├── scheduler_simulator.cpp
│   └── main.cpp
├── include/            # Header files
│   ├── monitor.h
│   └── scheduler.h
├── scripts/            # Bash automation scripts
│   ├── cleanup_logs.sh
│   ├── health_check.sh
│   └── generate_report.sh
├── data/               # Input, logs, and reports
│   ├── processes.csv
│   ├── logs/
│   │   └── resource_log.txt
│   └── reports/
│       └── scheduler_report.txt
├── bin/                # Built executables
├── Makefile
└── README.md
```

---

---


## 🧪 Sample Output

**Gantt chart (console):**

```text
Algorithm: FCFS
PID	Waiting	Turnaround
1	0	5
2	4	6
3	7	15
Avg Waiting: 3.7, Avg Turnaround: 8.7, Throughput: 0.3
Gantt: |P1(0-5)|P2(5-8)|P3(8-16)| end=16
```

**Resource log snippet (`data/logs/resource_log.txt`):**

```text
1697654321000,CPU,42.35
1697654321500,MEM,61.20
1697654322000,DISK,128,64
1697654322500,NET,4096,2048
1697654323000,ALERT,CPU_HIGH,91.75
```

**IPC consumer (summaries):**

```text
[Summary] CPU=37.4% MEM=58.2%
[Summary] CPU=41.9% MEM=60.1%
```

---

---


## 🚀 Future Enhancements

- Shared Memory or Pipes as an additional IPC demo
- Cron integration for `health_check.sh`
- Live statistics in the interactive menu (e.g., stream last summary)
- Configurable thresholds and sampling intervals via a config file

---

---


## 🤝 Contributing

Contributions are welcome! Feel free to open issues or submit PRs:

- Add new scheduling algorithms or visualizations
- Improve error handling, parsing, or portability
- Expand documentation and examples

Please follow conventional commit messages and include a brief description of changes.

---

---


## 📜 License

This project is licensed under the [MIT License](LICENSE) – see the LICENSE file for details.

---

---


## ✨ Credits / Authors

- Divya Sinha
- Thanks to the Linux `/proc` documentation and the C/C++ standard libraries
