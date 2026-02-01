Readme
======

The IC7610IQ project provides interfaces and tools for the capture, recording
and GNU Radio integration of the Icom IC7610 I/Q output.

Directory Structure
-------------------

.

├── CMakeLists.txt

├── FindFTD3XX.cmake

├── Getting_Started

│   ├── Getting_Started.md.docx

│   ├── Notes.txt

│   ├── minGW64

│   │   ├── Install-MSYS2-MinGW.ps1

│   │   ├── minGW_libpkgs.bash

│   │   └── minGW_tools.bash

│   └── ucrt64

│   ├── GNU_packages.bash

│   ├── Install-MSYS2-UCRT64.ps1

│   ├── MSYS2_Setup.md

│   ├── Notes.txt

│   ├── SupportedSoapyAPIMethods.md

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

├── dir_tree.txt

├── docs

│   ├── Application_Note_xxxx.docx

│   └── IC7610_picture.png

├── get_Documents.cmake

├── gnuradio

│   ├── CosineSource.py

│   ├── SoapyIC7610SDR.grc

│   ├── cf32DataFile.grc

│   ├── cs16DataFile.grc

│   ├── hamradio.grc

│   ├── s16DataFile.grc

│   ├── s16DataTCP.grc

│   └── s16DataZmq.grc

├── include

│   ├── IcomCIVPort.hpp

│   ├── IcomIQPort.hpp

│   ├── SoapyIC7610SDR.hpp

│   ├── common.h

│   ├── ftd3xx.h

│   └── version.h

├── libs

│   └── libftd3xx.lib

├── src

│   ├── CMakeLists.txt

│   ├── IC7610SDR_Registration.cpp

│   ├── IC7610SDR_Settings.cpp

│   ├── IC7610SDR_Streaming.cpp

│   ├── IcomCIVPort.cpp

│   └── IcomIQPort.cpp

└── test

├── ArgumentFlags.cpp

├── BooleanFlags.cpp

├── CIVPort_test.cpp

├── Commands.cpp

├── IQPortStream_test.cpp

├── IQPort_test.cpp

├── SimpleExample.cpp

├── SoapyAPI_test.cpp

├── SoapyCS16StreamTest.cpp

├── SoapyStreamTest.cpp

├── WriteToFile.cpp

├── cs16TCPClientRandomGen.cpp

├── cs16TCPRandomGenerator.cpp

└── cs16zmqPush.cpp

Getting Started
---------------

Documents
---------

Know Issues
-----------
