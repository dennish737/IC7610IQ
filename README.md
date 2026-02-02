Readme
======

The IC7610IQ project provides interfaces and tools for the capture, recording
and GNU Radio integration of the Icom IC7610 I/Q output.

Directory Structure
-------------------

.

├── CMakeLists.txt

├── FindFTD3XX.cmake : Downloads needed FTDI Library

├── Getting_Started

│   ├── Getting_Started.md.docx : Directions for setting up dev env.

│   ├── minGW64

│   │   ├── Tools for installing minGW development environment

│   └── ucrt64

│   ├── Tools for installing UCRT64 development environment

│   ├── ucrt64_libpkgs.bash

│   └── ucrt64_tools.bash

├── README.md

├── Tools

│   ├── IC7610TCPClient.cpp

│   ├── IC7610TCPServer.cpp

│   ├── IC7610zmqPush.cpp

│   ├── SigMFFileWriter.cpp

│   └── findDevice.cpp

├── bin

├── docs

│   ├── Application_Note_xxxx.docx

│   └── IC7610_picture.png

├── get_Documents.cmake : downloads support development documents

├── gnuradio

│   ├── Gnu Radio Models

├── include

│   ├──Project include files

├── libs

│   └── Project libraries

├── src

│   ├── Library Sources

└── test

├── Test Applications

└── Tools : Generated Tools

Getting Started
---------------

Instructions for setting up the development environment and building tools and
libraries are provided in the Getting_Started/Getting_Started.md document

Documents
---------

Application Note xxx1 - Covers tools and IC 7610 IQ Port Library

Application Note xxx2 - Covers the SoapySDR interface

Gnu Radio Models
----------------

Example GNURadio models are available in the gnuradio directory.

Know Issues
-----------

This is an initial release for more extensive test.

In Interfacing with GNU Radio dead lock conditions have occurred. The problem
appears to be device FIFO issues, requiring a reset of the device.

There are several known issues for GNURadio on a PC. The primary ones are the
Socket Support, and Threads. Because of the socket issue, the IC7610TCPServer
cannot be used with GnuRadio. Because of threads, the SoapySDR Interface does
not work with PC GNURadio. I am currently working on these issues to resolve the
problems.
