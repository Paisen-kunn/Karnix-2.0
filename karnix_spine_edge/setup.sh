#!/bin/bash
# =============================================================================
#  Karnix Spine Edge – setup.sh
#  One-shot build + end-to-end test runner
#
#  What this script does:
#    1.  Verifies prerequisites  (clang++, sips, python3)
#    2.  Creates required directories  (data/images/, models/)
#    3.  Generates synthetic grayscale test images via python3
#         normal_test.pgm   – bright region (simulates healthy tissue)
#         abnormal_test.pgm – dark region   (simulates anomalous tissue)
#    4.  Compiles spine_cli.c++ with clang++ -std=gnu++14
#    5.  Runs ./spine_cli end-to-end
#    6.  Prints a pass/fail summary
#
#  Usage:
#    chmod +x setup.sh
#    ./setup.sh
#
#  To keep generated test images after the run (default: they are removed):
#    KEEP_TEST_IMAGES=1 ./setup.sh
# =============================================================================

set -euo pipefail

# ── Colours ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

pass() { echo -e "${GREEN}[PASS]${RESET} $1"; }
fail() { echo -e "${RED}[FAIL]${RESET} $1"; }
info() { echo -e "${CYAN}[INFO]${RESET} $1"; }
warn() { echo -e "${YELLOW}[WARN]${RESET} $1"; }
step() { echo -e "\n${BOLD}── $1${RESET}"; }

KEEP_TEST_IMAGES="${KEEP_TEST_IMAGES:-0}"
ERRORS=0

# ── Resolve script directory so the script works from any working directory ───
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
info "Working directory: $SCRIPT_DIR"

# =============================================================================
#  STEP 1 – Prerequisite check
# =============================================================================
step "Step 1: Checking prerequisites"

check_tool() {
    local tool="$1"
    local install_hint="$2"
    if command -v "$tool" &>/dev/null; then
        pass "$tool found: $(command -v "$tool")"
    else
        fail "$tool not found.  $install_hint"
        ERRORS=$((ERRORS + 1))
    fi
}

check_tool clang++ "Install Xcode Command Line Tools: xcode-select --install"
check_tool sips    "sips ships with every macOS. If missing, reinstall macOS."
check_tool python3 "Install Python 3: brew install python3"

if [[ $ERRORS -gt 0 ]]; then
    echo -e "\n${RED}Prerequisite check failed. Fix the above errors and re-run.${RESET}"
    exit 1
fi

# Show clang++ version
info "Compiler: $(clang++ --version 2>&1 | head -1)"

# =============================================================================
#  STEP 2 – Create required directories
# =============================================================================
step "Step 2: Creating directories"

mkdir -p data/images
mkdir -p models
pass "data/images/  created (or already exists)"
pass "models/       created (or already exists)"

# =============================================================================
#  STEP 3 – Generate synthetic test images
# =============================================================================
step "Step 3: Generating synthetic test images"

python3 - <<'PYEOF'
import struct, random, os

random.seed(42)

def make_pgm(path, base_value, size=64, noise=20):
    """Write a binary PGM (P5) grayscale image."""
    with open(path, 'wb') as f:
        header = f"P5\n{size} {size}\n255\n"
        f.write(header.encode('ascii'))
        for _ in range(size * size):
            v = base_value + random.randint(-noise, noise)
            v = max(0, min(255, v))
            f.write(struct.pack('B', v))

# normal_test  – higher brightness (bright tissue pattern)
make_pgm("data/images/normal_test.pgm",   base_value=200, size=64)

# abnormal_test – lower brightness (darker anomalous region)
make_pgm("data/images/abnormal_test.pgm", base_value=55,  size=64)

print("[PASS] normal_test.pgm   created  (64×64, bright)")
print("[PASS] abnormal_test.pgm created  (64×64, dark)")
PYEOF

# =============================================================================
#  STEP 4 – Compile
# =============================================================================
step "Step 4: Compiling spine_cli.c++"

info "Command: clang++ -std=gnu++14 spine_cli.c++ -o spine_cli"

# Capture warnings but do not fail on them
COMPILE_OUTPUT=$(clang++ -std=gnu++14 spine_cli.c++ -o spine_cli 2>&1) || {
    fail "Compilation failed."
    echo "$COMPILE_OUTPUT"
    ERRORS=$((ERRORS + 1))
}

if [[ $ERRORS -eq 0 ]]; then
    pass "Compilation succeeded → ./spine_cli"
    # Print pre-existing Karnix core warnings as informational only
    if [[ -n "$COMPILE_OUTPUT" ]]; then
        warn "Pre-existing Karnix core warnings (non-fatal):"
        echo "$COMPILE_OUTPUT" | sed 's/^/        /'
    fi
else
    echo -e "\n${RED}Build failed. Aborting.${RESET}"
    exit 1
fi

# =============================================================================
#  STEP 5 – Run end-to-end
# =============================================================================
step "Step 5: Running ./spine_cli (end-to-end test)"

echo ""
echo "──────────────────────────────────────────────────────────────────"
./spine_cli
EXIT_CODE=$?
echo "──────────────────────────────────────────────────────────────────"
echo ""

if [[ $EXIT_CODE -eq 0 ]]; then
    pass "spine_cli exited with code 0"
else
    fail "spine_cli exited with code $EXIT_CODE"
    ERRORS=$((ERRORS + 1))
fi

# =============================================================================
#  STEP 6 – Verify outputs
# =============================================================================
step "Step 6: Verifying outputs"

# Weights file
if [[ -f "models/spine_model.weights" ]]; then
    WEIGHTS_SIZE=$(wc -c < "models/spine_model.weights" | tr -d ' ')
    pass "models/spine_model.weights  exists  (${WEIGHTS_SIZE} bytes)"
else
    fail "models/spine_model.weights  NOT found"
    ERRORS=$((ERRORS + 1))
fi

# JSON records
if [[ -f "data/spine_records.json" ]]; then
    RECORD_COUNT=$(grep -c '"patient_id"' data/spine_records.json 2>/dev/null || echo 0)
    pass "data/spine_records.json     exists  (${RECORD_COUNT} record(s))"
else
    fail "data/spine_records.json     NOT found"
    ERRORS=$((ERRORS + 1))
fi

# =============================================================================
#  STEP 7 – Cleanup (optional)
# =============================================================================
step "Step 7: Cleanup"

if [[ "$KEEP_TEST_IMAGES" == "1" ]]; then
    warn "KEEP_TEST_IMAGES=1 – test images retained in data/images/"
else
    rm -f data/images/normal_test.pgm data/images/abnormal_test.pgm
    info "Synthetic test images removed."
    info "To keep them, run:  KEEP_TEST_IMAGES=1 ./setup.sh"
fi

# Leave the compiled binary and weights/records in place for user exploration

# =============================================================================
#  Summary
# =============================================================================
echo ""
echo "══════════════════════════════════════════════════════════════════"
if [[ $ERRORS -eq 0 ]]; then
    echo -e "${GREEN}${BOLD}  ALL CHECKS PASSED — Karnix Spine Edge is working correctly.${RESET}"
    echo ""
    echo "  To run with your own images:"
    echo "    1. Copy labelled images into data/images/"
    echo "       Naming: normal_XXXX.jpg  or  abnormal_XXXX.jpg"
    echo "    2. ./spine_cli"
    echo ""
    echo "  Supported formats: PPM · PGM · JPEG · PNG · TIFF · BMP"
else
    echo -e "${RED}${BOLD}  $ERRORS CHECK(S) FAILED — Review errors above.${RESET}"
fi
echo "══════════════════════════════════════════════════════════════════"
echo ""

exit $ERRORS
