#!/bin/bash
# Vol3 batch analysis with clean output

if [ $# -ne 2 ]; then
    echo "Usage: $0 <dump_file> <output_dir>"
    exit 1
fi

DUMP="$1"
OUTDIR="$2"

[ ! -f "$DUMP" ] && echo "Dump not found: $DUMP" && exit 1

mkdir -p "$OUTDIR"
echo "[+] Analyzing: $(basename $DUMP)"

# Plugin list
PLUGINS=(
    "linux.pslist.PsList"
    "linux.pstree.PsTree"
    "linux.envars.Envars"
    "linux.proc.Maps"
    "linux.bash.Bash"
    "linux.lsof.Lsof"
    "linux.sockstat.Sockstat"
    "linux.sockscan.Sockscan"
    "linux.lsmod.Lsmod"
    "linux.elfs.Elfs"
    "linux.malware.malfind"
)

for plugin in "${PLUGINS[@]}"; do
    name=$(echo $plugin | cut -d'.' -f3 | tr '[:upper:]' '[:lower:]')
    echo "  [+] $name..."
    
    # Clean output: suppress progress, remove carriage returns
    vol3 -q -f "$DUMP" "$plugin" 2>/dev/null  > "$OUTDIR/${name}.txt"
done

echo "[+] Done: $OUTDIR/"
