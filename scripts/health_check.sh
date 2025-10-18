#!/usr/bin/env bash
set -euo pipefail

CPU_TH=${1:-85}
MEM_TH=${2:-85}

echo "Health check: CPU>${CPU_TH}% or MEM>${MEM_TH}% => ALERT"

# CPU: compute usage over short interval
read cpu a b c idle rest < /proc/stat
sleep 0.2
read cpu a2 b2 c2 idle2 rest2 < /proc/stat
idle_delta=$((idle2-idle))
non_delta=$(((a2-a)+(b2-b)+(c2-c)))
total=$((idle_delta+non_delta))
cpu_usage=$(( 100*non_delta/ (total==0?1:total) ))

# MEM: MemTotal and MemAvailable in kB
mem_total=$(grep -m1 MemTotal /proc/meminfo | awk '{print $2}')
mem_avail=$(grep -m1 MemAvailable /proc/meminfo | awk '{print $2}')
mem_used=$((mem_total-mem_avail))
mem_usage=$(( 100*mem_used/ (mem_total==0?1:mem_total) ))

status=0
if (( cpu_usage > CPU_TH )); then echo "ALERT: CPU $cpu_usage%"; status=1; else echo "OK: CPU $cpu_usage%"; fi
if (( mem_usage > MEM_TH )); then echo "ALERT: MEM $mem_usage%"; status=1; else echo "OK: MEM $mem_usage%"; fi
exit $status
