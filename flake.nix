{
  description = "C++ project cross-compiled for Linux and Windows";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    { flake-parts, ... }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [ "x86_64-linux" ];

      perSystem =
        { pkgs, ... }:
        let
          name = "moba-sim";

          buildCpp =
            stdenv:
            stdenv.mkDerivation {
              inherit name;
              src = ./.;
              nativeBuildInputs = [
                pkgs.cmake
                pkgs.ninja
              ];
              cmakeFlags = [
                "-G"
                "Ninja"
              ];
              installPhase = ''
                runHook preInstall
                mkdir -p $out/bin
                cp -v ${name}${stdenv.hostPlatform.extensions.executable} \
                      $out/bin/${name}${stdenv.hostPlatform.extensions.executable}
                runHook postInstall
              '';
            };
        in
        {
          packages = {
            default = buildCpp pkgs.clangStdenv;
            windows = buildCpp pkgs.pkgsCross.mingwW64.stdenv;
          };

          devShells.default = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
            packages = with pkgs; [
              cmake
              ninja
              clang
              pkgs.pkgsCross.mingwW64.buildPackages.gcc
              wine64
              clang-tools
              lldb
            ];
          };

          apps.buildWindows = {
            type = "app";
            program =
              (pkgs.writeShellScriptBin "build-windows" ''
                set -euo pipefail
                echo "Building Windows .exe via cross-compilation..."
                nix build .#windows
                echo "Done. Output:"
                ls -la result/bin/
              '')
              + "/bin/build-windows";
          };
        };
    };
}