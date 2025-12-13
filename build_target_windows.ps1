$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path "$PSScriptRoot").Path
$WorkDir = Join-Path $RepoRoot 'fuzz_build'

New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

$Image = 'ubuntu:22.04'

$HostRepoRoot = $env:HOST_REPO_ROOT
if ([string]::IsNullOrWhiteSpace($HostRepoRoot)) {
  $HostRepoRoot = $RepoRoot
}

$Inner = @'
set -euxo pipefail

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
  ca-certificates git build-essential autoconf automake libtool pkg-config \
  clang-14 llvm-14 llvm-14-dev lld-14 zlib1g-dev

WORK=/repo/fuzz_build
mkdir -p "$WORK"

if [ ! -d "$WORK/AFLplusplus" ]; then
  git clone --depth 1 https://github.com/AFLplusplus/AFLplusplus.git "$WORK/AFLplusplus"
fi
make -C "$WORK/AFLplusplus" clean || true
make -C "$WORK/AFLplusplus" source-only LLVM_CONFIG=llvm-config-14
AFL_CC="$WORK/AFLplusplus/afl-cc"

if [ ! -d "$WORK/libpng" ]; then
  git clone --depth 1 https://github.com/pnggroup/libpng.git "$WORK/libpng"
fi
cd "$WORK/libpng"

echo "Building Part B..."
git clean -fdx
autoreconf -fi
CFLAGS="-O2" CC="$AFL_CC" ./configure --disable-shared --prefix="$WORK/install_b"
make -j"$(nproc)" install
"$AFL_CC" /repo/part-a-harness/png_harness.c -I"$WORK/install_b/include" "$WORK/install_b/lib/libpng.a" -lz -lm -o "$WORK/harness_part_b"

echo "Building Part C..."
git clean -fdx
autoreconf -fi
export AFL_USE_ASAN=1 AFL_USE_UBSAN=1
CFLAGS="-O2" CC="$AFL_CC" ./configure --disable-shared --prefix="$WORK/install_c"
make -j"$(nproc)" install
"$AFL_CC" /repo/part-a-harness/png_harness.c -I"$WORK/install_c/include" "$WORK/install_c/lib/libpng.a" -lz -lm -o "$WORK/harness_part_c"
unset AFL_USE_ASAN AFL_USE_UBSAN

echo "Building Part D..."
gcc -shared -Wall -O3 -fPIC /repo/part-d-mutator/custom_mutator.c -o "$WORK/custom_mutator.so" -lz

echo "Done! Targets are in /repo/fuzz_build"
'@

docker run --rm -t `
  -v "${HostRepoRoot}:/repo" `
  -w /repo `
  $Image `
  bash -lc $Inner

Write-Host "Artifacts written to: $WorkDir"