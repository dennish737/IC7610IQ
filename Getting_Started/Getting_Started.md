|       |
|-------|
|       |
|       |
| \| \| |
|       |

This document covers the acquisition, build and deployment of tools and
interfaces for the IC7610 to GNU Radio, and provide SigMF recording.

Goals
=====

1.  Provide a tool for the capture of IQ data from the IC7610 hi speed USB port.

2.  Provide an interface between the IC7610 and GNURadio using ZMQ blocks.

3.  Develop a SoapySDR Model for the IC7610 (optional)

4.  Develop for the Windows Environment.

Why?
====

-   GNU Radio is an open-source software development toolkit for providing
    signal processing blocks for building Software-Defined Radios (SDR) and
    digital communication systems. The IC7610, provides a receive only, 16 bit A
    to D, with a sampling rate of 1.920 MHz (860 KHz bandwidth). Being able to
    capture and process high quality data in real time will provide
    opportunities for education, training and development of new radio products
    and tools.

-   SoapySDR is an open-source C++ API and runtime library that provides a
    standardized hardware-agnostic interface for SDR Devices.

-   SigMF (Signal Metadata Format) is an open standard for describing digital
    signal recording, especially RF data, by separating raw samples from
    human-readable JSON metadata, enabling interoperability, data sharing and
    analysis. Often data recording is made, with no meta data (e.g. radio
    settings, sampling rate, frequency, …), which limits the usability and
    openness of the data.

The software is available on github at \<add location\>. You can download the
software as a Zip file or using git clone.

If you download the Zip file, unzip it into a directory, you can remember, and
have no space in the path. Similarly, if you use git clone, clone the software
into a directory with no space in the path.

Other Useful Documents

In addition to the software, you should consider downloading the ICOM IC-7610
CI-V Reference Guide and the ICOM IC-7610 I/Q Output Reference Guide. These
items are available at \<ICOM Site\> and require accepting terms of use.

What is in the Package

1.  IcomIQPort Library

2.  IC7610SDR library

3.  SigMF file writer

4.  IC7610zmpPush

5.  IC7610 TCP Writer

6.  Documents

Installing GNU Radio and GNU Radio Development Environment
==========================================================

Introduction

GNU Radio is an open-source software development toolkit for implementing
software define (SDR) radios. Multiple software downloads are available for the
windows environment (Radio Conda, Gnu Radio for Windows …). If you want to
develop/add custom drivers and/or modules to Gnu Radio, you need a working GNU
Radio install, and a compatible C++ 11 (or greater) and Python 3.10 (or greater)
development environment.

For Windows, there are two environments, which have sufficient documentation for
successfully installing a integrated development. For this project we have
chosen the MSYS2 development environment.

MSYS2

MSYS2 is a collection of tools for C and C++ development on Windows using the
GCC tool set. MSYS2 provides build environment for GCC, mingw-w64, CPython,
CMake, Meson, OpenSSL, FFmpeg, Rust, Ruby, and others. In addition, an easy
installation package management tool, Pacman, is provided.

MSYS2 provides three (3) environments to choose from:

-   UCRT64 - Windows new Universal C Runtime (UCRT) Libraries

-   MINGW64 - POSIX compliant development old MSVCRT Libraries

-   CLANG64

No matter which environment is chosen, we must always remember that Windows is
not Linux, and most of the Gnuradio work is done on Linux. Any of the
environment will work, with known GNURadio issues involving network and threads.
You will find two (2) directories providing tools to install the UCRT and MinGW
environments.

For this app note, I have chosen the UCRT environment, and provide detailed
Set-up instructions. Tools are provided for UCRT64 directory.

Step 1: Installing the base MSYS2 UCRT64 environment
----------------------------------------------------

1.  Download the and unzip the \<github file\>

2.  Change Directory (cd \<repo\>/Getting_Started) to the repository
    ‘Getting_Started directory

3.  Open power shell, with admin privileges

4.  Go to the ./ucrt64 directory.

5.  Run the ‘Install-MYS2-UCRT64.ps1’ script. This will install the MSYS2 and
    the UCRT64 gcc tool chain

Alternate Method for Installing the base MSYS2 UCRT64 environment
-----------------------------------------------------------------

1.  Download and Install MSYS2 software package:  
    The installer package can be found on [github
    here](https://github.com/msys2/msys2-installer/releases/download/2025-08-30/msys2-x86_64-20250830.exe)
    . Once Downloaded, go to the download directory and run the installer (click
    on msys2-x86_64\<release date\>.exe to install. At the time of this writing
    the file name was msys2-x86_64-20251212.exe

2.  After installation, open the MSYS2 URTC64 terminal: Windows key: type
    "msys2" and select URTC64.

3.  Update the packages with the command:  
    pacman -Syu --noconfirm  
    Note: you may have to close and reopen the terminal after this update

4.  Install the GCC tool with the command:  
    pacman -S --noconfirm --needed base-devel mingw-w64-ucrt-x86_64-toolchain  
    Press Enter to accept default packages and Y to proceed.

5.  Verify Install. In the terminal window enter the command:  
    gcc --version

6.  GCC should respond with the current version number (e.g. gcc.exe (Rev8,
    Built by MSYS2 project) 15.2.0)

Step 2: Install Additional tools

This step will install additional tools for development, Python and GNU Radio

1.  Open a msys2 ucrt64 terminal.

2.  Go to the \<repo\>/Getting_Started/ucrt directory.

3.  Enter the command:  
    ./ucrt_tools.bash

4.  Answer Y to all prompts and wait for the script to finish

5.  Check that python is installed by running the following command  
    python --version  
    Python should respond with its version (e.g. Python 3.12.12)

6.  Check that git is installed  
    git --version  
    git will respond with its version number

Step 3: Add Additional Require Packages

This step will load any additional packages such as boost, codec2,
libserialport, etc.

1.  Open a msys2 ucrt64 terminal.

2.  Go to the \<repo\>/Getting_Started/ucrt64 directory.

3.  Enter the command:  
    ./ucrt_libplgs.bash

4.  Answer Y to all prompts and wait for the script to finish

5.  Check that gnuradio-companioh is installed by running the following command  
    gunradio-companion  
    gnuradio-companion should start running. Note: be patient, it takes a while,
    it will open a new window, so look for it.  
    Close gnuradio-compainion.

Step 4: Get Updates

This step will update and synchronize all the packages installed. Note you may
need to close and open the Msys2 UCRT64 terminal, to complete the update. If the
terminal is closed, reopen it and rerun the command.

1.  Open a msys2 ucrt64 terminal

2.  Enter the following command:  
    packman -Syu -- pacman -Syu --noconfirm

Building Software

1.  Open a Msys2 UCRT64 terminal

2.  Change Directory to the \<repo\> directory

3.  Create the build directory (mkdir build)

4.  Change Directory to the build directory (cd build)

5.  Run cmake (cmake -G “MSYS Makefiles” .. )

6.  Cmake does the following:

    1.  Downloads needed libraries and include files from FTDI for the D3xx
        device

    2.  Downloads FTDI Development Documents

    3.  Builds the makefile

7.  Make the tools, libraries and test apps (make)

8.  Install assets (make install_assets)

9.  Install needed test libraries (make install_test)
