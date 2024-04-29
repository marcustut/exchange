{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    # nativeBuildInputs is usually what you want -- tools you need to run
    nativeBuildInputs = with pkgs.buildPackages; [
        clang_17        # compiler
        clang-tools_17  # clang tools
        gnumake         # build system
        meson           # build system
        ninja           # build system
        bear            # to generate compile_commands.json
        valgrind        # memory profiler
        gdb             # GNU debugger
        # hiredis         # redis driver
        # cjson           # JSON parsing
        # libyaml         # YAML parsing
        # libcyaml        # Strongly-typed YAML parsing
    ];
    inputsFrom = [ pkgs.libcyaml ];
}
