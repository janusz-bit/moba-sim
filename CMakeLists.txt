cmake_minimum_required(VERSION 3.16)
project(moba-sim CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# On NixOS, the clang/clangd wrappers inject standard library include paths via
# NIX_CFLAGS_COMPILE at compile time. These paths do not end up in the
# compile_commands.json that CMake generates, so editors launching clangd
# outside the nix shell cannot find <iostream> etc. Surface the compiler's
# implicit include directories as explicit -isystem flags so the compile
# database is self-contained. See: https://discourse.nixos.org/t/11268/10
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(moba-sim
  src/main.cpp
)

# Windows-specific: static link runtime to avoid DLL dependencies
if(WIN32)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".dll")
  target_link_options(moba-sim PRIVATE -static-libgcc -static-libstdc++)
endif()
