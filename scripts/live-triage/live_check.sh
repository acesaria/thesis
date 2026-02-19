#!/bin/bash
# Quick live vs forensics check
# Usage: ./live_check <vm_name> <exp_dir>

VM_IP="$1"
EXPDIR="$2"
OUT="$EXPDIR/live_hooked.txt"

echo "=== LIVE COMMANDS (potentially hooked) ===" > "$OUT"
echo "" >> "$OUT"

echo "[ps count]" >> "$OUT"
ssh user@$VM_IP "ps aux | wc -l" >> "$OUT"
echo "" >> "$OUT"

echo "[/tmp/ files]" >> "$OUT"
ssh user@$VM_IP "ls /tmp/" >> "$OUT"
echo "" >> "$OUT"

echo "[/etc/ld.so.preload]" >> "$OUT"
ssh user@$VM_IP "cat /etc/ld.so.preload 2>&1" >> "$OUT"
echo "" >> "$OUT"

echo "[lsmod count]" >> "$OUT"
ssh user@$VM_IP "lsmod | wc -l" >> "$OUT"

echo "" >> "$OUT"
echo "NOTE: Compare with memory_analysis/ and disk_analysis/ for ground truth" >> "$OUT"

echo "[+] Quick comparison saved: $OUT"
