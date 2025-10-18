#!/usr/bin/env bash
set -euo pipefail
# Cleanup old logs and keep last N files
KEEP=${1:-5}
LOG_DIR="$(dirname "$0")/../data/logs"
mkdir -p "$LOG_DIR"
echo "Keeping last $KEEP log files in $LOG_DIR"
ls -1t "$LOG_DIR" | tail -n +$((KEEP+1)) | while read -r f; do
  echo "Removing $f"; rm -f "$LOG_DIR/$f" || true; done
