#!/bin/bash

# Array of packages to check and install

PACKAGES=(
"mingw-w64-x86_64-zeromq" 			# mqqt package
"mingw-w64-x86_64-cppzmq" 
"mingw-w64-x86_64-libusb"			# usb library
"mingw-w64-x86_64-libserialport"	# serial port library
"mingw-w64-x86_64-minizip"			# zip and unzip tools
"mingw-w64-x86_64-zlib"
"mingw-w64-x86_64-zziplib"
"mingw-w64-x86_64-angleproject"	# gnuradio dependencies
"mingw-w64-x86_64-boost"			# install boost and boost-libs
"mingw-w64-x86_64-codec2"
"mingw-w64-x86_64-fftw"
"mingw-w64-x86_64-libsndfile"
"mingw-w64-x86_64-sigutils"
"mingw-w64-x86_64-fmt"
"mingw-w64-x86_64-gmp"
"mingw-w64-x86_64-gsl"
"mingw-w64-x86_64-gtk3"
"mingw-w64-x86_64-nlohmann-json"
)

echo "Checking for and installing required packages in UCRT64 environment..."

for pkg in "${PACKAGES[@]}"; do
    echo "Checking for package: $pkg"
    # Check if the package is installed
    if pacman -Q "$pkg" &>/dev/null; then
        echo "$pkg is already installed."
    else
        echo "$pkg is not installed. Installing now..."
        # Install the package
        if pacman -S --noconfirm "$pkg"; then
            echo "$pkg installed successfully."
        else
            echo "Error: Failed to install $pkg."
            exit 1 # Exit if a package fails to install
        fi
    fi
done

# update all package
pacman -Syu --noconfirm
echo "All specified packages are installed."
