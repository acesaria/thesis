#!/bin/bash
# Memory acquisition (low-level)
# Usage: ./mem_acquire.sh <vm_name> <output_dir>

if [ $# -ne 2 ]; then
    echo "Usage: $0 <vm_name> <output_dir>"
    exit 1
fi

VM="$1"
OUTDIR="$2"

[ ! -d "$OUTDIR" ] && mkdir -p "$OUTDIR"

echo "[mem] Dumping from $VM..."

START=$(date '+%Y-%m-%d %H:%M:%S')
virsh dump "$VM" "$OUTDIR/memory.dump" --memory-only

if [ $? -ne 0 ]; then
    echo "[mem] FAILED"
    exit 1
fi

END=$(date '+%Y-%m-%d %H:%M:%S')

sudo chown -R $USER:$USER "$OUTDIR"

sha256sum "$OUTDIR/memory.dump" > "$OUTDIR/memory.dump.sha256"
HASH=$(cut -d' ' -f1 "$OUTDIR/memory.dump.sha256")
SIZE=$(stat -c%s "$OUTDIR/memory.dump" | awk '{printf "%.1f MB", $1/1024/1024}')

cat >> "$OUTDIR/manifest.txt" <<MEM
Memory Acquisition:
-------------------
Start:    $START
End:      $END
File:     memory.dump
Size:     $SIZE
SHA256:   $HASH
Method:   virsh dump --memory-only

MEM

echo "[mem] Done: $SIZE (SHA256: ${HASH:0:16}...)"
