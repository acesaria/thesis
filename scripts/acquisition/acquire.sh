#!/bin/bash
# Forensic acquisition wrapper
# Usage: ./acquire.sh <vm> <scenario> [--memory|--disk|--all]

set -euo pipefail

# Auto-locate project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
EXPERIMENTS_DIR="$PROJECT_ROOT/experiments"

usage() {
    cat << EOF
Usage: $(basename "$0") <vm_name> <scenario> [options]

Options:
  --memory    Acquire memory only
  --disk      Acquire disk only
  --all       Acquire both (default)

Examples:
  $(basename "$0") ubuntu22 baseline_u22
  $(basename "$0") ubuntu22 father_u22 --memory
EOF
    exit 1
}

[ $# -lt 2 ] && usage

VM="$1"
SCENARIO="$2"
MODE="${3:---all}"

case "$MODE" in
    --memory|--disk|--all) ;;
    *) echo "[-] Invalid option: $MODE"; usage ;;
esac

EXPDIR="$EXPERIMENTS_DIR/${SCENARIO}"
mkdir -p "$EXPDIR"

echo "=========================================="
echo "Forensic Acquisition"
echo "=========================================="
echo "VM:       $VM"
echo "Scenario: $SCENARIO"
echo "Mode:     $MODE"
echo "Output:   $EXPDIR"
echo "=========================================="
echo ""

cat > "$EXPDIR/manifest.txt" <<HEADER
Experiment Information:
-----------------------
Scenario:        $SCENARIO
VM Name:         $VM
Analyst:         $(whoami)@$(hostname)
Case:            Thesis - Linux Rootkit Detection
Date:            $(date '+%Y-%m-%d')

HEADER

if [ "$MODE" = "--memory" ] || [ "$MODE" = "--all" ]; then
    echo "[+] Acquiring memory..."
    "$SCRIPT_DIR/mem_acquire.sh" "$VM" "$EXPDIR" || exit 1
    echo ""
fi

if [ "$MODE" = "--disk" ] || [ "$MODE" = "--all" ]; then
    echo "[+] Acquiring disk..."
    "$SCRIPT_DIR/disk_acquire.sh" "$VM" "$EXPDIR" || exit 1
    echo ""
fi

echo "=========================================="
echo "Acquisition complete"
echo "=========================================="
echo "Location: $EXPDIR"
echo ""
echo "Next steps:"
[ -f "$EXPDIR/memory.dump" ] && echo "  vol3 -f $EXPDIR/memory.dump linux.pslist"
[ -f "$EXPDIR/disk.E01" ] && echo "  mmls $EXPDIR/disk.E01"
echo ""
echo "Edit Notes: nano $EXPDIR/manifest.txt"
