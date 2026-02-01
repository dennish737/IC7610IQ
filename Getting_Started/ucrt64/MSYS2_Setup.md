Instillation
============

1.  Clone the git repository.

2.  Go the git repository/Setup_instruction Directory

Install MSYS2
=============

-   MSYS2 is a collection of tool for C and C++ development on Windows using the
    GCC tool set. MSYS2 provides build envrionments for GCC, mingw-w64, CPython,
    CMake, Meson, OpenSSL, FFmpeg, Rust, Ruby, and others.

-   For easy installation a package management tool, Pacman, is provided

-   Four Environments to choose from:

    -   UCRT64 - Windows new Universal C Runtime (UCRT) Libraries

    -   MINGW64 - POSIX complient development old MSVCRT Libraries

    -   CLANG64

    -   MSYS \#\# Choosing your environment UCRT64 is a good default
        environment. Otherwise the choice depend on:

    -   Preferred compiled: GCC or Clang (some provide both).

    -   Preferred C++ standard library (GCC's libstdc++ or Clang's libc++, some
        provide both).

    -   Preferred C standard library (the old MSVCRT, or the new UCRT with the
        UTF-8 support).

    -   Whether you need Cygwin-style POSIX emulation.

    -   Whether you want to produce executables for ARM.

This document covers setup got URCT64

Download
--------

An Installer package can be found [here]
https://github.com/msys2/msys2-installer/releases/download/2025-08-30/msys2-x86_64-20250830.exe

After installation completes

1.  open the URTC64 environment: Windows key: type "msys2" and select URTC64

2.  change the directory git repository directory Setup Instructions  
    cd \<git repository\>/Setup_instructions

3.  Install the tools

>   `./ucrt64-tools.pash`

1.  test for compiler

-   `gcc --version`

1.  Install library packages

>   .`/ucrt64_libpkgs.bash`

1.  Install gnuradio:

>   ./GNU_packages.bash

Build Soapy IC76IQ interface
============================

1.  Cd to \<bit repository\> -- cd ..

2.  mkdir build

3.  cd build

4.  cmake -G "MSYS Makefiles" .. (or if you prefer Ninja cmake -G Ninja ..)

5.  Make the project -\> make (or if you prefer Ninja ninja)

6.  Move soapy package from build to target location  
    cp libIC7610SDRSupport.dll /c/msys64/ucrt64/lib/SoapySDR/modules0.8

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
