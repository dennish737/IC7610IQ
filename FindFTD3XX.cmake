###if(NOT FTD3XX_FOUND)
	set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
	set(INC_DIR "${INCLUDE_DIR}")
	set(LIBS_DIR "${CMAKE_SOURCE_DIR}/libs")
	set(ARCHIVE_PREFIX "FTD3XXLibrary_1.3.0.10")
	set(ARCHIVE_URL "https://ftdichip.com/wp-content/uploads/2024/06/${ARCHIVE_PREFIX}.zip")
	message("Include Dir ${INC_DIR}")
	message("Liberary Dir ${LIBS_DIR}")
	message("Archive URL ${ARCHIVE_URL}")
	
	# make sure we have the include directory and libs directory are define
	if(NOT DEFINED INC_DIR)
		message(FATAL_ERROR "Required Include Directory, INC_DIR, is not defined. Please define it before configuring the project.")
	endif()
	if(NOT DEFINED LIBS_DIR)
		message(FATAL_ERROR "Required Library Directory, LIBS_DIR, is not defined. Please define it before configuring the project.")
	endif()	
	if(NOT DEFINED ARCHIVE_URL)
		message(FATAL_ERROR "Required Include Directory, ARCHIVE_PREFIX, is not defined. Please define it before configuring the project.")
	endif()
	
	message("Current Binary Dir ${CMAKE_BINARY_DIR}")
	# Define variables for the download
	set(INC_FILE_1 "ftd3xx.h")
	set(LIB_FILE_1 "libftd3xx.a")
	set(LIB_DLL_FILE_1 "libftd3xx.dll")

	set(DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/downloads")

	cmake_path(GET ARCHIVE_URL FILENAME ARCHIVE_FILE_NAME)
	set(ARCHIVE_FILE "${DOWNLOAD_DIR}/${ARCHIVE_FILE_NAME}")
	string(LENGTH "${ARCHIVE_FILE_NAME}" ARCHIVE_NAME_LEN)
	math(EXPR ARCHIVE_PREFIX_LEN "${ARCHIVE_NAME_LEN} - 4")
	string(SUBSTRING "${ARCHIVE_FILE_NAME}" 0 "${ARCHIVE_PREFIX_LEN}" ARCHIVE_PREFIX)
	
	set(EXTRACT_DIR "${CMAKE_BINARY_DIR}/extracted_files")
	set(PACKAGE_DIR "${EXTRACT_DIR}/${ARCHIVE_PREFIX}")
	set(ARCHIVE_INCLUDE_FILE_1 "${PACKAGE_DIR}/FTD3XX.h")
	set(ARCHIVE_LIB_FILE_1 "${PACKAGE_DIR}/x64/Static_Lib/FTD3XX.lib")
	set(ARCHIVE_LIB_DLL_1 "${PACKAGE_DIR}/x64/Static_Lib/FTD3XX.dll")
	message("Archive Prefix: ${ARCHIVE_PREFIX}")
	message("Archive File Name: ${ARCHIVE_FILE_NAME}")
	message("Include Dir ${INC_DIR}")
	message("Liberary Dir ${LIBS_DIR}")
	message("Archive URL ${ARCHIVE_URL}")
	message("Archive FILE ${ARCHIVE_FILE}")
	message("Download Dir ${DOWNLOAD_DIR}")
	message("Extract Dir ${EXTRACT_DIR}")
		
#------------------------------------------------------------------------------------------------------------	
	cmake_path(GET ARCHIVE_INCLUDE_FILE_1 FILENAME INC_NAME1)
	if(EXISTS "${ARCHIVE_INCLUDE_FILE_1}")
		file(COPY "${ARCHIVE_INCLUDE_FILE_1}" DESTINATION "${INC_DIR}")
		file(RENAME "${INC_DIR}/${INC_NAME1}" "${INC_DIR}/${INC_FILE_1}")
	else()
		file(MAKE_DIRECTORY "${DOWNLOAD_DIR}")
		# Download the file
		file(DOWNLOAD "${ARCHIVE_URL}" "${ARCHIVE_FILE}"
				STATUS download_status
				LOG download_log)
			
		# Check if the download was successful
		if(NOT download_status EQUAL 0)
				set(FTD3XX_FOUND FALSE)
				message(FATAL_ERROR "Failed to download ${FILE_URL}: ${download_log}")
				return()
		endif()	
		# Create the extraction directory if it doesn't exist
		file(MAKE_DIRECTORY "${EXTRACT_DIR}")
		# Extract the archive
		file(ARCHIVE_EXTRACT INPUT "${ARCHIVE_FILE}"
								 DESTINATION "${EXTRACT_DIR}")		

		file(COPY "${ARCHIVE_INCLUDE_FILE_1}" DESTINATION "${INC_DIR}")
		file(RENAME "${INC_DIR}/${INC_NAME1}" "${INC_DIR}/${INC_FILE_1}")
	endif()
	# The FTD3XX package is a zip file with a name like FTD3XXLibrary_1.3.0.10 where everything past the '_'
	# is the version number. When downloaded and un packed the package will be in the ${EXTRACT_DIR}/FTD3XXLibrary_1.3.0.10
	# directory. This directory should have two files and two directories:
	#		Files		: FD3XX.h and ReleaseNotes.rtf
	#		Directories	: Win32 and x64
	# Under each of the two directories there two subdirecties DLL, and Static_Lib. 
	# If you are in an Win32 environment  use Win32, if x64 use Win64. we assume you system is
	# 64 bit and static libraries. We want the include file in include dir, and the library in the libs dir.
	# The files will also be rename to "standard names"

	cmake_path(GET ARCHIVE_LIB_FILE_1 FILENAME LIB_NAME_1)
	if(EXISTS "${ARCHIVE_LIB_FILE_1}" AND EXISTS "${ARCHIVE_LIB_DLL_1}") 
		file(COPY "${ARCHIVE_LIB_FILE_1}" DESTINATION "${LIBS_DIR}")
		file(COPY "${ARCHIVE_LIB_DLL_1}" DESTINATION "${LIBS_DIR}")
		file(RENAME "${LIBS_DIR}/FTD3XX.lib" "${LIBS_DIR}/libftd3xx.lib")
		file(RENAME "${LIBS_DIR}/FTD3XX.dll" "${LIBS_DIR}/${libftd3xx.dll}")
	else()
		file(MAKE_DIRECTORY "${DOWNLOAD_DIR}")
		# Download the file
		file(DOWNLOAD "${ARCHIVE_URL}" "${ARCHIVE_FILE}"
				STATUS download_status
				LOG download_log)
			
		# Check if the download was successful
		if(NOT download_status EQUAL 0)
				set(FTD3XX_FOUND FALSE)
				message(FATAL_ERROR "Failed to download ${FILE_URL}: ${download_log}")
				return()
		endif()			
		# Create the extraction directory if it doesn't exist
		file(MAKE_DIRECTORY "${EXTRACT_DIR}")
		# Extract the archive
		file(ARCHIVE_EXTRACT INPUT "${ARCHIVE_FILE}"
								 DESTINATION "${EXTRACT_DIR}")
		message("Copy ${PACKAGE_DIR}/x64/Static_Lib/FTD3XX.lib to ${LIBS_DIR}")
		file(COPY "${PACKAGE_DIR}/x64/DLL/FTD3XX.lib" DESTINATION "${LIBS_DIR}")
		file(COPY "${PACKAGE_DIR}/x64/DLL/FTD3XX.dll" DESTINATION "${LIBS_DIR}")
		file(RENAME "${LIBS_DIR}/FTD3XX.lib" "${LIBS_DIR}/libftd3xx.lib")
		# Note: we need libftd3xx.dll for linking, and ftd3xx.dll for running
		file(COPY_FILE "${LIBS_DIR}/FTD3XX.dll"  "${LIBS_DIR}/libftd3xx.dll")
		file(RENAME "${LIBS_DIR}/FTD3XX.dll"  "${LIBS_DIR}/ftd3xx.dll")

	endif()

	SET(FTD3XX_FOUND TRUE)
	SET(FTD3XX_INCLUDE_DIRS "${INC_DIR}")
	SET(FTD3XX_LIBRARIES "${LIBS_DIR}/libftd3xx.lib" "${LIBS_DIR}/libftd3xx.dll")

	mark_as_advanced(FTD3XX_INCLUDE_DIR FTD3XX_LIBRARIES)
