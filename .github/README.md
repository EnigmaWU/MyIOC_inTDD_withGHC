# CI/CD Configuration

This project includes GitHub Actions workflows for continuous integration and testing on multiple platforms.

## Requirements

- **C Standard**: C11
- **C++ Standard**: C++17 (required by modern Google Test)
- **CMake**: 3.16 or higher

## Workflows

### MacOS-CMake-GTest
- **File**: `.github/workflows/macos-cmake-gtest.yml`
- **Platform**: macOS-latest
- **Compiler**: LLVM Clang/Clang++
- **Build Tool**: CMake
- **Test Framework**: Google Test (GTest)
- **Build Types**:
  - RelWithDebInfo (Release with Debug Info)
  - DiagASAN (Address Sanitizer)
  - DiagTSAN (Thread Sanitizer)

### Ubuntu-CMake-GTest
- **File**: `.github/workflows/ubuntu-cmake-gtest.yml`
- **Platform**: ubuntu-latest
- **Compiler**: GCC/G++
- **Build Tool**: CMake with Ninja generator
- **Test Framework**: Google Test (GTest)
- **Build Types**:
  - RelWithDebInfo (Release with Debug Info)
  - DiagASAN (Address Sanitizer)
  - DiagTSAN (Thread Sanitizer)
  - DiagUBSAN (Undefined Behavior Sanitizer)

## Features

- **Multi-platform support**: Both macOS and Ubuntu
- **Multiple build configurations**: Different sanitizers for debugging
- **Parallel builds**: Faster compilation using all available cores
- **Test result artifacts**: Automatic upload of test results for analysis
- **Error reporting**: `--output-on-failure` flag shows detailed test failures

## Sanitizers

The project uses various sanitizers to catch different types of issues:

1. **AddressSanitizer (ASAN)**: Detects memory errors like buffer overflows, use-after-free, etc.
2. **ThreadSanitizer (TSAN)**: Detects data races and threading issues
3. **UndefinedBehaviorSanitizer (UBSAN)**: Detects undefined behavior in C/C++ code

## Triggers

Both workflows are triggered on:
- Push to any branch
- Pull requests

## Dependencies

### macOS:
- CMake
- Google Test
- LLVM (for Clang/Clang++)

### Ubuntu:
- build-essential
- CMake
- Google Test
- GCC/G++
- Ninja build system

## Build Directory Structure

Each build type creates its own directory:
- `Dir4RelWithDebInfo/`
- `Dir4DiagASAN/`
- `Dir4DiagTSAN/`
- `Dir4DiagUBSAN/` (Ubuntu only)

This prevents conflicts between different build configurations.
