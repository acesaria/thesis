#!/bin/bash
# Disk acquisition (low-level)
# Usage: ./disk_acquire.sh <vm_name> <output_dir>

if [ $# -ne 2 ]; then
    echo "Usage: $0 <vm_name> <output_dir>"
    exit 1
fi

VM="$1"
OUTDIR="$2"

if [ ! -d "$OUTDIR" ]; then
    echo "[-] Directory not found: $OUTDIR"
    exit 1
fi

echo "[disk] Acquiring from $VM..."

DISK_PATH=$(virsh domblklist "$VM" --details | awk '$1=="file" && $2=="disk" {print $4}')

if [ -z "$DISK_PATH" ]; then
    echo "[disk] Disk not found"
    exit 1
fi

echo "[disk] Source: $DISK_PATH"

if virsh list --state-running | grep -q "$VM"; then
    echo "[disk] Shutting down VM..."
    virsh shutdown "$VM"
    sleep 10
    
    if virsh list --state-running | grep -q "$VM"; then
        virsh destroy "$VM"
        sleep 2
    fi
fi

OUTPUT="$OUTDIR/disk"
START=$(date '+%Y-%m-%d %H:%M:%S')

# Check if QCOW2 - convert to temporary RAW first
if sudo file "$DISK_PATH" | grep -q "QCOW"; then
    echo "[disk] Detected QCOW2 format, converting to RAW first..."
    TMPRAW="/tmp/${VM}_$(date +%s).raw"
    
    sudo qemu-img convert -f qcow2 -O raw "$DISK_PATH" "$TMPRAW"
    
    if [ $? -ne 0 ]; then
        echo "[disk] QCOW2 conversion FAILED"
        exit 1
    fi
    
    ACQUIRE_SOURCE="$TMPRAW"
else
    ACQUIRE_SOURCE="$DISK_PATH"
fi

echo "[disk] Acquiring (EWF fast compression, ~10min)..."

sudo ewfacquire -t "$OUTPUT" -C "Thesis" -D "VM:$VM" \
                -E "$(whoami)" -c fast -f encase6 -u "$ACQUIRE_SOURCE"

if [ $? -ne 0 ]; then
    echo "[disk] FAILED"
    [ -n "$TMPRAW" ] && sudo rm -f "$TMPRAW"
    exit 1
fi

# Cleanup temporary RAW
[ -n "$TMPRAW" ] && sudo rm -f "$TMPRAW"

END=$(date '+%Y-%m-%d %H:%M:%S')

sudo chown $USER:$USER "$OUTPUT".E*

echo "[disk] Verifying..."
ewfverify "${OUTPUT}.E01" > "$OUTDIR/disk_verify.log" 2>&1

MD5=$(ewfinfo "${OUTPUT}.E01" 2>/dev/null | grep -i "MD5:" | awk '{print $NF}')
SIZE=$(stat -c%s "${OUTPUT}".E* 2>/dev/null | awk '{sum+=$1} END {printf "%.1f GB", sum/1024/1024/1024}')

cat >> "$OUTDIR/manifest.txt" <<DISK
Disk Acquisition:
-----------------
Start:    $START
End:      $END
File:     disk.E01
Size:     $SIZE
MD5:      ${MD5:-N/A}
Method:   ewfacquire -c fast (QCOW2→RAW converted)
Source:   $DISK_PATH

DISK

echo "[disk] Done: $SIZE (MD5: ${MD5:0:16}...)"

read -p "[disk] Restart VM? (y/n): " -n 1 R
echo
[[ $R =~ ^[Yy]$ ]] && virsh start "$VM"
