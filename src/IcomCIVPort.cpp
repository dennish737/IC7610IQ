/*************************************
 * civ_utils: This file contained utilities to send and receive command to the Icom 7610 CIV port. This is a serial port. 
 * There are two control port.
 * The stanrdard serial CI-V port 
**/
#include "IcomCIVPort.hpp"


IcomCIVPort::IcomCIVPort( ) 
{
	_baudrate = defaultBaudrate; //default rate
	_isInitialized = false;
	_isOpen = false;
}

std::string IcomCIVPort::findIcomCIVPort() {
    struct sp_port **port_list;
    enum sp_return result;
	std::string civ_name;
    // Enumerate all available serial ports
    result = sp_list_ports(&port_list);
    if (result != SP_OK) {
        std::cerr << "Failed to list ports." << std::endl;
        return nullptr;
    }

    bool device_found = false;
    struct sp_port *found_port = nullptr;
	std::string port_name = IcomCIVPort::findIcomCtrlPort();
    // Iterate through the list of ports


    if (!device_found) {
		// log error in soapylog
		throw std::runtime_error( "No Serial Port devices found\n");
        //std::cout << "Target device (VID: 0x" << std::hex << TARGET_VENDOR_ID 
        //          << ", PID: 0x" << std::hex << TARGET_PRODUCT_ID << ") not found." << std::endl;
    } 
	

    // Free the port list when done
    sp_free_port_list(port_list);
	
    return(civ_name );
}


void IcomCIVPort::init (std::string port_name, int baudrate)
{
	fprintf(stderr, "get_civ_port  called\n");
	if(port_name.empty())
	{
		fprintf(stderr, "port_name is null\n");
		throw std::runtime_error("Failed: No Port Name");
	}
	
	_baudrate = baudrate;
    enum sp_return result = SP_OK;
    // Get a port structure by name
    result = sp_get_port_by_name(port_name.c_str(), &_civ_port);
    if (result != SP_OK) {
		fprintf(stderr,"Port %s not found\n",port_name.c_str());
        throw std::runtime_error("Failed: to get CIV port");
    }
	
    // Open the port
    result = sp_open(_civ_port, SP_MODE_READ_WRITE);
    if (result != SP_OK) {
		fprintf(stderr,"Failed to open port %s.\n",port_name.c_str());
        sp_free_port(_civ_port);
        throw std::runtime_error("Failed: to open CIV port");
    }

    // Configure the port settings for Icom 7610 CI-V
    // Common settings: 9600 or 115200 baud, 8 data bits, no parity, 1 stop bit
    sp_set_baudrate(_civ_port, _baudrate); // Set baud rate (adjust as needed)
    sp_set_bits(_civ_port, 8);
    sp_set_parity(_civ_port, SP_PARITY_NONE);
    sp_set_stopbits(_civ_port, 1);
    sp_set_flowcontrol(_civ_port, SP_FLOWCONTROL_NONE);
	_isInitialized = true;
	_isOpen = true;
	fprintf(stderr, "civ_port initialized\n");
}

bool IcomCIVPort::isOpen()
{
	return (_isOpen);
}

bool IcomCIVPort::isInitialized()
{
	return (_isInitialized);
}

IcomCIVPort::~IcomCIVPort()
{
    sp_close(_civ_port);
    sp_free_port(_civ_port);
}


void IcomCIVPort::check_sp_result(enum sp_return result, const std::string& message) {
    if (result != SP_OK) {
        char *error_message = sp_last_error_message();
		fprintf(stderr, "%s: %s\n", message.c_str(), error_message);
        sp_free_error_message(error_message);
        //exit(EXIT_FAILURE);
    }
}

std::string IcomCIVPort::findIcomCtrlPort() 
{
    struct sp_port **port_list;
    enum sp_return result;
	std::string civ_name;
    // Enumerate all available serial ports
    result = sp_list_ports(&port_list);
    if (result != SP_OK) {
        std::cerr << "Failed to list ports." << std::endl;
        return nullptr;
    }

    bool device_found = false;
	struct sp_port *found_port;
    //struct sp_port *found_port = nullptr;
	
    // Iterate through the list of ports
    for (int i = 0; port_list[i] != nullptr; i++) {
        struct sp_port *port = port_list[i];
        
        // Get port information
        const char *port_name = sp_get_port_name(port);
        int vendor_id, product_id;
        
        // Check if the port is a USB device and retrieve VID/PID
        if (sp_get_port_transport(port) == SP_TRANSPORT_USB) {
            sp_get_port_usb_vid_pid(port, &vendor_id, &product_id);
			fprintf(stdout, "Found port: %s |  VID: 0x%x | PID: 0x%x", port_name, vendor_id, product_id);
			
            //std::cout << "Found port: " << port_name 
            //          << " VID: 0x" << std::hex << std::setw(4) << std::setfill('0') << vendor_id 
            //          << " PID: 0x" << std::hex << std::setw(4) << std::setfill('0') << product_id << std::endl;

            // Check if the VID and PID match the target device
            if (vendor_id == TARGET_VENDOR_ID && product_id == TARGET_PRODUCT_ID) {
                found_port = port;
				std::ostringstream oss;
				oss << port_name ;
				civ_name = oss.str();
				fprintf(stdout, "Target Device Found at port %s", port_name);
				//std::cout << "Target device found at port: " << civ_name << std::endl;
                device_found = true;
                // You can break here if you only need the first matching device
                 break; 
            }
        }
    }

    if (!device_found) {
		// log error
		fprintf(stdout, "No Serial Port devices found");
        //std::cout << "Target device (VID: 0x" << std::hex << TARGET_VENDOR_ID 
        //          << ", PID: 0x" << std::hex << TARGET_PRODUCT_ID << ") not found." << std::endl;
    } //else {
        // You can now use 'found_port' to open and configure the serial port
        // Example: sp_open(found_port, SP_MODE_READ_WRITE);
        // sp_set_baudrate(found_port, 9600);
        // ... communication code ...
        // sp_close(found_port);
    //}

    // Free the port list when done
    sp_free_port_list(port_list);
	fprintf(stdout, "Target CIV Device Found at port %s", civ_name.c_str());
    return(civ_name );
}


int IcomCIVPort::sendCIVCmd( std::vector<uint8_t> cmd)
{

	uint8_t buf[32] = {CIV_PREAMBLE, CIV_PREAMBLE, RADIO_ADDR, PC_ADDR};
	int buf_size = 4;
	// add the command
    for (size_t i = 0; i < cmd.size(); i++) {
        buf[buf_size++] = cmd[i];
    }
	// end the civ message
    buf[buf_size++] = CIV_EOM;
	
    for (size_t i = 0; i < (size_t)buf_size; i++) {
		fprintf(stderr, "Sending Command %02x\n", buf[i]);
    }
	
    // Write the command data
    int write_status = sp_blocking_write(_civ_port, buf, buf_size, 1000); // 1000ms timeout
    if (write_status < 0) {
		fprintf(stderr, "Error writing command data: %s\n", sp_last_error_message());
        return write_status;
    }

    // Return the total number of bytes written
    return buf_size;
}

int IcomCIVPort::readCIVReply( std::vector<uint8_t>& buffer)
{
    int bytes_read = 0;
    uint8_t byte;

    // Read byte by byte until EOM (0xFD) is found or buffer is full/timeout occurs
    //while (bytes_read < buffer_len - 1) { // Leave space for EOM if needed
	while (bytes_read < CIV_READ_BUFFER_SIZE - 1) { // Leave space for EOM if needed
        int read_status = sp_blocking_read(_civ_port, &byte, 1, 500); // 500ms timeout per byte

        if (read_status < 0) {
			fprintf(stderr, "Error reading reply: %s\n", sp_last_error_message());
            return read_status;
        }

        if (read_status == 0) {
            // Timeout occurred before EOM was found
			fprintf(stderr, "Timeout waiting for EOM or reply data\n");
            break;
        }

        //buffer[bytes_read++] = byte;
		bytes_read++;
		buffer.push_back(byte);
		
        if (byte == CIV_EOM) {
			for (size_t i = 0; i < (size_t)bytes_read; i++) {
				//fprintf(stderr, "civ_received %02x\n", buffer[i]);
				fprintf(stderr, "civ_received: %02x\n", buffer[i]);
			}
            break; // EOM received, end of reply
        }
    }

    return bytes_read;
}
	
int IcomCIVPort::icomCIVCommand( std::vector<uint8_t> cmd, std::vector<uint8_t> &reply)
{
	int sent_bytes;
	int received_bytes;
	
	fprintf(stderr, "icom_civ_cmd called\n");
	sent_bytes = sendCIVCmd(cmd);
	fprintf(stderr, "send_civ_cmd completed: %d\n", sent_bytes);
	
	if (sent_bytes > 0) {
		
		fprintf(stderr, "calling read_civ_reply\n");
		received_bytes = readCIVReply(reply);
		fprintf(stderr, "read_civ_reply completed\n");
        if (received_bytes > 0) {
            fprintf(stderr,"Received %d bytes. \n", received_bytes);
			fprintf(stderr,"Vector Size %lld bytes. \n ", reply.size());
            for (uint8_t byte : reply) {
				std::cout << "Received: " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
				std::cout << std::endl;
            }
			return received_bytes;
        }
		fprintf(stderr,"Receive Timeout or Error %d bytes. \n ", received_bytes);
		return received_bytes;
	}
	return sent_bytes;	
}