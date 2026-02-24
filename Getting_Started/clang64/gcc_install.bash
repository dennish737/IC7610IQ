#!/bin/bash
pacman -Syu --noconfirm
# Install basic tool packages
pacman -S --noconfirm mingw-w64-clang-x86_64-clang		#GCC C and C++ compilier tools
#pacman -S --noconfirm --needed base-devel mingw-w64-clang-x86_64-toolchain
pacman -S --noconfirm mingw-w64-clang-x86_64-toolchain