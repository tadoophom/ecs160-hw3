#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

AFL_PATH="${PROJECT_ROOT}/part-b-afl/AFLplusplus"
HARNESS_SRC="${PROJECT_ROOT}/part-a-harness/png_harness.c"
MUTATOR_SRC="${SCRIPT_DIR}/custom_mutator.c"
SEEDS_DIR="${SCRIPT_DIR}/seeds"
OUTPUT_DIR="${SCRIPT_DIR}/out_part_d"
HARNESS_BIN="${SCRIPT_DIR}/harness_asan"
MUTATOR_LIB="${SCRIPT_DIR}/custom_mutator.so"

export PATH="${AFL_PATH}:${PATH}"
export SDKROOT="$(xcrun --show-sdk-path)"

afl-clang-fast \
    -fsanitize=address,undefined \
    -isysroot "${SDKROOT}" \
    -g \
    "${HARNESS_SRC}" \
    -o "${HARNESS_BIN}" \
    $(pkg-config --cflags --libs libpng)

gcc \
    -shared \
    -isysroot "${SDKROOT}" \
    -Wall -O3 -fPIC \
    "${MUTATOR_SRC}" \
    -o "${MUTATOR_LIB}" \
    -lz

export AFL_CUSTOM_MUTATOR_LIBRARY="${MUTATOR_LIB}"
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
export ASAN_OPTIONS="abort_on_error=1:symbolize=0:detect_leaks=0"
export UBSAN_OPTIONS="halt_on_error=1"

afl-fuzz \
    -i "${SEEDS_DIR}" \
    -o "${OUTPUT_DIR}" \
    -m none \
    -- "${HARNESS_BIN}" @@ /dev/null