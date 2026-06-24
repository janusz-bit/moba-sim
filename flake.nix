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
        { pkgs, self', ... }:
        let
          name = "moba-sim";
          windowsPkgs = pkgs.pkgsCross.mingwW64;

          buildCpp =
            stdenv:
            stdenv.mkDerivation {
              inherit name;
              src = ./.;
              nativeBuildInputs = [
                pkgs.cmake
                pkgs.ninja
              ];
              buildInputs = [
                pkgs.boost
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

          runCheck =
            name: exe:
            pkgs.runCommand "check-${name}" { } ''
              HOME=$PWD
              ${exe} > $out
              cat $out
              grep -q "Hello from moba-sim!" $out
            '';
        in
        {
          packages = {
            default = buildCpp pkgs.clangStdenv;
            windows = buildCpp pkgs.pkgsCross.mingwW64.stdenv;
          };

          checks = {
            linux = runCheck "linux" "${self'.packages.default}/bin/moba-sim";
            windows = runCheck "windows-exe" "${windowsPkgs.stdenv.hostPlatform.emulator windowsPkgs.buildPackages} ${self'.packages.windows}/bin/moba-sim.exe";
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
              boost
            ];
          };
        };
    };
}
