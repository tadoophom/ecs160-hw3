# ECS160 HW3 (AFL++ + libpng)

AI GENERATED

## Setup (macOS/Homebrew)

```sh
brew install llvm lld pkg-config libpng
```

If you want to use a locally built AFL++ (instead of the build scripts), build it and add it to your PATH:

```sh
git clone https://github.com/AFLplusplus/AFLplusplus.git part-b-afl/AFLplusplus
cd part-b-afl/AFLplusplus
LLVM_CONFIG=/opt/homebrew/opt/llvm/bin/llvm-config make source-only

export PATH="$(pwd):$PATH"
export LLVM_CONFIG=/opt/homebrew/opt/llvm/bin/llvm-config
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:${PKG_CONFIG_PATH}"

afl-fuzz -h | head -n 1
afl-cc -version
pkg-config --modversion libpng
```

## Build targets (macOS / Linux)

```sh
./build_target.sh
```

Outputs are written to `fuzz_build/`.

## Run AFL++ (Parts B/C)

```sh
# Part B: no seeds
mkdir -p part-b-afl/in_noseeds
afl-fuzz -i part-b-afl/in_noseeds -o part-b-afl/out_b_no_seeds -m none -- fuzz_build/harness_part_b @@ /dev/null

# Part B: with seeds
afl-fuzz -i part-b-afl/in_seeds -o part-b-afl/out_b_with_seeds -m none -- fuzz_build/harness_part_b @@ /dev/null

# Part C: ASAN + UBSAN
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
export ASAN_OPTIONS="abort_on_error=1:symbolize=0:detect_leaks=0"
export UBSAN_OPTIONS="halt_on_error=1"
afl-fuzz -i part-b-afl/in_seeds -o part-c-asan-ubsan/out_c_asan_ubsan -m none -- fuzz_build/harness_part_c @@ /dev/null
```

## Run custom mutator (Part D)

```sh
./part-d-mutator/script.sh
```

## Build targets (Windows)

```powershell
pwsh -File ./build_target_windows.ps1
```
