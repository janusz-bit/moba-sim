# Test Suite for moba-sim — Design Spec

## Motivation

moba-sim currently has no automated unit tests. The project defines core game types
(`Resistances`, `WithHP`, `Alive`, `DamageBase`, `KindDamage`) in `src/main.cpp` but
these cannot be tested in isolation because they are tangled with `main()`. We need a
test harness integrated into the existing Nix flake so `nix flake check` runs the suite
alongside the existing Linux/Windows smoke checks.

## Scope

**In scope:**
- Extract game types from `src/main.cpp` into a header `include/moba/units.hpp`.
- Add Boost.Test suite in `test/basic_test.cpp` (header-only mode).
- Wire CMake to build and run tests via CTest.
- Add a `tests` derivation to `flake.nix` registered as a flake check.

**Out of scope:**
- Mocking, fuzzing, benchmarks.
- Testing Windows build of the test suite (Linux-only is fine for checks).
- Refactoring the game logic itself — only extraction to a header.

## Architecture

### File layout

```
include/moba/units.hpp   # game types, header-only, no main()
src/main.cpp             # main() only, includes units.hpp
test/basic_test.cpp      # Boost.Test runner + cases
CMakeLists.txt           # + tests target, enable_testing(), add_test()
flake.nix                # + buildTests derivation, checks.tests
```

### Header extraction — `include/moba/units.hpp`

Move verbatim from `src/main.cpp`:
- `namespace moba { ... }` containing `Type`, `KindDamage`, `DamageBase`, `Resistances`, `WithHP`, `Alive`.
- Forward declarations preserved (`class Alive;` before `DamageBase`).
- Include guards: `#pragma once`.
- Required standard headers (`<cstdint>`, `<functional>`, `<variant>`) moved here.

`src/main.cpp` becomes:
```cpp
#include <iostream>
#include "moba/units.hpp"

int main() { std::cout << "Hello from moba-sim!\n"; }
```

### Boost.Test — `test/basic_test.cpp`

Header-only mode (no link dependency beyond `Boost::headers`):
```cpp
#define BOOST_TEST_MODULE moba_tests
#include <boost/test/included/unit_test.hpp>
#include "moba/units.hpp"
```

Initial test cases (covering default constructors and invariant behavior):

1. **`resistances_default_zero`** — fresh `Resistances` has armor=0 and magic_resistance=0.
2. **`withhp_starts_full`** — fresh `WithHP` has current==max health.
3. **`alive_has_resistances_and_hp`** — `Alive` (multiple inheritance) compiles and satisfies both base contracts.
4. **`damage_base_default_physical`** — default `DamageBase` has `kind == KindDamage::Physical` and `amount` holds `Type{0}` (valueless-by-default index 0).
5. **`kind_damage_underlying_int8`** — `std::is_same_v<std::underlying_type_t<KindDamage>, std::int8_t>`.

`WithHP` needs accessors for test #2 — add `maxHp()` and `currentHp()` const methods
(currently fields are private with no getters). This is a minimal addition to make the
type testable, not a behavioral change.

### CMake — `CMakeLists.txt`

Additions:
```cmake
enable_testing()

add_executable(moba-sim-tests test/basic_test.cpp)
target_link_libraries(moba-sim-tests PRIVATE Boost::headers)
target_include_directories(moba-sim-tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME moba-tests COMMAND moba-sim-tests)
```

Also add `${CMAKE_SOURCE_DIR}/include` to the `moba-sim` target's include dirs so
`main.cpp` can `#include "moba/units.hpp"`.

### Nix — `flake.nix`

Add `buildTests` derivation alongside `buildCpp`:
```nix
buildTests =
  stdenv:
  stdenv.mkDerivation {
    name = "moba-sim-tests";
    src = ./.;
    nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
    buildInputs = [ pkgs.boost ];
    cmakeFlags = [ "-G" "Ninja" ];
    doCheck = true;
    checkPhase = ''
      ctest --output-on-failure
    '';
    installPhase = ''
      runHook preInstall
      mkdir -p $out/bin
      cp -v moba-sim-tests $out/bin/
      runHook postInstall
    '';
  };
```

Register as a check:
```nix
checks.tests = buildTests pkgs.clangStdenv;
```

Keep existing `checks.linux` and `checks.windows` unchanged.

## Data flow

```
nix flake check
  └─ checks.linux     (existing — runs moba-sim, greps output)
  └─ checks.windows   (existing — runs moba-sim.exe via wine)
  └─ checks.tests     (new — builds & runs moba-sim-tests via ctest)
        └─ CMake builds test/basic_test.cpp
        └─ CTest runs the test binary, exits nonzero on failure
```

## Error handling

- Test failures cause `ctest` to exit nonzero → derivation build fails → `nix flake check` reports the error.
- Missing Boost.Test headers are caught at configure time (CMake `find_package(Boost)` already required).
- Compile errors in the header surface in both `moba-sim` and `moba-sim-tests` builds.

## Testing the tests

- Run `nix flake check` to verify all three checks pass.
- Manually break an assertion (e.g., change expected armor to 1) and confirm `nix flake check` fails.
- Run `nix build .#tests` and execute `$out/bin/moba-sim-tests` directly to see Boost.Test output.

## Risks

- **Header-only Boost.Test is slow to compile.** Acceptable for a small test suite; revisit if it grows past ~10 files by switching to linked mode (`Boost::test_support`).
- **Extracting to a header changes the translation unit boundary.** Existing `checks.linux`/`windows` smoke tests must still pass — they only check the binary output ("Hello from moba-sim!").
- **`WithHP` accessors are new public API.** Minimal risk; only added to enable testing, not changing existing behavior.
