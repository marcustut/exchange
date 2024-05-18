{ lib, stdenv, meson, ninja, cmake, pkg-config, valgrind, gdb }:

let
in stdenv.mkDerivation {
  pname = "orderbook";
  version = "0.0.1";
  nativeBuildInputs = [ meson ninja cmake pkg-config valgrind gdb ];
  buildInputs = [ criterion cjson ];
}
