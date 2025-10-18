#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REPORT_DIR="$ROOT/data/reports"
LOG_DIR="$ROOT/data/logs"
OUT="$REPORT_DIR/full_report_$(date +%Y%m%d_%H%M%S).txt"
mkdir -p "$REPORT_DIR"
{
  echo "=== System Resource Monitor Summary ==="
  tail -n 50 "$LOG_DIR/resource_log.txt" 2>/dev/null || echo "No resource logs yet."
  echo
  echo "=== Scheduler Latest Report ==="
  tail -n 50 "$REPORT_DIR/scheduler_report.txt" 2>/dev/null || echo "No scheduler report yet."
} > "$OUT"
echo "Generated: $OUT"
