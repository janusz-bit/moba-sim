# moba-sim

C++ project cross-compiled for Linux and Windows using [Nix Flakes](https://wiki.nixos.org/wiki/Flakes) with [flake-parts](https://flake.parts/).

## Requirements

- [Nix](https://nixos.org/download/) with flakes enabled
- Zed editor (optional, for IDE integration)

## Quick start

```sh
# Enter dev shell (gcc, mingw, cmake, clangd, lldb)
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
| `nix build .#default` | Build Linux ELF binary |
| `nix build .#windows` | Cross-compile Windows `.exe` (MinGW-w64) |
| `nix run .#buildWindows` | Build Windows + print output listing |
| `nix flake check` | Validate flake outputs |
| `nix flake update` | Update flake inputs |

## Project structure

```
.
├── flake.nix          # flake-parts flake: Linux + Windows cross-compile
├── flake.lock         # Pinned nixpkgs + flake-parts revisions
├── CMakeLists.txt     # CMake config (C++17, exports compile_commands.json)
├── build.sh           # Generate compile_commands.json for clangd
├── .zed/settings.json # Zed editor LSP config (clangd + query-driver)
├── src/main.cpp       # Demo app detecting platform
└── .gitignore
```

## Zed editor integration

clangd (C++ language server) reads `compile_commands.json` to resolve include paths from the Nix toolchain.

### Setup

```sh
nix develop
./build.sh           # generates compile_commands.json (Linux toolchain)
# or
./build.sh windows   # generates compile_commands.json (mingw toolchain)
```

Then open the project in Zed — clangd provides completion, diagnostics, and go-to-definition using the Nix store toolchain paths.

`.zed/settings.json` configures clangd with `--query-driver=/nix/store/**/g++` so it can invoke the Nix-wrapped compiler driver to discover system include paths.

## How cross-compilation works

The flake defines two packages via a shared `buildCpp` helper:

- **`.#default`** — native Linux build using `pkgs.stdenv` (GCC)
- **`.#windows`** — Windows cross-compile using `pkgs.pkgsCross.mingwW64.stdenv` (MinGW-w64)

Both use the same `CMakeLists.txt` and source. CMake's `WIN32` guard static-links the MinGW runtime to reduce DLL dependencies.