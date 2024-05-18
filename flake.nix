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

          packages = with pkgs; [
            # dev tools
            meson # build system
            ninja # build system
            cmake # build system
            pkg-config # packages finder
            valgrind # memory profiler
            gdb # GNU debugger

            # libraries
            criterion # Unit test framework
            cjson # JSON parsing
          ];
        };

        packages.default = pkgs.callPackage ./orderbook/default.nix { };
      });
}
