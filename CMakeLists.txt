

cmake_minimum_required(VERSION 3.10)
project(SoapyIC7610SDR CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
# make sure the include and libs directories are available
# note 'clean' dies not remove these directories
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(LIBS_DIR "${CMAKE_SOURCE_DIR}/libs")
set(DOCUMENTS_DIR "${CMAKE_SOURCE_DIR}/docs")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")
set(TEST_BIN "${CMAKE_SOURCE_DIR}/test")
set(BIN_DIR "${CMAKE_SOURCE_DIR}/bin")

set(INC_FILE "ftd3xx.h")
set(LIB_FILE "libftd3xx.a")
set(LIB_SHARED_FILE "libftd3xx.dll")
set(ARCHIVE_PREFIX "FTD3XXLibrary_1.3.0.10")

set(NET_LIBS ws2_32 zmq)

set(ARCHIVE_URL "https://ftdichip.com/wp-content/uploads/2024/06/${ARCHIVE_PREFIX}.zip")

# select build type to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

#file(MAKE_DIRECTORY "${INCLUDE_DIR}")
file(MAKE_DIRECTORY "${LIBS_DIR}")
file(MAKE_DIRECTORY "${DOCUMENTS_DIR}")
#file(MAKE_DIRECTORY "${TEST_BIN}")
file(MAKE_DIRECTORY "${BIN_DIR}")
message("Documemt DIR : ${DOCUMENTS_DIR}")
message("Work DIR : ${CMAKE_CURRENT_SOURCE_DIR}")
# Get product documentation
message("Getting Documentation")
set(CMAKE_INCLUDE_PATH "${INCLUDE_DIR}")
set(CMAKE_LIBRARY_PATH "${LIBS_DIR}")


execute_process(
  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/get_Documents.cmake"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  RESULT_VARIABLE script_result
  OUTPUT_VARIABLE script_output
)

# check script_result, script_output, or script_error if needed
if(script_result EQUAL 0)
  message(STATUS "get_Documents.cmake executed successfully.")
else()
  message(FATAL_ERROR "get_Documents.cmake failed with error: ${script_error}")
endif()

message("Checking for SoapySDR Package")
# Check for SoapySDR package
find_package(SoapySDR NO_MODULE REQUIRED)
if (NOT SoapySDR_FOUND)
    message(FATAL_ERROR "Soapy SDR development files not found...")
else()
	message(STATUS "SoapySDR version ${SoapySDR_VERSION} found...")
endif ()
include_directories(${SoapySDR_INCLUDE_DIRS})


message("Getting FTD3XX Include files and library files")
message("Current Binary Dir ${CMAKE_BINARY_DIR}")
find_package(FTD3XX MODULE REQUIRED)
#execute_process(
#  COMMAND "${CMAKE_COMMAND}"
#			"-DINC_DIR=${INCLUDE_DIR}" 
#			"-DLIBS_DIR=${LIBS_DIR}"
#			"-DARCHIVE_URL=${ARCHIVE_URL}"
#			"-P" "${CMAKE_SOURCE_DIR}/FindFTD3XX.cmake"
#  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#  RESULT_VARIABLE script_result
#  OUTPUT_VARIABLE script_output
#  ERROR_VARIABLE script_error
#)

# check script_result, script_output, or script_error if needed
#if(script_result EQUAL 0)
if ("${FTD3XX_FOUND}")
	message(STATUS "FindFTD3XX.cmake executed successfully.")
	message(STATUS "FTD3XX_INCLUDE_DIRS - ${FTD3XX_INCLUDE_DIRS}")
	message(STATUS "FTD3XX_LIBRARIES - ${FTD3XX_LIBRARIES}")
else()
	message("Value of FTD3XX_FOUND: ${FTD3XX_FOUND}")
	message(FATAL_ERROR "FindFTD3XX.cmake failed with error: ${script_error}")
endif()


message("FTD3XX_LIBRARIES: ${FTD3XX_LIBRARIES}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${FTD3XX_INCLUDE_DIRS}")
link_directories("${FTD3XX_LIBRARIES}")

add_library(${LIB_SHARED_FILE} STATIC IMPORTED)
set_target_properties(${LIB_SHARED_FILE} PROPERTIES
			IMPORTED_LOCATION "${LIBS_DIR}/${LIB_SHARED_FILE}"
			IMPORTED_IMPLIB "${LIBS_DIR}/${LIB_SHARED_FILE}"
			INTERFACE_INCLUDE_DIRECTIES "${FTD3XX_INCLUDE_DIRS}"
)

add_library(icomCIVPort STATIC src/IcomCIVPort.cpp )
set_target_properties(icomCIVPort PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/libs")

add_library(icomIQPort STATIC src/IcomIQPort.cpp )
set_target_properties(icomIQPort PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/libs")
		
#add_subdirectory(test)

set(LIBRARY_SRCS
    src/IC7610SDR_Registration.cpp
    src/IC7610SDR_Settings.cpp
    src/IC7610SDR_Streaming.cpp
	src/IcomCIVPort.cpp
	src/IcomIQPort.cpp
)
			
SOAPY_SDR_MODULE_UTIL(
    TARGET IC7610SDRSupport
    SOURCES ${LIBRARY_SRCS}	
    LIBRARIES ${SoapySDR_LIBRARIES} ${FTD3XX_LIBRARIES} serialport usb-1.0
)

set_target_properties(IC7610SDRSupport PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIBS_DIR}
)
		
########### Test Applications - requires install_test before running
add_executable(IQPortTest test/IQPort_test.cpp src/IcomIQPort.cpp)
target_include_directories(IQPortTest PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(IQPortTest PUBLIC ${LIB_SHARED_FILE} )
set_target_properties(IQPortTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TEST_BIN}")

add_executable(IQPortStreamTest test/IQPortStream_test.cpp src/IcomIQPort.cpp)
target_include_directories(IQPortStreamTest PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(IQPortStreamTest PUBLIC ${LIB_SHARED_FILE} )
set_target_properties(IQPortStreamTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TEST_BIN}")

add_executable(CIVPortTest test/CIVPort_test.cpp src/IcomCIVPort.cpp)
target_include_directories(CIVPortTest PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(CIVPortTest PUBLIC ${LIB_SHARED_FILE} serialport usb-1.0 )
set_target_properties(CIVPortTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TEST_BIN}")

add_executable(WriteFileTest test/WriteToFile.cpp src/IcomIQPort.cpp)
target_include_directories(WriteFileTest PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(WriteFileTest PUBLIC ${LIB_SHARED_FILE} serialport usb-1.0 )
set_target_properties(WriteFileTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TEST_BIN}")

add_executable(SoapyAPITest test/SoapyAPI_test.cpp )
target_include_directories(SoapyAPITest PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(SoapyAPITest PUBLIC ${LIB_SHARED_FILE} serialport usb-1.0 SoapySDR)
set_target_properties(SoapyAPITest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TEST_BIN}")

add_executable(SoapyStreamTest test/SoapyStreamTest.cpp )
target_include_directories(SoapyStreamTest PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(SoapyStreamTest PUBLIC ${LIB_SHARED_FILE} serialport usb-1.0 SoapySDR)
set_target_properties(SoapyStreamTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TEST_BIN}")

add_executable(SoapyCS16StreamTest test/SoapyCS16StreamTest.cpp )
target_include_directories(SoapyCS16StreamTest PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(SoapyCS16StreamTest PUBLIC ${LIB_SHARED_FILE} serialport usb-1.0 SoapySDR)
set_target_properties(SoapyCS16StreamTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TEST_BIN}")

add_executable(SigMFWriter Tools/SigMFFileWriter.cpp src/IcomIQPort.cpp)
target_include_directories(SigMFWriter PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(SigMFWriter PUBLIC ${LIB_SHARED_FILE} serialport usb-1.0 )
set_target_properties(SigMFWriter PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}")

add_executable(IC7610zmqPush Tools/IC7610zmqPush.cpp src/IcomIQPort.cpp)
target_include_directories(IC7610zmqPush PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(IC7610zmqPush PUBLIC ${LIB_SHARED_FILE} serialport usb-1.0 ${NET_LIBS})
set_target_properties(IC7610zmqPush PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}")

add_executable(IC7610TCPServer Tools/IC7610TCPServer.cpp src/IcomIQPort.cpp)
target_include_directories(IC7610TCPServer PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(IC7610TCPServer PUBLIC ${LIB_SHARED_FILE} serialport usb-1.0 ${NET_LIBS})
set_target_properties(IC7610TCPServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}")

add_executable(IC7610TCPClient Tools/IC7610TCPClient.cpp src/IcomIQPort.cpp)
target_include_directories(IC7610TCPClient PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(IC7610TCPClient PUBLIC ${LIB_SHARED_FILE} serialport usb-1.0 ${NET_LIBS})
set_target_properties(IC7610TCPClient PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}")

add_executable(findDevice Tools/findDevice.cpp)
target_include_directories(findDevice PRIVATE ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(findDevice PUBLIC ${LIB_SHARED_FILE} serialport usb-1.0 )
set_target_properties(findDevice PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}")

add_custom_target(install_test
		#COMMAND ${CMAKE_COMMAND} -E copy "${LIBS_DIR}/${LIB_SHARED_FILE}" ${TEST_BIN}
		COMMAND ${CMAKE_COMMAND} -E copy "${LIBS_DIR}/ftd3xx.dll" ${TEST_BIN}
		#COMMAND ${CMAKE_COMMAND} -E copy "${LIBS_DIR}/libIC7610SDRSupport.dll" ${TEST_BIN}
)

# Link your module with the SoapySDR library
set(DEST_DIR "/c/msys64/ucrt64/lib/SoapySDR/modules0.8")

add_custom_target(install_assets
		COMMAND ${CMAKE_COMMAND} -E copy "${LIBS_DIR}/${LIB_SHARED_FILE}" ${DEST_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy "${LIBS_DIR}/ftd3xx.dll" ${DEST_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy "${LIBS_DIR}/libIC7610SDRSupport.dll" ${DEST_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy "${LIBS_DIR}/${LIB_SHARED_FILE}" ${BIN_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy "${LIBS_DIR}/ftd3xx.dll" ${BIN_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy "${LIBS_DIR}/libIC7610SDRSupport.dll" ${BIN_DIR}
)