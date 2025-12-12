#!/bin/bash
set -e

ROOT="$(pwd)"
WORK="$ROOT/fuzz_build"
HARNESS="$ROOT/part-a-harness/png_harness.c"
MUTATOR="$ROOT/part-d-mutator/custom_mutator.c"

mkdir -p "$WORK"

if [ ! -d "$WORK/AFLplusplus" ]; then
    git clone https://github.com/AFLplusplus/AFLplusplus.git "$WORK/AFLplusplus"
fi
cd "$WORK/AFLplusplus"
[ -x "$(command -v llvm-config)" ] || export LLVM_CONFIG="/opt/homebrew/opt/llvm/bin/llvm-config"
make source-only
AFL_CC="$WORK/AFLplusplus/afl-cc"

if [ ! -d "$WORK/libpng" ]; then
    git clone https://github.com/pnggroup/libpng.git "$WORK/libpng"
fi
cd "$WORK/libpng"

echo "[*] Building Part B..."
git clean -fdx
autoreconf -fi
CC="$AFL_CC" ./configure --disable-shared --prefix="$WORK/install_b"
make -j$(sysctl -n hw.ncpu) install
"$AFL_CC" "$HARNESS" -I"$WORK/install_b/include" "$WORK/install_b/lib/libpng.a" -lz -o "$WORK/harness_part_b"

echo "[*] Building Part C..."
git clean -fdx
autoreconf -fi
export AFL_USE_ASAN=1 AFL_USE_UBSAN=1
CC="$AFL_CC" ./configure --disable-shared --prefix="$WORK/install_c"
make -j$(sysctl -n hw.ncpu) install
"$AFL_CC" "$HARNESS" -I"$WORK/install_c/include" "$WORK/install_c/lib/libpng.a" -lz -o "$WORK/harness_part_c"
unset AFL_USE_ASAN AFL_USE_UBSAN

echo "[*] Building Part D..."
gcc -shared -Wall -O3 -fPIC "$MUTATOR" -o "$WORK/custom_mutator.so" -lz

echo "[+] Done! Targets are in $WORK"