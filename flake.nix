{
  description = "C++ project cross-compiled for Linux and Windows";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    git-hooks-nix = {
      inputs.nixpkgs.follows = "nixpkgs";
      url = "github:cachix/git-hooks.nix";
    };
  };

  outputs =
    { flake-parts, ... }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {

      imports = [
        inputs.git-hooks-nix.flakeModule
      ];

      systems = [ "x86_64-linux" ];

      perSystem =
        { config
        , pkgs
        , lib
        , self'
        , ...
        }:
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
          pre-commit.settings.hooks = {
            nixpkgs-fmt.enable = true;
            clang-format = {
              enable = true;
              types_or = lib.mkForce [ "c++" ];
            };
            clang-tidy = {
              enable = true;
              types_or = lib.mkForce [ "c++" ];
              # Point clang-tidy at the CMake-generated compile database and let
              # the project-level .clang-tidy file drive the check selection.
              entry = lib.mkForce
                "${pkgs.clang-tools}/bin/clang-tidy -p build";
              pass_filenames = true;
            };
            end-of-file-fixer.enable = true;
            trim-trailing-whitespace.enable = true;
          };

          packages = {
            default = buildCpp pkgs.clangStdenv;
            windows = buildCpp pkgs.pkgsCross.mingwW64.stdenv;
          };

          checks = {
            linux = runCheck "linux" "${self'.packages.default}/bin/moba-sim";
            windows = runCheck "windows-exe" "${windowsPkgs.stdenv.hostPlatform.emulator windowsPkgs.buildPackages} ${self'.packages.windows}/bin/moba-sim.exe";
          };

          devShells.default = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
            shellHook = ''
              ${config.pre-commit.shellHook}
            '';
            packages =
              (with pkgs; [
                cmake
                ninja
                clang
                pkgs.pkgsCross.mingwW64.buildPackages.gcc
                wine64
                clang-tools
                lldb
                boost
              ])
              ++ config.pre-commit.settings.enabledPackages;
          };
        };
    };
}
