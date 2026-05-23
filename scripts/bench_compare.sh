#!/usr/bin/env bash
# bench_compare.sh — unified C++ vs Python UIED benchmark comparison
# Usage: ./scripts/bench_compare.sh [--reps N]   (default: 5 C++ reps, 30 Python iters)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
VENV_DIR="$SCRIPT_DIR/.venv"
UIED_ROOT="$(cd "$REPO_ROOT/../UIED" 2>/dev/null && pwd)" || true
CPP_REPS=5

# ── Parse args ──────────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case $1 in
        --reps) CPP_REPS="$2"; shift 2 ;;
        *) echo "Unknown arg: $1" >&2; exit 1 ;;
    esac
done

RED='\033[0;31m'; GRN='\033[0;32m'; YLW='\033[1;33m'
CYN='\033[0;36m'; BLD='\033[1m'; RST='\033[0m'

bar() { printf '%*s' "$1" '' | tr ' ' '─'; }

# ── Step 1: build C++ if needed ─────────────────────────────────────────────
echo -e "\n${BLD}[1/3] Building C++ targets...${RST}"
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF \
    --log-level=ERROR -Wno-dev 2>/dev/null
cmake --build "$BUILD_DIR" --parallel 2>/dev/null
echo -e "    ${GRN}✓ Build OK${RST}"

# ── Step 2: run C++ benchmark, capture JSON output ──────────────────────────
echo -e "\n${BLD}[2/3] Running C++ benchmark (${CPP_REPS} repetitions)...${RST}"
# Run from build/ so the benchmark can resolve ../assets/test_ui.png
CPP_JSON=$(cd "$BUILD_DIR" && ./run_benchmark \
    --benchmark_repetitions="$CPP_REPS" \
    --benchmark_report_aggregates_only=true \
    --benchmark_format=json \
    2>/dev/null)

# Parse mean values (ms) from JSON using python3
parse_cpp() {
    local name="${1}_mean"
    echo "$CPP_JSON" | python3 -c "
import sys, json
data = json.load(sys.stdin)
for b in data.get('benchmarks', []):
    if b['name'] == '$name' and not b.get('error_occurred', False):
        print(f\"{b['real_time']:.3f}\")
        break
"
}

CPP_P1=$(parse_cpp "BM_Phase1_Binarization")
CPP_P2=$(parse_cpp "BM_Phase2_FullDetection")
CPP_FP=$(parse_cpp "BM_FullPipeline")
echo -e "    ${GRN}✓ C++ done${RST}  (Phase1=${CPP_P1}ms  Phase2=${CPP_P2}ms  Full=${CPP_FP}ms)"

# ── Step 3: run Python benchmark ────────────────────────────────────────────
echo -e "\n${BLD}[3/3] Running Python benchmark...${RST}"

if [[ -z "${UIED_ROOT:-}" || ! -d "$UIED_ROOT" ]]; then
    echo -e "    ${YLW}⚠ UIED not found at $REPO_ROOT/../UIED — skipping Python.${RST}"
    PY_SKIP=1
else
    PY_SKIP=0
    # Setup venv with opencv if missing
    if [[ ! -f "$VENV_DIR/bin/python3" ]]; then
        echo "    Creating Python venv..."
        python3 -m venv "$VENV_DIR"
        "$VENV_DIR/bin/pip" install -q opencv-python-headless numpy
    fi

    PY_OUT=$(PYTHONPATH="$UIED_ROOT" "$VENV_DIR/bin/python3" \
        "$SCRIPT_DIR/bench_python.py" 2>/dev/null) || {
        echo -e "    ${RED}✗ Python benchmark failed${RST}"
        PY_SKIP=1
    }

    if [[ $PY_SKIP -eq 0 ]]; then
        PY_P1=$(echo "$PY_OUT" | awk '/BM_Phase1_Binarization/ {printf "%.2f", $2}')
        PY_P2=$(echo "$PY_OUT" | awk '/BM_Phase2_FullDetection/ {printf "%.2f", $2}')
        PY_FP=$(echo "$PY_OUT" | awk '/BM_FullPipeline/         {printf "%.2f", $2}')
        echo -e "    ${GRN}✓ Python done${RST}  (Phase1=${PY_P1}ms  Phase2=${PY_P2}ms  Full=${PY_FP}ms)"
    fi
fi

# ── Print comparison table ───────────────────────────────────────────────────
speedup() {
    local py=$1 cpp=$2
    awk -v p="$py" -v c="$cpp" 'BEGIN {
        if (c > 0) printf "%.0fx", p/c
        else printf "N/A"
    }'
}

SP_P1=$(speedup "$PY_P1" "$CPP_P1")
SP_P2=$(speedup "$PY_P2" "$CPP_P2")
SP_FP=$(speedup "$PY_FP" "$CPP_FP")

echo ""
echo -e "${BLD}$(bar 70)${RST}"
printf "${BLD}  %-32s  %10s  %10s  %10s${RST}\n" "Benchmark" "Python (ms)" "C++ (ms)" "Speedup"
echo -e "${BLD}$(bar 70)${RST}"

row() {
    local label="$1" py="$2" cpp="$3" sp="$4"
    if [[ $PY_SKIP -eq 1 ]]; then
        printf "  %-32s  %10s  %10s  %10s\n" "$label" "N/A" "${cpp}" "N/A"
    else
        printf "  %-32s  %10s  %10s  " "$label" "${py}" "${cpp}"
        echo -e "${GRN}${BLD}${sp}${RST}"
    fi
}

row "Phase 1  (Binarization)"        "$PY_P1" "$CPP_P1" "$SP_P1"
row "Phase 2  (Full Detection)"      "$PY_P2" "$CPP_P2" "$SP_P2"
row "Full Pipeline (Phase 1 + 2)"    "$PY_FP" "$CPP_FP" "$SP_FP"

echo -e "${BLD}$(bar 70)${RST}"
echo -e "  Image: assets/test_ui.png (1024×582)  |  C++ reps: ${CPP_REPS}  |  Python iters: 30"
echo -e "${BLD}$(bar 70)${RST}\n"
