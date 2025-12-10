#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}Running Part D mutator setup...${NC}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

AFL_DIR="${PROJECT_ROOT}/part-b-afl/AFLplusplus"
HARNESS_BIN="${PROJECT_ROOT}/part-c-asan-ubsan/png_harness"

MUTATOR_SRC="${SCRIPT_DIR}/png_mutator.c"
MUTATOR_SO="${SCRIPT_DIR}/png_mutator.so"

SEEDS_DIR="${SCRIPT_DIR}/seeds"
OUTPUT_DIR="${SCRIPT_DIR}/findings"

FUZZ_DURATION=3600

echo -e "${YELLOW}checking files...${NC}"

if [ ! -d "${AFL_DIR}" ]; then
    echo -e "${RED}could not find AFL++ at: ${AFL_DIR}${NC}"
    exit 1
fi

echo "afl directory ok"

if [ ! -f "${MUTATOR_SRC}" ]; then
    echo -e "${RED}mutator source missing -> ${MUTATOR_SRC}${NC}"
    exit 1
fi

echo "mutator source found"

if [ ! -f "${HARNESS_BIN}" ]; then
    echo -e "${RED}harness not found: ${HARNESS_BIN}${NC}"
    echo "searching around..."
    find "${PROJECT_ROOT}" -name "*harness*" -type f 2>/dev/null || true
    exit 1
fi

echo "harness file ok"

if [ ! -d "${SEEDS_DIR}" ]; then
    mkdir -p "${SEEDS_DIR}"
    echo -e "${RED}no seeds in ${SEEDS_DIR}${NC}"
    exit 1
fi

SEED_COUNT=$(find "${SEEDS_DIR}" -type f \( -name "*.png" -o -name "*.PNG" \) 2>/dev/null | wc -l)

if [ "$SEED_COUNT" -eq 0 ]; then
    SEED_COUNT=$(find "${SEEDS_DIR}" -type f | wc -l)
fi

if [ "$SEED_COUNT" -eq 0 ]; then
    echo -e "${RED}no seed files found in seeds folder${NC}"
    exit 1
fi

echo "found $SEED_COUNT seeds"

mkdir -p "${OUTPUT_DIR}"
echo "output dir ready: ${OUTPUT_DIR}"

echo -e "${YELLOW}building mutator...${NC}"

gcc -shared -Wall -O3 -fPIC "${MUTATOR_SRC}" -o "${MUTATOR_SO}" -lz

if [ ! -f "${MUTATOR_SO}" ]; then
    echo -e "${RED}mutator compile error${NC}"
    exit 1
fi

echo "mutator compiled ok"

echo -e "${YELLOW}setting environment...${NC}"

export PATH="${AFL_DIR}:${PATH}"
export AFL_CUSTOM_MUTATOR_LIBRARY="${MUTATOR_SO}"
export ASAN_OPTIONS="abort_on_error=1:symbolize=0:detect_leaks=0"
export UBSAN_OPTIONS="halt_on_error=1"

echo "env ready"

echo -e "${GREEN}starting fuzzing...${NC}"
echo "duration = ${FUZZ_DURATION}s"
echo "using seeds from: ${SEEDS_DIR}"
echo "saving output to: ${OUTPUT_DIR}"

"${AFL_DIR}/afl-fuzz" \
    -i "${SEEDS_DIR}" \
    -o "${OUTPUT_DIR}" \
    -t 1000 \
    -m none \
    -- "${HARNESS_BIN}" @@ /dev/null || true

echo ""
echo "fuzzing done."

CRASH_COUNT=0
HANG_COUNT=0

if [ -d "${OUTPUT_DIR}/default/crashes" ]; then
    CRASH_COUNT=$(find "${OUTPUT_DIR}/default/crashes" -type f ! -name "README.txt" | wc -l)
fi

if [ -d "${OUTPUT_DIR}/default/hangs" ]; then
    HANG_COUNT=$(find "${OUTPUT_DIR}/default/hangs" -type f | wc -l)
fi

echo ""
echo "crashes: $CRASH_COUNT"
echo "hangs:   $HANG_COUNT"
echo ""

if [ -f "${OUTPUT_DIR}/default/fuzzer_stats" ]; then
    grep -E "^(execs_done|execs_per_sec|corpus_count|saved_crashes|saved_hangs|bitmap_cvg)" \
        "${OUTPUT_DIR}/default/fuzzer_stats" || true
fi

echo ""
echo "done."
echo ""
