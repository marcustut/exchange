{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    # nativeBuildInputs is usually what you want -- tools you need to run
    nativeBuildInputs = with pkgs.buildPackages; [
        # clang_17        # compiler
        # clang-tools_17  # clang tools
        # gnumake         # build system
        # bear            # to generate compile_commands.json
        meson           # build system
        ninja           # build system
        valgrind        # memory profiler
        gdb             # GNU debugger
        criterion       # Unit test framework
        # hiredis         # redis driver
        cjson           # JSON parsing
        # libyaml         # YAML parsing
        # libcyaml        # Strongly-typed YAML parsing
    ];
    inputsFrom = [ pkgs.criterion ];
}
