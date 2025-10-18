#include "scheduler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <queue>
#include <map>

static void printGantt(const std::vector<GanttSlice>& g) {
    if (g.empty()) return;
    int lastEnd = g.back().end;
    std::cout << "Gantt: ";
    for (auto &s : g) {
        std::cout << "|P" << s.pid << "(" << s.start << "-" << s.end << ")";
    }
    std::cout << "| end=" << lastEnd << "\n";
}

static void printReport(const ScheduleReport& r) {
    std::cout << "Algorithm: " << r.algorithm << "\n";
    std::cout << "PID\tWaiting\tTurnaround\n";
    for (auto &row : r.rows) {
        std::cout << row.pid << "\t" << row.waiting << "\t" << row.turnaround << "\n";
    }
    std::cout << "Avg Waiting: " << r.avgWaiting << ", Avg Turnaround: " << r.avgTurnaround << ", Throughput: " << r.throughput << "\n";
    printGantt(r.gantt);
}

// Minimal FCFS example to make binary functional; replace with full implementations later
static void finalizeReport(const std::vector<Process>& p, std::vector<ResultRow>& rows, ScheduleReport& rep, int lastFinish) {
    double sumW=0,sumT=0;
    for (auto &r : rows) { sumW += r.waiting; sumT += r.turnaround; }
    if (!rows.empty()) {
        rep.avgWaiting = sumW / rows.size();
        rep.avgTurnaround = sumT / rows.size();
        if (lastFinish>0) rep.throughput = (double)rows.size()/ (double)lastFinish;
    }
}

ScheduleReport runFCFS(const std::vector<Process>& procs) {
    std::vector<Process> p = procs;
    std::sort(p.begin(), p.end(), [](auto&a, auto&b){return a.arrival < b.arrival;});
    int time = 0; ScheduleReport rep; rep.algorithm = "FCFS";
    std::vector<ResultRow> rows;
    for (auto &pr : p) {
        if (time < pr.arrival) time = pr.arrival;
        int start = time;
        int wait = start - pr.arrival;
        int tat = wait + pr.burst;
        time += pr.burst;
        rows.push_back({pr.pid, wait, tat});
        rep.gantt.push_back({pr.pid, start, time});
    }
    rep.rows = rows; finalizeReport(p, rep.rows, rep, time);
    return rep;
}

ScheduleReport runSJF(const std::vector<Process>& procs){
    std::vector<Process> p = procs; ScheduleReport rep; rep.algorithm = "SJF";
    std::sort(p.begin(), p.end(), [](auto&a, auto&b){return a.arrival<b.arrival;});
    auto cmp = [](const Process&a, const Process&b){return a.burst>b.burst;};
    std::priority_queue<Process, std::vector<Process>, decltype(cmp)> pq(cmp);
    int time=0, i=0; std::vector<ResultRow> rows;
    while (i<(int)p.size() || !pq.empty()) {
        while (i<(int)p.size() && p[i].arrival<=time) pq.push(p[i++]);
        if (pq.empty()) { time = p[i].arrival; continue; }
        Process cur = pq.top(); pq.pop();
        int start = time; time += cur.burst; int wait = start - cur.arrival; int tat = wait + cur.burst;
        rows.push_back({cur.pid, wait, tat});
        rep.gantt.push_back({cur.pid, start, time});
    }
    rep.rows = rows; finalizeReport(p, rep.rows, rep, time); return rep;
}

ScheduleReport runRoundRobin(const std::vector<Process>& procs, int quantum){
    std::vector<Process> p = procs; ScheduleReport rep; rep.algorithm = "RoundRobin";
    std::sort(p.begin(), p.end(), [](auto&a, auto&b){return a.arrival<b.arrival;});
    std::queue<Process> q; int time=0, i=0; std::map<int,int> remaining;
    for (auto &pr: p) remaining[pr.pid]=pr.burst;
    std::map<int,int> firstStart;
    std::map<int,int> finish;
    while (i<(int)p.size() || !q.empty()) {
        while (i<(int)p.size() && p[i].arrival<=time) q.push(p[i++]);
        if (q.empty()) { time = p[i].arrival; continue; }
        Process cur = q.front(); q.pop();
        int run = std::min(quantum, remaining[cur.pid]);
        int start = time; time += run; remaining[cur.pid]-=run;
        if (!firstStart.count(cur.pid)) firstStart[cur.pid]=start;
        rep.gantt.push_back({cur.pid, start, time});
        while (i<(int)p.size() && p[i].arrival<=time) q.push(p[i++]);
        if (remaining[cur.pid]>0) { q.push(cur); }
        else { finish[cur.pid]=time; }
    }
    std::vector<ResultRow> rows;
    for (auto &pr : p) {
        int tat = finish[pr.pid]-pr.arrival;
        int waiting = tat - pr.burst;
        rows.push_back({pr.pid, waiting, tat});
    }
    rep.rows = rows; finalizeReport(p, rep.rows, rep, time); return rep;
}

ScheduleReport runPriority(const std::vector<Process>& procs){
    std::vector<Process> p = procs; ScheduleReport rep; rep.algorithm = "Priority";
    std::sort(p.begin(), p.end(), [](auto&a, auto&b){return a.arrival<b.arrival;});
    auto cmp = [](const Process&a, const Process&b){return a.priority>b.priority;};
    std::priority_queue<Process, std::vector<Process>, decltype(cmp)> pq(cmp);
    int time=0, i=0; std::vector<ResultRow> rows;
    while (i<(int)p.size() || !pq.empty()) {
        while (i<(int)p.size() && p[i].arrival<=time) pq.push(p[i++]);
        if (pq.empty()) { time = p[i].arrival; continue; }
        Process cur = pq.top(); pq.pop();
        int start=time; time += cur.burst; int wait=start-cur.arrival; int tat=wait+cur.burst;
        rows.push_back({cur.pid, wait, tat});
        rep.gantt.push_back({cur.pid, start, time});
    }
    rep.rows = rows; finalizeReport(p, rep.rows, rep, time); return rep;
}

ScheduleReport runMultilevelQueue(const std::vector<Process>& procs){
    // Simple 2-queue MLQ: high-priority (priority<=1) Round Robin (q=2), low-priority FCFS; strict priority to high queue.
    std::vector<Process> p = procs; ScheduleReport rep; rep.algorithm = "MultilevelQueue";
    std::sort(p.begin(), p.end(), [](auto&a, auto&b){return a.arrival<b.arrival;});
    std::queue<Process> high, low; int time=0, i=0; std::map<int,int> remaining, finish;
    for (auto &pr : p) remaining[pr.pid]=pr.burst;
    while (i<(int)p.size() || !high.empty() || !low.empty()) {
        while (i<(int)p.size() && p[i].arrival<=time) {
            if (p[i].priority<=1) high.push(p[i]); else low.push(p[i]);
            i++;
        }
        if (high.empty() && low.empty()) { time=p[i].arrival; continue; }
        if (!high.empty()) {
            auto cur = high.front(); high.pop();
            int run = std::min(2, remaining[cur.pid]);
            int start=time; time+=run; remaining[cur.pid]-=run; rep.gantt.push_back({cur.pid,start,time});
            while (i<(int)p.size() && p[i].arrival<=time) { if (p[i].priority<=1) high.push(p[i]); else low.push(p[i]); i++; }
            if (remaining[cur.pid]>0) high.push(cur); else finish[cur.pid]=time;
        } else {
            auto cur = low.front(); low.pop();
            int start=time; time+=remaining[cur.pid]; rep.gantt.push_back({cur.pid,start,time}); remaining[cur.pid]=0; finish[cur.pid]=time;
        }
    }
    std::vector<ResultRow> rows;
    for (auto &pr : p) { int tat=finish[pr.pid]-pr.arrival; int waiting=tat-pr.burst; rows.push_back({pr.pid,waiting,tat}); }
    rep.rows=rows; finalizeReport(p, rep.rows, rep, time); return rep;
}

static std::vector<Process> loadCsv(const std::string& path) {
    std::vector<Process> out; std::ifstream f(path);
    if (!f) return out; std::string line; bool header=true;
    while (std::getline(f,line)) {
        if (line.empty()) continue; if (header) { header=false; if (line.find("PID")!=std::string::npos) continue; }
        std::replace(line.begin(), line.end(), ';', ',');
        std::stringstream ss(line); std::string tok; std::vector<int> vals;
        while (std::getline(ss,tok,',')) { if (!tok.empty()) vals.push_back(std::stoi(tok)); }
        if (vals.size()>=4) out.push_back({vals[0],vals[1],vals[2],vals[3]});
    }
    return out;
}

int main(int argc, char** argv) {
    std::string csv = (argc>1) ? argv[1] : "data/processes.csv";
    auto procs = loadCsv(csv);
    if (procs.empty()) {
        std::cerr << "No processes loaded from " << csv << ". Ensure CSV exists with: PID,Arrival,Burst,Priority\n";
        return 1;
    }
    std::vector<ScheduleReport> reps;
    reps.push_back(runFCFS(procs));
    reps.push_back(runSJF(procs));
    reps.push_back(runRoundRobin(procs, 2));
    reps.push_back(runPriority(procs));
    reps.push_back(runMultilevelQueue(procs));
    std::ofstream ofs("data/reports/scheduler_report.txt", std::ios::app);
    for (auto &r : reps) {
        printReport(r);
        if (ofs) {
            ofs << "Algorithm," << r.algorithm << "\n";
            for (auto &row : r.rows) ofs << row.pid << "," << row.waiting << "," << row.turnaround << "\n";
            ofs << "AvgWaiting," << r.avgWaiting << ",AvgTurnaround," << r.avgTurnaround << ",Throughput," << r.throughput << "\n";
        }
    }
    return 0;
}
