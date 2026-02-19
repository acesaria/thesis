# Ubuntu 24.04 Forensics VM - Installation and configuration Guide

## 0. Pre-requisites: Setup Storage Pool ISO and Network

### 0.1. Storage Pool ISO configuration

```bash
sudo virsh pool-define-as iso dir \
  --source-path /var/lib/libvirt/iso \
  --target /var/lib/libvirt/iso

sudo virsh pool-start iso

sudo virsh pool-autostart iso

sudo virsh pool-refresh iso
```

Verify:
```bash
sudo virsh vol-list iso # Expected: ubuntu-24.04, ubuntu-22.04, debian-12
```

### 0.2. Host only network config

```bash
cd forensics-project/vm/network

sudo virsh net-define hostonly.xml

sudo virsh net-start hostonly

sudo virsh net-autostart hostonly

sudo virsh net-list --all # Expected: default (active), hostonly (active)
```

---

## 1. VM Installation (virt-install)

```bash
sudo virt-install \
  --name ubuntu24-forensics \
  --ram 8192 \
  --vcpus 4 \
  --disk path=/var/lib/libvirt/images/ubuntu24-forensics.qcow2,size=50 \
  --os-variant ubuntu24.04 \
  --network network=hostonly \
  --cdrom /var/lib/libvirt/iso/ubuntu-24.04.3-live-server-amd64.iso \
  --graphics vnc
```

After `virt-viewer`, proceeds with next steps.

---

## 2. Installer Configuration (virt-viewer)

### Welcome / Language
- Language: **English**

### Keyboard
- Layout: **Italian**

### Type of Install
- [X] **Ubuntu Server**

### Network Connections
- Select default configuration
- If asked, select "continue without network"

### Configure Proxy
- Proxy address: **(empty)**

### Configure Ubuntu Archive Mirror
- Mirror address: **default**

### Guided Storage Configuration
- Storage layout: **Use an entire disk**
- **[ ] Set up this disk as an LVM group** ← **DESELECT**
- Confirm disk distruction

### Profile Setup
- Your name: **Lab User**
- Your server's name: **ubuntu24**
- Pick a username: **labuser**
- Choose a password: **labuser**
- Confirm password: **labuser**

### Upgrade to Ubuntu Pro
- [X] **Skip for now**

### SSH Setup
- [X] **Install OpenSSH server**
- [ ] Import SSH identity: **(empty)**

### Featured Server Snaps
- Don't select (minimal system)

### Installation
- Wait for completion (~5–10 mins)

### Reboot
- **Reboot Now**

---

## 3. Post-installazione: System update and SSH configuration

After reboot, VM with network interface hostonly **won't have Internet connection**.

In order to do updates, must **enable temporarily NAT network**.

### 3.1. Host side: enable NAT network (default)

```bash
sudo virsh net-autostart default

sudo virsh net-start default  # if not active

sudo virsh net-list --all     # verify 'default' is active
```

### 3.2. Add NAT interface into VM (temporarily)

Use Virtual Machine Manager (GUI) in order to attach NAT interface alla VM:

1. Right click on ubuntu24-forensics → Open
2. Show virtual hardware details (icona "i")
3. Sidebar: NIC xxx:xx:xx
4. Network source: hostonly → default: NAT
5. Apply
6. reboot required


### 3.3. Inside VM: login, test and update

```bash
# Login from virt-viewer console
ubuntu24 login: labuser
Password: labuser

# Verify interface
ip addr

# Test connectivity
ping -c 3 archive.ubuntu.com

# System update
sudo apt update
sudo apt upgrade -y

## Install build-essential (make)
sudo apt -y install build-essential

```

### 3.4. Disable NAT network

From Virtual Machine Manager, re-enable host-only network

---

## 4. SSH-passwordless configuration

### 4.1. On host: generate SSH key (if not present)

```bash
ssh-keygen -t ed25519 -C "forensics-lab"
# Press ENTER (password empty)
# Key generated in: ~/.ssh/id_ed25519 e ~/.ssh/id_ed25519.pub
```

### 4.2. Copy key into VM

```bash
ssh-copy-id labuser@192.168.100.X
# Substitute X with VM hostonly network IP
# Insert password: labuser
```

### 4.3. Test login passwordless

```bash
ssh labuser@192.168.100.X
# Should login WITHOUT password
```

### 4.4. (Optional) Simplify SSH config on host

```bash
cat >> ~/.ssh/config <<EOF
Host ubuntu24
    HostName 192.168.100.X
    User labuser
    StrictHostKeyChecking no
    UserKnownHostsFile /dev/null
EOF
```

Now you can login easily:
```bash
ssh ubuntu24
```

---

## 6. Snapshot clean-vanilla

Once VM is correctly configured (OS updated + SSH key auth + build-essential), **creates pristine snapshot**:

```bash
# Shutoff VM (from VM)
sudo poweroff

# Or from host
sudo virsh shutdown ubuntu24-forensics

# Wait for "shut off"
sudo virsh list --all

# Create pristine/vanilla snapshot
sudo virsh snapshot-create-as ubuntu24-forensics clean-vanilla \
  "Pristine Ubuntu 24.04 - updated + build-essential installed + SSH key auth configured"

# Verifify
sudo virsh snapshot-list ubuntu24-forensics
```

This is the **clean baseline** for all the experiments.

---
## 7. ISF generation (Volatility3 symbols)
To use the Volatility3 framework, you need to generate ISF for each distro. First, you need to obtain kernel debug symbols. To do this, temporarily enable NAT and run the following commands:

On VM ubuntu :
```bash
# Verify kernel
uname -r

# 1. Install il official keyring (handle key automatically)
sudo apt install ubuntu-dbgsym-keyring

# 2. Add ddebs repo
echo "Types: deb
URIs: http://ddebs.ubuntu.com/
Suites: $(lsb_release -cs) $(lsb_release -cs)-updates
Components: main restricted universe multiverse
Signed-by: /usr/share/keyrings/ubuntu-dbgsym-keyring.gpg" | \
sudo tee /etc/apt/sources.list.d/ddebs.sources

# 3. Update and install debug symbols kernel
sudo apt update
sudo apt install linux-image-$(uname -r)-dbgsym

# 4. Find vmlinux
find /usr/lib/debug -name "vmlinux*"

# 5. Copy file on host and execute dwarf2json. Save file in volatility3/volatility3/symbols/linux
dwarf2json linux --elf /tmp/vmlinux-6.8.0-90-generic  > ubuntu-6.8.0-90-generic.json
```

## 8. Experiments workflow

Before every experiment:

```bash
# 1. Revert a clean-vanilla
virsh snapshot-revert ubuntu24-forensics clean-vanilla

# 2. Start VM
virsh start ubuntu24-forensics

# 3. Deploy exploit
scp ~/thesis/attack_tools/exploit ubuntu-22:/tmp/
ssh ubuntu24

# [execute exploit from VM]

# 4. Acquire memory and disk
./scripts/acquisition/acquire.sh ubuntu22-forensics baseline_u22  

# 5. Analysis
./scripts/analysis/mem_analyze.sh experiments/baseline_u22/memory.dump experiments/baseline_u22/mem_analysis 

./scripts/analysis/disk_analyze.sh experiments/baseline_u22/disk.E01 experiments/baseline_u22/disk_analysis

# 6. Revert
virsh snapshot-revert ubuntu24-forensics clean-vanilla
```

---
