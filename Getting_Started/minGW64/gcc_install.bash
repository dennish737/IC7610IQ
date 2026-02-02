#!/bin/bash
pacman -Syu --noconfirm
# Install basic tool packages
pacman -S --noconfirm mingw-w64-x86_64-gcc		#GCC C and C++ compilier tools
pacman -S --noconfirm --needed base-devel mingw-w64-x86_64-toolchain