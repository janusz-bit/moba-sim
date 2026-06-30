# moba-sim

C++ project cross-compiled for Linux and Windows using [Nix Flakes](https://wiki.nixos.org/wiki/Flakes) with [flake-parts](https://flake.parts/).

## Requirements

- [Nix](https://nixos.org/download/) with flakes enabled

## Quick start

```sh
# Enter dev shell (clang, mingw, cmake, clangd, lldb)
nix develop

# Build for Linux
nix build .#default
./result/bin/moba-sim

# Build for Windows (.exe)
nix build .#windows
file result/bin/moba-sim.exe   # PE32+ executable
```

## Commands

| Command | Description |
|---|---|
| `nix develop` | Dev shell with both toolchains + clangd |
| `nix build .#default` | Build Linux ELF binary (clang) |
| `nix build .#windows` | Cross-compile Windows `.exe` (MinGW-w64) |
| `nix flake check` | Validate flake outputs + run Wine test of `.exe` |
| `nix flake update` | Update flake inputs |
| `nix develop -c pre-commit run --all-files` | Run all pre-commit hooks on every file |
| `./build.sh` | Regenerate `compile_commands.json` (Linux toolchain) |
| `./build.sh windows` | Regenerate `compile_commands.json` (mingw toolchain) |

## Project structure

```
.
├── flake.nix          # flake-parts flake: Linux + Windows cross-compile
├── flake.lock         # Pinned nixpkgs + flake-parts revisions
├── CMakeLists.txt     # CMake config (C++23, self-contained compile_commands.json)
├── build.sh           # Generate compile_commands.json for clangd
├── .clangd            # clangd config (diagnostics, inlay hints, completion, index)
├── .clang-format      # clang-format style (C++23, 2-space, LLVM braces)
├── .clang-tidy        # clang-tidy checks and naming conventions
├── .editorconfig      # Cross-editor indentation and line-ending settings
├── .zed/              # Zed editor config (settings, tasks, debug)
│   ├── settings.json  # clangd binary/args, format-on-save, C++ settings
│   ├── tasks.json     # Build, test, check, regenerate compile_commands
│   └── debug.json     # CodeLLDB debug configurations
├── src/main.cpp       # Demo app detecting platform
└── .gitignore
```

## Editor integration (clangd)

clangd reads `compile_commands.json` to resolve include paths and compiler flags.

```sh
nix develop
./build.sh           # Linux toolchain
# or
./build.sh windows   # mingw toolchain
```

On NixOS the `clang++` wrapper injects standard-library include paths via `NIX_CFLAGS_COMPILE` at compile time. These implicit paths normally don't end up in `compile_commands.json`, so any editor launching clangd outside `nix develop` fails to find `<iostream>` and similar.

`CMakeLists.txt` works around this by emitting the compiler's implicit include directories as explicit `-isystem` flags:

```cmake
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()
```

This makes `compile_commands.json` self-contained — clangd resolves headers correctly whether or not it's launched from inside the nix shell. No `--query-driver` or per-editor config is needed.

`.clangd` tunes diagnostics (clang-tidy checks), inlay hints, completion, and the background/standard-library index.

## Editor integration — Zed

[Zed](https://zed.dev) reads the project-level `.zed/` directory for settings, tasks, and debug configurations.

### Setup

```sh
nix develop
./build.sh           # generate compile_commands.json for clangd
```

Open the project in Zed — clangd automatically picks up `.clangd`, `.clang-format`, and `compile_commands.json`.

### What's configured

| File | Purpose |
|---|---|
| `.zed/settings.json` | clangd arguments, format-on-save, C++ language settings |
| `.zed/tasks.json` | One-click tasks: build, test, check, regenerate compile_commands |
| `.zed/debug.json` | CodeLLDB debug configurations (moba-sim, moba-sim-tests) |
| `.editorconfig` | Cross-editor indentation, charset, line endings |
| `.clangd` | clangd diagnostics, inlay hints, completion, background index |
| `.clang-format` | clang-format style (C++23, 2-space, LLVM braces, 100 cols) |
| `.clang-tidy` | clang-tidy checks and naming conventions |

### Tasks

Open the task runner (`cmd-T` / `ctrl-T`) to run:

- `nix build (Linux)` — build the native binary
- `nix build (Windows .exe)` — cross-compile for Windows
- `nix build tests` — build the test binary
- `nix flake check` — validate flake outputs
- `regenerate compile_commands.json` — rebuild the compile database
- `pre-commit run --all-files` — run all hooks

### Debugging

CodeLLDB configurations are in `.zed/debug.json`. Use `F5` or the debug panel to launch the binary with breakpoints. The build step runs `nix build` automatically before each debug session.

## How cross-compilation works

The flake defines two packages via a shared `buildCpp` helper:

- **`.#default`** — native Linux build using `pkgs.clangStdenv` (clang)
- **`.#windows`** — Windows cross-compile using `pkgs.pkgsCross.mingwW64.stdenv` (MinGW-w64)

Both use the same `CMakeLists.txt` and source. CMake's `WIN32` guard static-links the MinGW runtime to reduce DLL dependencies.

The `checks.windows` derivation runs the cross-compiled `.exe` through Wine (`hostPlatform.emulator`, like the [nix.dev cross-compilation tutorial](https://nix.dev/tutorials/cross-compilation)) and verifies the output contains the expected greeting. Run it with `nix flake check`.
