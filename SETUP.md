part-a-setup (fast commands, macOS/Homebrew)

1) Prereqs:
   brew install llvm lld pkg-config libpng
   # llvm supplies clang/llvm-config; lld helps AFL++ LTO (optional).

2) Get AFL++ (keep local; do not commit):
   git clone https://github.com/AFLplusplus/AFLplusplus.git part-b-afl/AFLplusplus
   cd part-b-afl/AFLplusplus
   LLVM_CONFIG=/opt/homebrew/opt/llvm/bin/llvm-config make source-only
   # If shm errors: sudo ./afl-system-config

3) Environment for a shell:
   export PATH="$(pwd)/part-b-afl/AFLplusplus:$PATH"
   export LLVM_CONFIG=/opt/homebrew/opt/llvm/bin/llvm-config
   export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:${PKG_CONFIG_PATH}"

4) Verify:
   afl-fuzz -h | head -n 1
   afl-cc -version
   pkg-config --modversion libpng

Notes:
- AFLplusplus stays untracked; add to .gitignore if needed.
- Harness compiles with afl-cc/afl-clang-fast and pkg-config libpng flags.
