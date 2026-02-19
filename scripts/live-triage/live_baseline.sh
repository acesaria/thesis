#!/bin/bash
# Live baseline collection
# Usage: ./live_baseline.sh <vm_ip> <scenario> <exp_dir>

if [ $# -ne 3 ]; then
    echo "Usage: $0 <vm_ip> <scenario> <exp_dir>"
    exit 1
fi

VM_IP="$1"
SCENARIO="$2"
EXPDIR="$3"

OUTDIR="$EXPDIR/live_baseline"
mkdir -p "$OUTDIR"

echo "[+] Collecting live baseline from $VM_IP..."

# System info
ssh root@$VM_IP "uname -a" > "$OUTDIR/uname.txt"
ssh root@$VM_IP "cat /etc/os-release" > "$OUTDIR/os-release.txt"
ssh root@$VM_IP "uptime" > "$OUTDIR/uptime.txt"

# Processes
ssh root@$VM_IP "ps auxww" > "$OUTDIR/ps.txt"
ssh root@$VM_IP "lsof -n" > "$OUTDIR/lsof.txt" 2>&1
ssh root@$VM_IP "lsmod" > "$OUTDIR/lsmod.txt"

# Network
ssh root@$VM_IP "ss -tunapo" > "$OUTDIR/ss.txt" 2>&1
ssh root@$VM_IP "ip addr" > "$OUTDIR/ip-addr.txt"
ssh root@$VM_IP "ip route" > "$OUTDIR/ip-route.txt"
ssh root@$VM_IP "arp -a" > "$OUTDIR/arp.txt"

# Users
ssh root@$VM_IP "cat /etc/passwd" > "$OUTDIR/passwd.txt"
ssh root@$VM_IP "last -F" > "$OUTDIR/last.txt"
ssh root@$VM_IP "w" > "$OUTDIR/w.txt"

# Files
ssh root@$VM_IP "find /etc -type f -mtime -7" > "$OUTDIR/etc-recent.txt" 2>&1
ssh root@$VM_IP "find /tmp -ls" > "$OUTDIR/tmp-files.txt" 2>&1
ssh root@$VM_IP "find /var/tmp -ls" > "$OUTDIR/var-tmp-files.txt" 2>&1
ssh root@$VM_IP "find / -perm -4000 -type f" > "$OUTDIR/suid-files.txt" 2>&1

# Persistence
ssh root@$VM_IP "crontab -l" > "$OUTDIR/root-crontab.txt" 2>&1
ssh root@$VM_IP "cat /etc/crontab" > "$OUTDIR/etc-crontab.txt" 2>&1
ssh root@$VM_IP "systemctl list-units" > "$OUTDIR/systemd-units.txt"
ssh root@$VM_IP "cat /etc/rc.local" > "$OUTDIR/rc-local.txt" 2>&1

# LD_PRELOAD check (rootkit indicator)
ssh root@$VM_IP "cat /etc/ld.so.preload" > "$OUTDIR/ld.so.preload.txt" 2>&1
ssh root@$VM_IP "env | grep -i preload" > "$OUTDIR/env-preload.txt"

# Logs (last 100 lines)
ssh root@$VM_IP "tail -100 /var/log/syslog" > "$OUTDIR/syslog-tail.txt" 2>&1
ssh root@$VM_IP "tail -100 /var/log/auth.log" > "$OUTDIR/auth-tail.txt" 2>&1
ssh root@$VM_IP "journalctl -n 100" > "$OUTDIR/journal-tail.txt" 2>&1

echo "[+] Live baseline complete: $OUTDIR"
echo "    Files collected: $(ls $OUTDIR | wc -l)"
