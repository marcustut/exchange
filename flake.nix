{
  description = "Nix environment for development.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";

    utils.url = "github:numtide/flake-utils";
  };

  outputs = { nixpkgs, ... }@inputs: inputs.utils.lib.eachSystem [
    "x86_64-linux"
    "aarch64-linux"
    "aarch64-darwin"
  ]
    (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      {
        devShells.default = pkgs.mkShell rec {
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
            stdenv.cc.libc # libc
            criterion # unit test framework
            cjson # JSON parsing

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
          '';
        };
      });
}
