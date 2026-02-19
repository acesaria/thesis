#!/bin/bash
# Sleuth Kit disk analysis
# Usage: ./disk_analyze.sh <disk_image> <output_dir>

if [ $# -ne 2 ]; then
    echo "Usage: $0 <disk_image> <output_dir>"
    exit 1
fi

DISK="$1"
OUTDIR="$2"

mkdir -p $OUTDIR

if [ ! -f "$DISK" ]; then
    echo "[-] Image not found: $DISK"
    exit 1
fi

echo "[+] Analyzing: $(basename $DISK)"
echo "[+] Output: $OUTDIR"

echo "  [+] mmls..."
mmls "$DISK" > "$OUTDIR/mmls.txt" 2>&1

# GPT: take largest partition (skip lines with Meta/---)
# MBR: take Linux (0x83)
OFFSET=$(awk '
  /^[0-9]+:/ && !/Meta/ && !/-------/ && !/Safety/ {
    if (max_len == "" || $5 > max_len) {
      max_len = $5
      max_offset = $3
    }
  }
  END { print max_offset }
' "$OUTDIR/mmls.txt")

[ -z "$OFFSET" ] && OFFSET=0

echo "      Partition offset: $OFFSET sectors"

echo "  [+] fsstat..."
fsstat -o "$OFFSET" "$DISK" > "$OUTDIR/fsstat.txt" 2>&1

echo "  [+] fls (recursive, ~2min)..."
fls -r -o "$OFFSET" "$DISK" > "$OUTDIR/fls_recursive.txt" 2>&1

echo "  [+] timeline..."
fls -r -m / -o "$OFFSET" "$DISK" > "$OUTDIR/timeline.body" 2>&1
mactime -b "$OUTDIR/timeline.body" -d  -z UTC > "$OUTDIR/timeline.csv" 2>&1

echo "[+] Complete. Results in: $OUTDIR"
