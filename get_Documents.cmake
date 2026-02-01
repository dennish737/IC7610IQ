
# Define the path to the directory you want to check and potentially create

set(DOCS_DIR "${CMAKE_SOURCE_DIR}/docs")

# Check if the directory exists
if(NOT EXISTS "${DOCS_DIR}")
    # If it doesn't exist, create it
    file(MAKE_DIRECTORY "${DOCS_DIR}")
    message(STATUS "Created directory: ${DOCS_DIR}")
else()
    message(STATUS "Directory already exists: ${DOCS_DIR}")
endif()

#set(DOC_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/docs")



set(AN_107_PDF_FILE_PATH "${DOCS_DIR}/AN_107_AdvancedDriverOptions_AN_000073.pdf")
set(AN_220_PDF_FILE_PATH "${DOCS_DIR}/AN_220_FTDI_Drivers_Installation_Guide_for_Linux-1.pdf")
set(AN_379_PDF_FILE_PATH "${DOCS_DIR}/AN_379-D3xx-Programmers-Guide-1.pdf")
set(AN_396_PDF_FILE_PATH "${DOCS_DIR}/AN_396_FTDI_Drivers_Installation_Guide_for_Windows_10_11.pdf")
set(AN_386_PDF_FILE_PATH "${DOCS_DIR}/AN_386_FTDI_600_Maximize_Preformance.pdf")
set(AN_387_PDF_FILE_PATH "${DOCS_DIR}/AN_387_FTDI_600_Data_Streamer_Application_User_Guide.pdf")
set(IC7610_CIV_PDF_FILE_PATH "${DOCS_DIR}/ICOM_IC-7610_CI-V_Reference_Guide.pdf")
set(IC7610_IQ_REF_PDF_FILE_PATH "${DOCS_DIR}/ICOM_IC-7610_IQ_Reference_Guide.pdf")

set(AN_107_DOWNLOAD_URL "https://www.ftdichip.com/Documents/AppNotes/AN_107_AdvancedDriverOptions_AN_000073.pdf")
set(AN_220_DOWNLOAD_URL "https://ftdichip.com/wp-content/uploads/2020/08/AN_220_FTDI_Drivers_Installation_Guide_for_Linux-1.pdf")
set(AN_379_DOWNLOAD_URL "https://ftdichip.com/wp-content/uploads/2020/07/AN_379-D3xx-Programmers-Guide-1.pdf")
set(AN_396_DOWNLOAD_URL "https://ftdichip.com/wp-content/uploads/2022/05/AN_396-FTDI-Drivers-Installation-Guide-for-Windows-10_11.pdf")
set(AN_386_DOWNLOAD_URL "https://ftdichip.com/wp-content/uploads/2020/07/AN_386-FTDI-FT600-Maximize-Performance.pdf")
set(AN_387_DOWNLOAD_URL "https://ftdichip.com/wp-content/uploads/2020/07/AN_387-FT600-Data-Streamer-Application-User-Guide.pdf")
set(IC7610_CIV_REF_URL "https://static.dxengineering.com/global/images/technicalarticles/ico-ic-7610_yj.pdf")
set(IC7610_I/Q_REF_URL "https://static.dxengineering.com/global/images/technicalarticles/ico-ic-7610_pu.pdf")

if(NOT EXISTS "${AN_107_PDF_FILE_PATH}")
    message(STATUS "PDF file '${AN_107_PDF_FILE_PATH}' not found. Downloading...")
	
	get_filename_component(PDF_DIR "${AN_107_PDF_FILE_PATH}" DIRECTORY)
	if(NOT EXISTS "${PDF_DIR}")
        file(MAKE_DIRECTORY "${PDF_DIR}")
    endif()
	
    # Download the PDF file
    file(DOWNLOAD "${AN_107_DOWNLOAD_URL}" "${AN_107_PDF_FILE_PATH}"
         STATUS download_status
         LOG download_log
         SHOW_PROGRESS)

    list(GET download_status 0 download_error_code)
    list(GET download_status 1 download_error_message)

    if(download_error_code EQUAL 0)
        message(STATUS "PDF file downloaded successfully to '${AN_107_PDF_FILE_PATH}'.")
    else()
        message(FATAL_ERROR "Failed to download PDF file: ${download_error_message}\n${download_log}")
    endif()	
else()
    message(STATUS "PDF file '${PDF_FILE_PATH}' already exists.")
endif()

if(NOT EXISTS "${AN_220_PDF_FILE_PATH}")
    message(STATUS "PDF file '${AN_220_PDF_FILE_PATH}' not found. Downloading...")
	
	get_filename_component(PDF_DIR "${AN_220_PDF_FILE_PATH}" DIRECTORY)
	if(NOT EXISTS "${PDF_DIR}")
        file(MAKE_DIRECTORY "${PDF_DIR}")
    endif()
	
    # Download the PDF file
    file(DOWNLOAD "${AN_220_DOWNLOAD_URL}" "${AN_220_PDF_FILE_PATH}"
         STATUS download_status
         LOG download_log
         SHOW_PROGRESS)

    list(GET download_status 0 download_error_code)
    list(GET download_status 1 download_error_message)

    if(download_error_code EQUAL 0)
        message(STATUS "PDF file downloaded successfully to '${AN_220_PDF_FILE_PATH}'.")
    else()
        message(FATAL_ERROR "Failed to download PDF file: ${download_error_message}\n${download_log}")
    endif()	
else()
    message(STATUS "PDF file '${AN_220_PDF_FILE_PATH}' already exists.")
endif()

if(NOT EXISTS "${AN_379_PDF_FILE_PATH}")
    message(STATUS "PDF file '${AN_379_PDF_FILE_PATH}' not found. Downloading...")
	
	get_filename_component(PDF_DIR "${AN_379_PDF_FILE_PATH}" DIRECTORY)
	if(NOT EXISTS "${PDF_DIR}")
        file(MAKE_DIRECTORY "${PDF_DIR}")
    endif()
	
    # Download the PDF file
    file(DOWNLOAD "${AN_379_DOWNLOAD_URL}" "${AN_379_PDF_FILE_PATH}"
         STATUS download_status
         LOG download_log
         SHOW_PROGRESS)

    list(GET download_status 0 download_error_code)
    list(GET download_status 1 download_error_message)

    if(download_error_code EQUAL 0)
        message(STATUS "PDF file downloaded successfully to '${AN_379_PDF_FILE_PATH}'.")
    else()
        message(FATAL_ERROR "Failed to download PDF file: ${download_error_message}\n${download_log}")
    endif()	
else()
    message(STATUS "PDF file '${AN_396_PDF_FILE_PATH}' already exists.")
endif()

if(NOT EXISTS "${AN_396_PDF_FILE_PATH}")
    message(STATUS "PDF file '${AN_379_PDF_FILE_PATH}' not found. Downloading...")
	
	get_filename_component(PDF_DIR "${AN_396_PDF_FILE_PATH}" DIRECTORY)
	if(NOT EXISTS "${PDF_DIR}")
        file(MAKE_DIRECTORY "${PDF_DIR}")
    endif()
	
    # Download the PDF file
    file(DOWNLOAD "${AN_396_DOWNLOAD_URL}" "${AN_396_PDF_FILE_PATH}"
         STATUS download_status
         LOG download_log
         SHOW_PROGRESS)

    list(GET download_status 0 download_error_code)
    list(GET download_status 1 download_error_message)

    if(download_error_code EQUAL 0)
        message(STATUS "PDF file downloaded successfully to '${AN_396_PDF_FILE_PATH}'.")
    else()
        message(FATAL_ERROR "Failed to download PDF file: ${download_error_message}\n${download_log}")
    endif()	
else()
    message(STATUS "PDF file '${AN_396_PDF_FILE_PATH}' already exists.")
endif()

if(NOT EXISTS "${AN_386_PDF_FILE_PATH}")
    message(STATUS "PDF file '${AN_386_PDF_FILE_PATH}' not found. Downloading...")
	
	get_filename_component(PDF_DIR "${AN_386_PDF_FILE_PATH}" DIRECTORY)
	if(NOT EXISTS "${PDF_DIR}")
        file(MAKE_DIRECTORY "${PDF_DIR}")
    endif()
	
    # Download the PDF file
    file(DOWNLOAD "${AN_386_DOWNLOAD_URL}" "${AN_386_PDF_FILE_PATH}"
         STATUS download_status
         LOG download_log
         SHOW_PROGRESS)

    list(GET download_status 0 download_error_code)
    list(GET download_status 1 download_error_message)

    if(download_error_code EQUAL 0)
        message(STATUS "PDF file downloaded successfully to '${AN_386_PDF_FILE_PATH}'.")
    else()
        message(FATAL_ERROR "Failed to download PDF file: ${download_error_message}\n${download_log}")
    endif()	
else()
    message(STATUS "PDF file '${AN_386_PDF_FILE_PATH}' already exists.")
endif()

if(NOT EXISTS "${AN_387_PDF_FILE_PATH}")
    message(STATUS "PDF file '${AN_387_PDF_FILE_PATH}' not found. Downloading...")
	
	get_filename_component(PDF_DIR "${AN_387_PDF_FILE_PATH}" DIRECTORY)
	if(NOT EXISTS "${PDF_DIR}")
        file(MAKE_DIRECTORY "${PDF_DIR}")
    endif()
	
    # Download the PDF file
    file(DOWNLOAD "${AN_387_DOWNLOAD_URL}" "${AN_387_PDF_FILE_PATH}"
         STATUS download_status
         LOG download_log
         SHOW_PROGRESS)

    list(GET download_status 0 download_error_code)
    list(GET download_status 1 download_error_message)

    if(download_error_code EQUAL 0)
        message(STATUS "PDF file downloaded successfully to '${AN_387_PDF_FILE_PATH}'.")
    else()
        message(FATAL_ERROR "Failed to download PDF file: ${download_error_message}\n${download_log}")
    endif()	
else()
    message(STATUS "PDF file '${AN_387_PDF_FILE_PATH}' already exists.")
endif()

if(NOT EXISTS "${IC7610_CIV_PDF_FILE_PATH}")
    message(STATUS "PDF file '${IC7610_CIV_PDF_FILE_PATH}' not found. Downloading...")

	get_filename_component(PDF_DIR "${IC7610_CIV_PDF_FILE_PATH}" DIRECTORY)
	if(NOT EXISTS "${PDF_DIR}")
        file(MAKE_DIRECTORY "${PDF_DIR}")
    endif()
	
	# Find the wget executable
	find_package(Wget QUIET)

	if(Wget_FOUND)
		# Define the URL and output file

		# Run wget with options (e.g., quiet mode, output document to a specific file)
		execute_process(
			COMMAND ${WGET_EXECUTABLE} "--user-agent=${USER_AGENT}" "-e robots=off" "-O" "${IC7610_CIV_PDF_FILE_PATH}" "${IC7610_CIV_REF_URL}"
			# Specify behavior on error
			RESULT_VARIABLE WGET_RESULT
			OUTPUT_VARIABLE WGET_OUTPUT
			ERROR_VARIABLE WGET_ERROR
		)

		# Check the result of the command
		if(NOT WGET_RESULT EQUAL 0)
			message(FATAL_ERROR "wget failed to download ${IC7610_CIV_REF_URL}: ${WGET_ERROR}")
		else()
			message(STATUS "Successfully downloaded '${IC7610_CIV_PDF_FILE_PATH}'.")
		endif()
	else()
		message(WARNING "wget not found, download skipped. Please install wget for full functionality.")
	endif()
else()
    message(STATUS "PDF file '${IC7610_CIV_PDF_FILE_PATH}' already exists.")
endif()

if(NOT EXISTS "${IC7610_IQ_REF_PDF_FILE_PATH}")
    message(STATUS "PDF file '${IC7610_IQ_REF_PDF_FILE_PATH}' not found. Downloading...")
	
	get_filename_component(PDF_DIR "${IC7610_IQ_REF_PDF_FILE_PATH}" DIRECTORY)
	if(NOT EXISTS "${PDF_DIR}")
        file(MAKE_DIRECTORY "${PDF_DIR}")
    endif()

	# Find the wget executable
	find_package(Wget QUIET)

	if(Wget_FOUND)
		# Define the URL and output file

		# Run wget with options (e.g., quiet mode, output document to a specific file)
		execute_process(
			COMMAND ${WGET_EXECUTABLE} "--user-agent=${USER_AGENT}" "-e robots=off" "-O" "${IC7610_IQ_REF_PDF_FILE_PATH}" "${IC7610_I/Q_REF_URL}"
			# Specify behavior on error
			RESULT_VARIABLE WGET_RESULT
			OUTPUT_VARIABLE WGET_OUTPUT
			ERROR_VARIABLE WGET_ERROR
		)

		# Check the result of the command
		if(NOT WGET_RESULT EQUAL 0)
			message(FATAL_ERROR "wget failed to download ${IC7610_I/Q_REF_URL}: ${WGET_ERROR}")
		else()
			message(STATUS "Successfully downloaded ${IC7610_IQ_REF_PDF_FILE_PATH}")
		endif()
	else()
		message(WARNING "wget not found, download skipped. Please install wget for full functionality.")
	endif()
else()
    message(STATUS "PDF file '${IC7610_IQ_REF_PDF_FILE_PATH}' already exists.")
endif()