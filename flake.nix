{
  description = "Nix environment for development.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";
    flake-parts.url = "github:hercules-ci/flake-parts";
    flake-root.url = "github:srid/flake-root";
  };

  outputs = inputs@{ nixpkgs, flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; }
      {
        imports = [
          inputs.flake-root.flakeModule
        ];
        systems = [
          "x86_64-linux"
          "aarch64-linux"
          "aarch64-darwin"
        ];
        perSystem = { pkgs, lib, config, ... }: {
          devShells.default = pkgs.mkShell rec {
            inputsFrom = [ config.flake-root.devShell ];

            name = "exchange";

            # dev tools
            nativeBuildInputs = with pkgs; [
              meson # build system
              ninja # build system
              cmake # build system
              pkg-config # packages finder
              valgrind # memory profiler
              gdb # GNU debugger

              cargo # rust build system
              rustPlatform.bindgenHook # rust bindgen
            ];

            # libraries
            buildInputs = with pkgs; [
              llvmPackages.libclang.lib # clang lib
              zlib # compression lib
              criterion # unit test framework
              cjson # JSON parsing

              # for uwebsockets
              libstdcxx5
              libuv

              # lib for dixous (desktop app)
              openssl
              glib
              cairo
              pango
              atk
              gdk-pixbuf
              libsoup
              gtk3
              libappindicator
              webkitgtk_4_1
            ];

            shellHook = ''
              export LIBCLANG_PATH="${pkgs.llvmPackages.libclang.lib}/lib";
              export LD_LIBRARY_PATH="$FLAKE_ROOT/orderbook/build:${pkgs.lib.makeLibraryPath buildInputs}:$LD_LIBRARY_PATH"
            '';
          };
        };
      };
}
