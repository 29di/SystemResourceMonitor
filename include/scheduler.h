#pragma once
#include <vector>
#include <string>

struct Process {
    int pid;
    int arrival;
    int burst;
    int priority;
};

struct ResultRow {
    int pid;
    int waiting;
    int turnaround;
};

struct GanttSlice { int pid; int start; int end; };

struct ScheduleReport {
    std::string algorithm;
    std::vector<ResultRow> rows;
    double avgWaiting{};
    double avgTurnaround{};
    double throughput{}; // processes per unit time
    std::vector<GanttSlice> gantt;
};

ScheduleReport runFCFS(const std::vector<Process>&);
ScheduleReport runSJF(const std::vector<Process>&);
ScheduleReport runRoundRobin(const std::vector<Process>&, int quantum);
ScheduleReport runPriority(const std::vector<Process>&);
ScheduleReport runMultilevelQueue(const std::vector<Process>&);
