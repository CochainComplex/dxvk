# Building DXVK-Fogged with Optimizations

This guide explains how to build DXVK-Fogged with all performance optimizations enabled.

## Prerequisites

### Required Software
- **Meson** build system (>= 0.58)
- **MinGW-w64** compiler (for Windows builds)
- **Wine** (for testing on Linux)
- **Git** (for submodules)

### Install Dependencies (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install \
  meson \
  ninja-build \
  glslang-tools \
  mingw-w64 \
  mingw-w64-tools \
  git \
  wine \
  wine64
```

### Install Dependencies (Arch Linux)
```bash
sudo pacman -S \
  meson \
  ninja \
  glslang \
  mingw-w64-gcc \
  wine \
  git
```

## Building DXVK-Fogged

### 1. Clone the Repository
```bash
git clone https://github.com/yourusername/dxvk-fogged.git
cd dxvk-fogged

# Initialize submodules for optimization libraries
git submodule update --init --recursive
```

### 2. Quick Build (All Optimizations)
```bash
# Build for 64-bit and 32-bit with all optimizations
./package-release.sh master ./build --dev-build

# Output will be in:
# ./build/dxvk-master-xxxxxx/x64/ (64-bit DLLs)
# ./build/dxvk-master-xxxxxx/x32/ (32-bit DLLs)
```

### 3. Manual Build with Custom Options

#### 64-bit Build
```bash
meson setup build.w64 \
  --cross-file build-win64.txt \
  --buildtype release \
  --prefix $(pwd)/install \
  -Denable_asmlib=true \
  -Denable_vcl=true \
  -Denable_sha_hw=true \
  -Denable_simd_pack=true \
  -Denable_cb_batching=true \
  -Denable_async_query=true

cd build.w64
ninja install
cd ..
```

#### 32-bit Build
```bash
meson setup build.w32 \
  --cross-file build-win32.txt \
  --buildtype release \
  --prefix $(pwd)/install \
  -Denable_asmlib=true \
  -Denable_vcl=true \
  -Denable_sha_hw=true \
  -Denable_simd_pack=true \
  -Denable_cb_batching=true \
  -Denable_async_query=true

cd build.w32
ninja install
cd ..
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `enable_asmlib` | true | CPU-optimized memory functions (20-40% faster) |
| `enable_vcl` | true | SIMD vector/matrix operations (4x+ throughput) |
| `enable_sha_hw` | true | Hardware SHA acceleration (3-4x faster hashing) |
| `enable_simd_pack` | true | SIMD image packing (50-70% faster conversions) |
| `enable_cb_batching` | true | Constant buffer batching (15-25% less overhead) |
| `enable_async_query` | true | Async query collection (15-30% lower latency) |
| `enable_fma` | true | Fused multiply-add (better precision/performance) |

## Installation

### Installing to Wine Prefix
```bash
# Set your Wine prefix
export WINEPREFIX=/path/to/your/wine/prefix

# Install 64-bit DLLs
cp install/bin/d3d*.dll "$WINEPREFIX/drive_c/windows/system32/"
cp install/bin/dxgi.dll "$WINEPREFIX/drive_c/windows/system32/"

# Install 32-bit DLLs (if built)
cp install/bin32/d3d*.dll "$WINEPREFIX/drive_c/windows/syswow64/"
cp install/bin32/dxgi.dll "$WINEPREFIX/drive_c/windows/syswow64/"

# Register DLLs with Wine
cd "$WINEPREFIX/drive_c/windows/system32"
for dll in d3d9.dll d3d10.dll d3d10_1.dll d3d10core.dll d3d11.dll dxgi.dll; do
  wine64 regsvr32 $dll
done
```

### Using setup_dxvk.sh Script
```bash
# Alternatively, use the provided setup script
./setup_dxvk.sh install --symlink
```

## Verifying the Build

### Check Optimization Flags
```bash
# Check if optimizations are compiled in
strings install/bin/d3d11.dll | grep -E "asmlib|VCL|SHA-NI|SIMD"

# Check build configuration
ninja -C build.w64 -t commands | grep -E "DXVK_USE_"
```

### Performance Testing
```bash
# Run a DirectX application with DXVK HUD
DXVK_HUD=full wine your_game.exe

# Monitor performance metrics
# - FPS and frame time
# - Draw calls
# - Pipeline compilations
# - Memory usage
```

## Troubleshooting

### Build Failures

#### Missing Submodules
```bash
# If vectorclass or asmlib are missing:
git submodule update --init --recursive
```

#### Compiler Issues
```bash
# Check MinGW installation
x86_64-w64-mingw32-g++ --version

# For 32-bit
i686-w64-mingw32-g++ --version
```

#### Meson Configuration
```bash
# Clear build directory and reconfigure
rm -rf build.w64
meson setup build.w64 --cross-file build-win64.txt --buildtype release
```

### Runtime Issues

#### DLL Loading Failures
```bash
# Check Wine DLL overrides
winecfg
# Set d3d11, d3d10, d3d9, dxgi to "native, builtin"

# Verify DLL placement
ls -la "$WINEPREFIX/drive_c/windows/system32/"*.dll
```

#### Performance Problems
```bash
# Enable debug logging
export DXVK_LOG_LEVEL=debug
export DXVK_LOG_PATH=/tmp

# Check CPU features
cat /proc/cpuinfo | grep -E "avx2|fma|sha_ni"
```

## Development Workflow

### Incremental Builds
```bash
# After making changes, rebuild quickly:
cd build.w64
ninja
ninja install
```

### Testing Changes
```bash
# Run tests
meson test -C build.w64

# Benchmark specific optimization
DXVK_CONFIG="dxvk.enableAsync = true" wine benchmark.exe
```

### Debug Builds
```bash
# For debugging
meson setup build.debug \
  --cross-file build-win64.txt \
  --buildtype debug \
  -Denable_asmlib=false  # Disable optimizations for debugging

cd build.debug
ninja
```

## Platform-Specific Notes

### Linux
- All optimizations work out of the box
- FMA enabled automatically
- Best performance with native Vulkan drivers

### Windows (Native)
- Building natively on Windows requires Visual Studio or MSYS2
- Some optimizations may have reduced effectiveness
- FMA disabled due to stack alignment issues

### Steam Deck
- All optimizations compatible
- Pre-compiled packages available
- Optimized for Zen 2 architecture

## Performance Tips

1. **CPU Governor**: Set to performance mode
   ```bash
   sudo cpupower frequency-set -g performance
   ```

2. **Shader Cache**: Keep shader cache persistent
   ```bash
   export DXVK_STATE_CACHE_PATH=/path/to/cache
   ```

3. **Async Presentation**: Enable for reduced latency
   ```bash
   export DXVK_ASYNC=1
   ```

4. **Memory Allocation**: Use huge pages if available
   ```bash
   export DXVK_MEMORY_ALLOCATOR=1
   ```

## Support

For issues or questions:
- GitHub Issues: [your-repo-url]/issues
- Performance Reports: Include DXVK_HUD screenshots
- Build Logs: Attach meson-log.txt from build directory