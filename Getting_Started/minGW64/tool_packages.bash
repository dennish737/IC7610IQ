#!/bin/bash
pacman -Syu --noconfirm
# Install basic tool packages
# pacman -S --noconfirm mingw-w64-x86_64-gcc		#GCC C and C++ compilier tools
# pacman -S --noconfirm --needed base-devel mingw-w64-x86_64-toolchain

# Array of packages to check and install

PACKAGES=(
	"mingw-w64-x86_64-pkg-config"		# package management tools
	"mingw-w64-x86_64-cmake"			# cmake tools
	"mingw-w64-x86_64-cmake-gui"		# chame gui tools
	"mingw-w64-x86_64-make" 			# make utility
	"mingw-w64-x86_64-ninja"			# nija utility
	"mingw-w64-x86_64-python"			# python
	"mingw-w64-x86_64-python-pip"
	"mingw-w64-x86_64-python-setuptools"
	"mingw-w64-x86_64-python-numpy"
	"mingw-w64-x86_64-python-scipy"
	"mingw-w64-x86_64-python-pandas"
	"mingw-w64-x86_64-python-matplotlib"
	"mingw-w64-x86_64-python-opencv"
	"mingw-w64-x86_64-python-pyqt5"		# or pyqt6
	"mingw-w64-x86_64-python-requests"
	"mingw-w64-x86_64-python-lxml"
	"mingw-w64-x86_64-python-mako"
	"mingw-w64-x86_64-python-gobject"
	"git"								# git version management
	"unzip"
	"tree"

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
