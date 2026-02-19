## Workflow

### 1. Acquisition

**Full acquisition (memory + disk):**
```bash
cd ~/thesis/scripts
./acquire.sh <vm_name> <scenario>
```

**Memory only:**

```bash
./acquire.sh <vm_name> <scenario> --memory
```

**Disk only (requires existing experiment):**

```bash
./acquire.sh <vm_name> <scenario> --disk
```

**Example:**

```bash
./acquire.sh ubuntu22 baseline_u22
```

Output: `experiments/baseline_u22/exp_001/`

### 2. Analysis

**Memory analysis:**

```bash
./mem_analyze.sh <dump_file> <output_dir>
```

**Disk analysis:**

```bash
./disk_analyze.sh <disk_image> <output_dir>
```

**Example:**

```bash
EXPDIR=../experiments/baseline_u22/exp_001

./mem_analyze.sh $EXPDIR/memory.dump $EXPDIR/memory_analysis
./disk_analyze.sh $EXPDIR/disk.E01 $EXPDIR/disk_analysis
```


### 3. Documentation

**Complete manifest Notes section:**

```bash
nano experiments/baseline_u22/exp_001/manifest.txt
```

Add experiment-specific notes at the end.

## Experiment Scenarios

- `baseline_u22`: Clean Ubuntu 22.04 reference
- `baseline_u24`: Clean Ubuntu 24.04 reference
- `father_u22`: Father rootkit (LD_PRELOAD)
- `ftrace_u22`: Ftrace hook rootkit
- `ptrace_fa_u24`: Ptrace foreign allocation
- `diamorphine_u22`: Diamorphine LKM rootkit


## Output Files

**Memory:**

- `memory.dump`: RAM dump (ELF core)
- `memory.dump.sha256`: Integrity hash
- `memory_analysis/*.txt`: Volatility plugin outputs

**Disk:**

- `disk.E01`: Disk image (EWF compressed)
- `disk_verify.log`: Verification status
- `disk_analysis/*.txt`: TSK command outputs

**Metadata:**

- `manifest.txt`: Acquisition metadata and timeline


## Low-Level Usage

Scripts can be used independently for custom workflows:

```bash
# Manual experiment creation
mkdir -p ../experiments/custom/exp_001

# Direct acquisition
./mem_acquire.sh ubuntu22 ../experiments/custom/exp_001
./disk_acquire.sh ubuntu22 ../experiments/custom/exp_001

# Analysis
./mem_analyze.sh ../experiments/custom/exp_001/memory.dump \
                 ../experiments/custom/exp_001/memory_analysis
```

## **QUICK TEST**

```bash
cd ~/thesis/scripts

# Test baseline completo
./acquire.sh ubuntu22 test_baseline

# Verifica output
tree ../experiments/test_baseline/exp_001/

# Expected:
# exp_001/
# ├── memory.dump
# ├── memory.dump.sha256
# ├── disk.E01
# ├── disk_verify.log
# └── manifest.txt

# Cleanup test
rm -rf ../experiments/test_baseline
```


***