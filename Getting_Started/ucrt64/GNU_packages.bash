#!/bin/bash

# Array of packages to check and install

PACKAGES=(
"mingw-w64-ucrt-x86_64-gnuradio" 
"mingw-w64-ucrt-x86_64-soapysdr"
 "mingw-w64-ucrt-x86_64-rtl-sdr"
 "mingw-w64-ucrt-x86_64-soapyhackrf"
 "mingw-w64-ucrt-x86_64-soapyrtlsdr" 
 "mingw-w64-ucrt-x86_64-gr"
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
pacman -Syu --noconfirm
echo "All specified packages are installed."
