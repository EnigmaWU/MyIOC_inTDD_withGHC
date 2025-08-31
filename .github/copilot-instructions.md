# MyIOC (Inter-Object Communication) Library
MyIOC is a C/C++ library for inter-object communication using Test-Driven Development (TDD) methodology. It provides Event (EVT), Command (CMD), and Data (DAT) communication patterns for embedded systems and applications.

Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.

## Working Effectively

### Dependencies and Environment Setup
Install required build dependencies:
```bash
sudo apt update && sudo apt install -y build-essential cmake libgtest-dev googletest gcc g++ ninja-build
```

### Building the Project
- **Core Library Build**: 
  ```bash
  cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -B build
  make -C build MyIOC
  ```
  Takes ~10 seconds. NEVER CANCEL - Set timeout to 30+ seconds.

- **Full Build with Tests** (some tests may fail due to format warnings):
  ```bash
  cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -B build
  cmake --build build --parallel
  ```
  Takes 45-60 seconds. NEVER CANCEL - Set timeout to 90+ seconds.

- **Fast Build with Ninja**:
  ```bash
  cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja -B build
  ninja -C build
  ```
  Takes 18-25 seconds. NEVER CANCEL - Set timeout to 60+ seconds.

### Testing
- **Build and Run Core Working Tests**:
  ```bash
  make -C build UT_CommandTypical UT_ServiceTypical UT_ConlesEventTypical
  ctest --test-dir build --tests-regex "UT_CommandTypical|UT_ServiceTypical|UT_ConlesEventTypical" --output-on-failure
  ```
  Build takes ~3 seconds, tests take ~4 seconds. NEVER CANCEL - Set timeout to 60+ seconds.

- **Run All Tests** (some may fail):
  ```bash
  ctest --test-dir build --output-on-failure
  ```
  Takes 10-15 seconds. NEVER CANCEL - Set timeout to 60+ seconds.

### Build Configurations
The project supports multiple build types:
- `RelWithDebInfo` - Recommended for development
- `DiagASAN` - Address Sanitizer for memory debugging
- `DiagTSAN` - Thread Sanitizer for concurrency debugging  
- `DiagUBSAN` - Undefined Behavior Sanitizer
- `DiagLSAN` - Leak Sanitizer

Example with sanitizer:
```bash
cmake -DCMAKE_BUILD_TYPE=DiagASAN -B build
cmake --build build --parallel
```

## Validation

### Manual Validation Scenarios
Always run these validation steps after making changes:
1. **Build Core Library**: Verify `libMyIOC.a` is created successfully
2. **Build and Run Core Tests**: Execute working test suite to ensure basic functionality
3. **Test API Usage**: Verify core IOC functionality works through test scenarios

### Code Quality Checks
The project uses clang-format and clang-tidy for code style:
```bash
# Format check (if available)
clang-format --dry-run --Werror Source/*.c Include/IOC/*.h

# Lint check (if available) 
clang-tidy Source/*.c -- -I./Include
```

### CI Validation
Always ensure changes pass CI workflows:
- `.github/workflows/ubuntu-cmake-gtest.yml` - Ubuntu builds with multiple sanitizers
- `.github/workflows/macos-cmake-gtest.yml` - macOS builds

## Common Tasks

### Project Structure
```
MyIOC_inTDD_withGHC/
├── Include/IOC/          # Public API headers
├── Source/               # Library implementation (C files)
├── Test/                 # Unit tests (UT_*.cxx files)
├── .github/             # CI workflows and instructions
├── CMakeLists.txt       # Main build configuration
└── README*.md           # Documentation files
```

### Key API Headers
- `Include/IOC/IOC.h` - Main API entry point
- `Include/IOC/IOC_CmdAPI.h` - Command communication API
- `Include/IOC/IOC_EvtAPI.h` - Event communication API  
- `Include/IOC/IOC_DatAPI.h` - Data communication API
- `Include/IOC/IOC_SrvAPI.h` - Service management API

### Unit Testing Guidelines
Follow TDD methodology as described in `.github/instructions/UnitTestingGuide.instructions.md`:
- Always write tests first before implementing features
- Use Test/UT_FreelyDrafts.cxx as template for new tests
- Follow US/AC/TC structure (User Story/Acceptance Criteria/Test Cases)
- Each UT_*.cxx file becomes a standalone test executable

### Known Issues and Workarounds
- **Format Warnings**: Some tests fail due to `printf` format string warnings. These are non-critical but cause build failures.
- **Missing Headers**: Some tests need `#include <cmath>` for `sqrt` function
- **Build Artifacts**: The `build/` directory is excluded from git via `.gitignore`

### Working with the Codebase
- **Library**: Core IOC functionality in C (Source/ directory)
- **Tests**: Comprehensive unit tests in C++ using Google Test framework
- **Documentation**: Extensive README files explaining architecture, use cases, and design
- **Standards**: C11 for library, C++17 for tests

### Typical Development Workflow
1. Write unit test first (TDD approach)
2. Run test to see it fail (RED)
3. Implement minimal code to make test pass (GREEN)  
4. Refactor code while keeping tests passing (REFACTOR)
5. Build and run all tests to ensure no regressions
6. Update documentation if needed

### Important Notes
- This is a learning project exploring Large Language Model based Software Engineering (LLMSE)
- The codebase follows strict TDD methodology
- GitHub Copilot is used as development assistant
- Focus on embedded systems and small-scale applications (KB-MB range)
- Communication patterns: Events (publish/subscribe), Commands (request/response), Data (streaming)