{ clangOnly ? "no" }:
let
  sources = import ./nix/sources.nix;
  pkgs = import sources.nixpkgs { };
  mach-nix = import (
    pkgs.fetchFromGitHub {
    owner = "DavHau";
    repo = "mach-nix";
    rev = "refs/tags/3.3.0";
    sha256 = "sha256-RvbFjxnnY/+/zEkvQpl85MICmyp9p2EoncjuN80yrYA=";
  }
  ) {
    pkgs = pkgs;
    python = "python37";
  };
  customPython = mach-nix.mkPython rec {
    requirements = ''
      pytest
      toml
    '';
  };
  llvmPkgs = pkgs.buildPackages.llvmPackages_11;
  myStdenv = if clangOnly=="yes" then llvmPkgs.stdenv else pkgs.gcc10Stdenv;
  myBinutils = if clangOnly=="yes" then llvmPkgs.bintools else pkgs.binutils;
  mkShellNewEnv = pkgs.mkShell.override { stdenv = myStdenv; };
in mkShellNewEnv {
  nativeBuildInputs = [ pkgs.cmake ];
  buildInputs = with pkgs; [
    customPython
    bashInteractive
    which
    gfortran
    valgrind
    gdb
    fmt
    llvm_11
    lld_11
    myBinutils
    bison_3_5
    zlib
    libbfd
    re2c
    # git
    xonsh
    rapidjson
  ];
}
