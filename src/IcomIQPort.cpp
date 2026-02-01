  


/*
#include "ftd3xx.h"

#include <memory>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <string>
#include <format>
#include <chrono>
#include <iterator>
#include <algorithm>
#include <thread>

#include <unistd.h> // For usleep
*/

#include "IcomIQPort.hpp"
#include <complex>


IcomIQPort::IcomIQPort()
{
	_isOpen = false;
	_isInitialized = false;
	//_cmdtimeout = TIMEOUT;
	_cmdTimeout = CMD_TIMEOUT;
	_dataTimeout = DATA_TIMEOUT;
	_iqDataEnabled = false;
	//_data_ready = false;
	_running = false;
	_buffer_size = IQ_BLOCK_LENGTH;
	_num_buffers = 	NUM_BLOCKS;
	
	_IPPlus = false;
	_DIGI_SEL = false;
	//iqBuffer_ (16384);
}

void IcomIQPort::init(std::string deviceSerialNum)
{
    //(void)args;
	// The FT_DEVICE_LIST_INFO_NODE is a stucture consisting of information about the 
	// device (Flags, Type, ID, serial number, description and handle
	// change number of devices (currently 16) to a constant
	_deviceSerialNum = deviceSerialNum;
    FT_DEVICE_LIST_INFO_NODE nodes[16];
    DWORD count;
    int res;
	
	// Create a list of devices
	// 1) get the number of devices that are available: returned in count
    if ((res = FT_CreateDeviceInfoList(&count)) != FT_OK) {
        //SoapySDR_logf(SOAPY_SDR_ERROR, "FT_CreateDeviceInfoList failed: %d\n", res);
		//fprintf(stderr, "FT_CreateDeviceInfoList failed: %d\n", res);
        throw std::runtime_error("Failed to no FTDI device(s)");
    }
	
	if ((res = FT_GetDeviceInfoList(nodes, &count)) != FT_OK) {
        //SoapySDR_logf(SOAPY_SDR_ERROR, "FT_GetDeviceInfoList failed: %d\n", res);
		//fprintf(stderr, "FT_GetDeviceInfoList failed: %d\n", res);
        throw std::runtime_error("Failed to get list FTDI device(s)");
    }
	// Open the high speed usb data port
    FT_Create((PVOID) (deviceSerialNum.c_str()), FT_OPEN_BY_SERIAL_NUMBER, &handle);
    if (!handle) {
        //SoapySDR_logf(SOAPY_SDR_ERROR, "FT_Create failed: %s\n", deviceSerialNum.c_str());
		//fprintf(stderr, "FT_Create failed: %s\n", deviceSerialNum.c_str());
        throw std::runtime_error("Failed to open FTDI device");
	}	
	_isInitialized = true;
	_isOpen = true;

}

std::string IcomIQPort::getDeviceSerialNum()
{
	FT_DEVICE_LIST_INFO_NODE nodes[16];
	DWORD count;
	int res;
	
	if ((res = FT_CreateDeviceInfoList(&count)) != FT_OK) {
		fprintf(stderr, "FT_CreateDeviceInfoList failed: %d\n", res);
		exit(1);
	}
		
	if ((res = FT_GetDeviceInfoList(nodes, &count)) != FT_OK) {
		fprintf(stderr, "FT_GetDeviceInfoList failed : %d\n", res);
		exit(1);
	}

	// use the first device returned
	std::string serialNum = nodes[0].SerialNumber;

	return serialNum;	
}
IcomIQPort::~IcomIQPort()
{
    FT_AbortPipe(handle, CMD_OUT);
    FT_AbortPipe(handle, CMD_IN);
    FT_AbortPipe(handle, IQ_IN);
	if (_running) {
		_read_thread.join();
	}
    FT_Close(handle);
}
void IcomIQPort::close()
{
	if (_isOpen)
	{
		FT_AbortPipe(handle, CMD_OUT);
		FT_AbortPipe(handle, CMD_IN);
		FT_AbortPipe(handle, IQ_IN);
		FT_Close(handle);
		_isOpen = false;
	}
}

void IcomIQPort::print_vector(std::vector<uint8_t> v)
{
	for (long long int i = 0; i < v.size(); i++) 
	{
		printf("%02X ", v[i]);
	}
	printf("\n");	
}

// Helper function to check and print FT_STATUS errors
void IcomIQPort::CheckStatus(FT_STATUS status, const char* functionName) {
    if (status != FT_OK) {
        std::cerr << "Error in " << functionName << ": Status " << status << std::endl;
        // TBD add a lookup for the status code string here for better error messages
    }
}

bool IcomIQPort::sendIQCommand( std::vector<uint8_t> cmd)
{
    uint8_t buf[32] = {CIV_PREAMBLE, CIV_PREAMBLE, RADIO_ADDR, PC_ADDR};
    DWORD buf_size = 4;
    FT_STATUS res;
    DWORD count;

	//SoapySDR_logf(SOAPY_SDR_INFO, "send_cmd  called");
	//fprintf(stderr, "send_cmd  called\n");
    for (size_t i = 0; i < cmd.size(); i++) {
        buf[buf_size++] = cmd[i];
    }
    buf[buf_size++] = 0xfd;
    for (size_t i = buf_size; i % 4 > 0; i++) {
        buf[buf_size++] = 0xff;
    }

    //for (size_t i = 0; i < buf_size; i++) {
        //SoapySDR_logf(SOAPY_SDR_INFO, "sending command: %02x", buf[i]);
		//fprintf(stderr, "sending command: %02x\n", buf[i]);
    //}
    if ((res = FT_WritePipe(handle, CMD_OUT, buf, buf_size, &count, 0)) != FT_OK) {
                //SoapySDR_logf(SOAPY_SDR_INFO,"FT_WritePipe: %ld\n", res);
				//fprintf(stderr,"FT_WritePipe: %ld\n", res);
                return false;
        }
        if (count != buf_size) {
                //SoapySDR_logf(SOAPY_SDR_INFO,"FT_WritePipe wrote %d bytes, but we wanted %d\n", (int)count, (int)buf_size);
				//fprintf(stderr,"FT_WritePipe wrote %d bytes, but we wanted %d\n", (int)count, (int)buf_size);
                return false;
        }
        return true;
}

int IcomIQPort::readIQReply( std::vector<uint8_t>& buffer)
{
        uint8_t buf[_civBufferSize];
        DWORD i;
        FT_STATUS res;
        DWORD count;

		//SoapySDR_logf(SOAPY_SDR_INFO, "read_reply  called");
		//fprintf(stderr, "read_reply  called\n");
		
        if ((res = FT_ReadPipe(handle, CMD_IN, buf, sizeof(buf), &count, 0)) != FT_OK) {
                //SoapySDR_logf(SOAPY_SDR_INFO,"FT_ReadPipe: %ld\n", res);
				//fprintf(stderr,"FT_ReadPipe: %ld\n", res);
                res = FT_AbortPipe(handle, CMD_IN);
                //SoapySDR_logf(SOAPY_SDR_INFO,"FT_AbortPipe: %ld\n", res);
				//fprintf(stderr,"FT_AbortPipe: %ld\n", res);
                return 0;
        }



        //std::vector<uint8_t> vect = {};
        for (i = 0; i < count; i++)
        {
			//fprintf(stderr, "reading command: %02x\n", buf[i]);
            buffer.push_back(buf[i]);
        }

        return buffer.size();
}
	
int IcomIQPort::icomIQCommand( std::vector<uint8_t> cmd, std::vector<uint8_t> &reply)
{
	//SoapySDR_logf(SOAPY_SDR_INFO, "icom_cmd called");
	//fprintf(stderr, "icom_cmd called\n");
    bool res = sendIQCommand(cmd);
	//send_cmd(cmd);
	//what to do if res is false - unable to send command  read_reply should return a {} vector
	if (res)
	{
		//SoapySDR_logf(SOAPY_SDR_INFO, "send_cmd completed");
		//fprintf(stderr, "send_cmd completed\n");
		int byte_count = readIQReply(reply);
		if (byte_count > 0)
		{
			//for (size_t i = 0; i < reply.size(); i++) {
				//SoapySDR_logf(SOAPY_SDR_INFO, "reply %02x", reply[i]);
				//fprintf(stderr, "reply %02x\n", reply[i]);
			//}
			return reply.size();
		}
    }
    return 0;
}


void IcomIQPort::setTimeout(uint8_t channelID, int timeOut)
{
	FT_STATUS ftStatus;
	// if we are setting timeout on the CMD channels, we need to set them the same
	if (channelID == CMD_IN || channelID == CMD_OUT)
	{
		ftStatus = FT_SetPipeTimeout(handle, CMD_IN, timeOut);
		ftStatus = FT_SetPipeTimeout(handle, CMD_OUT, timeOut);
		// if set was unsuccessful, do not update _timeout value
		if (ftStatus == FT_OK)
		{
			_cmdTimeout = timeOut;
		} else {
			fprintf(stderr,"Set timeout on Command Channel %0x failed status= %ld\n", channelID, ftStatus);			
			// if set was unsuccessful, do not update _timeout value
		}
	} else {
		ftStatus = FT_SetPipeTimeout(handle, channelID, timeOut);
		// if set was unsuccessful, do not update _timeout value
		if (ftStatus == FT_OK)
		{
			_dataTimeout = timeOut;
		} else {
			fprintf(stderr,"Set timeout on Data Channel  %0x failed status= %ld\n", channelID, ftStatus);			
			// if set was unsuccessful, do not update _timeout value
		}
	}

}

bool IcomIQPort::enableIQData(uint8_t source)
{
	if (!_iqDataEnabled)
	{
		// note: source (vfo) are 0 (main) and 1 (sub). enable commands are 0 (off), 1 (main), 2(sub)
		std::vector<uint8_t> command = {0x1a, 0x0b, (uint8_t)(source + 1)};
		std::vector<uint8_t> reply;
		int byte_count = icomIQCommand(command, reply);
		if (byte_count > 0) {
		_iqDataEnabled = true;
		}
	}
	return _iqDataEnabled;
}

void IcomIQPort::disableIQData()
{
	std::vector<uint8_t> command = {0x1a, 0x0b, 0x00};
	std::vector<uint8_t> reply;
	int byte_count = icomIQCommand(command, reply);
	if (byte_count > 0) {
		_iqDataEnabled = false;
	}
	
}
	
//int IcomIQPort::readIQData(uint8_t *buffer, size_t buffer_size, void* overlapped_)
int IcomIQPort::readIQData(std::complex<short> *buffer, size_t buffer_size, void* overlapped_)
{
	// make sure buffer_size is multiple of 512 bytes (128 complex<short>, or adjust to lower size
	// to not over run the buffer;
	size_t alined_size = (buffer_size / 128) * 128;
	DWORD bytesTransferred = 0;
    //DWORD bytesToRead = (DWORD)buffer_size; // Size of the buffer to read
	DWORD bytesToRead = (DWORD) alined_size * sizeof(std::complex<short>); // Size of the buffer to read

	int res;
	
	OVERLAPPED* vOverlapped = static_cast<OVERLAPPED*>(overlapped_);
	uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(buffer);

	if (_iqDataEnabled){

		
		if ((res = FT_ReadPipe(handle, 
				IQ_IN, 					// Pipe ID (Endpoint address)
				byte_ptr, 				// Pointer to the buffer to read data into
				bytesToRead, 			// Number of bytes to read
				&bytesTransferred, 		// Pointer to store the number of bytes actually read
				((overlapped_ != nullptr)?vOverlapped : NULL)	// Reserved, must be NULL for synchronous operation, not null for async
				)
			) != FT_OK) {
				if (res == FT_IO_PENDING && (overlapped_ != nullptr))
				{
					fprintf(stderr,"FT_ReadPipe: FT_IO_PENDING (%d) \n", res);
					res = FT_GetOverlappedResult(handle, vOverlapped, &bytesTransferred, TRUE);
					
					// tbd handle overlapped errors
				} else
				{
					fprintf(stderr,"FT_ReadPipe: %d\n", res);
					return -((int)(res));
				}
			}
	} else {
		return -32;
	}
	int samplesRead = bytesTransferred / sizeof(std::complex<short>);
	return samplesRead;
}



std::string IcomIQPort::iqGetChipConfiguration()
{
	FT_STATUS ftStatus;
	std::cout << "\nChip Configuration:" << std::endl;
	FT_60XCONFIGURATION chipConfiguration;
    // Initialize the structure to zero
    ZeroMemory(&chipConfiguration, sizeof(chipConfiguration));
	
    ftStatus = FT_GetChipConfiguration(handle, &chipConfiguration);
    CheckStatus(ftStatus, "FT_GetChipConfiguration");
	
	std::vector<char> buffer(256);
	if (ftStatus == FT_OK) {
		//printf("Chip Configuration retrieved successfully:\n");
		//printf("	Vendor Id = %d, Producxt ID = %d \n", chipConfiguration.VendorID, chipConfiguration.ProductID);
		std::sprintf(buffer.data(),"Vendor Id = %d, Producxt ID = %d \n", chipConfiguration.VendorID, chipConfiguration.ProductID);
		//printf("	Description: %s \n", chipConfiguration.StringDescriptors);
		std::sprintf(buffer.data() + buffer.size(),"	Description: %s \n", chipConfiguration.StringDescriptors);
		//printf("	bInterrupt Interval: %#02x, Power Attribute: %#02x, Power Consumption: %d \n", chipConfiguration.bInterval, 
		//			chipConfiguration.PowerAttributes, chipConfiguration.PowerConsumption);
		std::sprintf(buffer.data() + buffer.size(),"	bInterrupt Interval: %#02x, Power Attribute: %#02x, Power Consumption: %d \n",
						chipConfiguration.PowerAttributes, chipConfiguration.PowerConsumption);
		//printf("	FIFO Clock: %#02x, FIFO Mode: %#02x, Channel Configuration: %#02x \n", chipConfiguration.FIFOClock,
		//			chipConfiguration.FIFOMode, chipConfiguration.ChannelConfig);
		std::sprintf(buffer.data() + buffer.size(),"	FIFO Clock: %#02x, FIFO Mode: %#02x, Channel Configuration: %#02x \n",
					chipConfiguration.FIFOClock,chipConfiguration.FIFOMode, chipConfiguration.ChannelConfig);
		
	} else {
		std::sprintf(buffer.data(), "Call to iqGetChipConfiguration() failed. FT_STATUS: %d", ftStatus);
	};

	std::string result(buffer.data());
	return result;
}

std::string IcomIQPort::iqGetDevicveDescriptor()
{
	FT_STATUS ftStatus;
	std::cout << "\nChip Device Configuration:" << std::endl;
	FT_DEVICE_DESCRIPTOR deviceConfiguration;
    // Initialize the structure to zero
    ZeroMemory(&deviceConfiguration, sizeof(deviceConfiguration));
	std::vector<char>buffer(256);
    ftStatus = FT_GetDeviceDescriptor(handle, &deviceConfiguration);
    CheckStatus(ftStatus, "FT_GetChipConfiguration");
	
	if (ftStatus == FT_OK) {
		//printf("Device Configuration retrieved successfully:\n");
		//printf("	Vendor Id = %d, Producxt ID = %d \n", deviceConfiguration.idVendor, deviceConfiguration.idProduct);
		//printf("	Number of Configurations Id = %d\n", deviceConfiguration.bNumConfigurations);
		std::sprintf(buffer.data(),"	Vendor Id = %d, Producxt ID = %d,  Number of Configurations Id: %d",
			deviceConfiguration.idVendor, deviceConfiguration.idProduct, deviceConfiguration.bNumConfigurations);
	}
	else {
		std::sprintf(buffer.data(),"Call to iqGetChipConfiguration() failed. FT_STATUS: ", ftStatus);
	}
	std::string result(buffer.data());
	return result;
}

int IcomIQPort::iqGetRFGain(uint8_t vfo)
{
	if (_isOpen)
	{
		int received_bytes = 0;
		std::vector<uint8_t> command = {0x29, vfo, 0x14, 0x02 }; // read main band rf gain
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);
		//fprintf(stderr,"Main gain: Received %d bytes\n", received_bytes);
		if (received_bytes > 0) {
			//reply message is header:command:<data>:end:fill
			// header is always 4 bytes (0-3)
			// command is size of command command.size() +1 to get to first data byte
			
			int data_index = CMD_INDEX + command.size() ;
			//fprintf(stderr,"Main RF Gain data_index = %d \n",data_index);
			
			int data_multiplier = 100;
			uint32_t value = 0;
			
			while(reply[data_index] != 0xfd)
			{
				value = value + read_bcd(reply[data_index])   * data_multiplier;
				data_multiplier /= 100;
				data_index++;
			}
			//fprintf(stderr,"RF Main Gain = %d\n", value);
			return value;			
		}
	}
	return 0;
}

int IcomIQPort::iqSetRFGain(uint8_t vfo, int rfgain)
{
	if (_isOpen)
	{
		int received_bytes = 0;
		std::vector<uint8_t> command = {0x29, vfo, 0x14, 0x02 }; // set main band rf gain
		// rfgain is limited to 0 to 255
		// add test and return error of 255
		command.push_back(bcd_digits(rfgain, 100));
		command.push_back(bcd_digits(rfgain, 1));
		//printf("SetRFGain Command \n");
		//print_vector(command);
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);
		fprintf(stdout,"Main gain: Received %d bytes\n", received_bytes);
		if (received_bytes > 0) {
			//printf("Received_bytes = %d\n", received_bytes);
			//print_vector(reply);
			//printf("CIV_ACK_NAK_OFFSET = %d\n",CIV_ACK_NAK_OFFSET);
			// return is Ack or Nack
			if (reply[CIV_ACK_NAK_OFFSET] == CIV_ACK)
			{
				return rfgain;
			}
		}
	}
	return (-1);
}
	
uint8_t IcomIQPort::iqGetAttenuatorSettings(uint8_t vfo)
{
	// attenuator
	//fprintf(stderr,"Main Attenuator\n");
	
	std::vector<uint8_t> command = {0x29, vfo, 0x11 }; // Attenuator
	if (_isOpen)
	{
		int received_bytes = 0;
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);
		//fprintf(stderr,"Main Attehuator: Received %d bytes\n", received_bytes);
		if (received_bytes > 0) {
			int data_index = CMD_INDEX + command.size();
			//fprintf(stderr,"Attenuatore data_index = %d \n",data_index);
			/*
			if (reply[data_index] == 0x00)
			{
				//fprintf(stderr,"Attenuator is OFF\n");
			} else {
				//fprintf(stderr,"Atenuator = %d\n",reply[data_index]);
			}
			*/
			return read_bcd(reply[data_index]);
		}
		
	}
	return (uint8_t)0xff;
}

uint8_t IcomIQPort::iqSetAttenuatorSettings(uint8_t vfo, int attenuation)
{
	std::vector<int> validAttenuation {0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45};
	std::vector<uint8_t> command = {0x29, vfo, 0x11 }; // Attenuator
	int value = -1;
	int received_bytes = 0;
	std::vector<uint8_t> reply;
    auto it = std::find( validAttenuation.begin(),
                         validAttenuation.end(),
                             attenuation);	
	if (it != validAttenuation.end())
	{
		if (_isOpen)
		{
			
			command.push_back(bcd_digits(attenuation, 1));
			//printf("Attentuation Command \n");
			//print_vector(command);
			received_bytes = icomIQCommand(command, reply);
			//printf("Main Attehuator: Received %d bytes\n", received_bytes);
			//print_vector(reply);
			if (received_bytes > 0) {
				
				if (reply[CIV_ACK_NAK_OFFSET] == CIV_ACK)
				{
					//printf("SetAttenuation received an ACK\n");
					value = attenuation;
				}
									
			}
			
		}
	}
	return value;
}

std::string IcomIQPort::iqGetAntenna(uint8_t vfo)
{

	std::vector<uint8_t> command = {0x29, vfo, 0x12 }; // Antenna
	std::string antenna= "";
	if (_isOpen)
	{
		int received_bytes = 0;
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);
		//fprintf(stderr,"Main Antenna: Received %d bytes\n", received_bytes);
		if (received_bytes > 0) {
			int data_index = CMD_INDEX + command.size();
			//fprintf(stderr,"Main Atenna data_index = %d \n",data_index);
			
			//fprintf(stderr, "Antenna %s : %s\n", ((reply[data_index] == 0x00)?"ANT1":"ANT2"),
			//		((reply[data_index + 1] == 0x00)?"OFF":"ON"));		
			antenna.append((reply[data_index] == 0x00)?"ANT1":"ANT2") ;
			antenna.append( "_") ;
			antenna.append((reply[data_index + 1] == 0x00)?"OFF":"ON");
			
		}
	}
		return antenna;
}

int IcomIQPort::iqSetAntenna(uint8_t vfo, int antenna, bool status)
{
	//If the RX-ANT Connector is set to "Connect External RX Device""OFF" OFF will be returned
	//If "ON" is sent, NG (0xFA) will return
	// //
	// antenna must be 0 or 1
	// status is true -> On. false -> Off
	std::vector<uint8_t> command = {0x29, vfo, 0x12 }; // Antenna
	int value = -1;
	if (antenna <0 || antenna > 1) 
	{
		return value; // error
	}
		
	if (_isOpen) {
		command.push_back((uint8_t) antenna);
		command.push_back((status)? 0x01: 0x00);
		int data_index = CMD_INDEX + command.size();
		int received_bytes = 0;
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);
		// We should receive an ACK or NAK, Other conditon is to receive a formal message
		if (received_bytes >= data_index)
		{
			//formal message simular to GetAntenna
			
			//printf("iqSetAntenna reply:\n");
			//print_vector(reply);
			value = 1;
		} else
		{
			// eithr an ack or a nak. assume nak
			if (reply[CIV_ACK_NAK_OFFSET] == CIV_ACK)
			{
				value = 1;
			}
		}
		value = 0;
	}
	return value;
}

int IcomIQPort::iqGetPreAmpStatus(uint8_t vfo)
{
	std::vector<uint8_t> command = {0x29, vfo, 0x16, 0x02 }; // Preamps
	int value = -1;
	if (_isOpen)
	{
		int received_bytes = 0;
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);	
		if (received_bytes > 0) {
			int data_index = CMD_INDEX + command.size();

			value =(int)reply[data_index];
		}
	}
	return value;
}

bool IcomIQPort::iqGetDIGI_SEL_Status(uint8_t vfo)
{
	std::vector<uint8_t> command = {0x29, vfo, 0x16, 0x4E }; // DIGI-SEL
	bool value = false;
	if (_isOpen)
	{
		
		int received_bytes = 0;
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);	
		if (received_bytes > 0) {
			//printf("iqGetDEL-SEL_Status reply:\n");
			//print_vector(reply);
			int data_index = CMD_INDEX + command.size();
			
			if (reply[data_index] == 0x01)
			{
				value = true;
			} 
		}
	}
	return value;
}

void IcomIQPort::iqSetDIGI_SEL_Status(uint8_t vfo, bool status)
{
	std::vector<uint8_t> command = {0x29, vfo, 0x16, 0x4E }; // DIGI-SEL

	if (_isOpen)
	{
		command.push_back(status? 0x01 : 0x00);
		//printf("iqSetDIGI_SEL_Status command:\n");
		//print_vector(command);
		int received_bytes = 0;
		int data_index = CMD_INDEX + command.size();
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);	
		if (received_bytes >= data_index) {
			printf("iqSetDEL-SEL_Status reply:\n");
			print_vector(reply);
		} else {	
			
			if (reply[CIV_ACK_NAK_OFFSET] == 0xFB)
			{
				//printf("DIGI-SEL set %s\n", (status?"ON" :"OFF"));
				_DIGI_SEL = status;
			} else
			{
				//printf("DIGI-SEL set failed\n");
				// do not change
			}
		}
	}

}

bool IcomIQPort::iqGetIP_Status(uint8_t vfo)
{
	std::vector<uint8_t> command = {0x29, vfo, 0x16, 0x65 }; // IP+ function
	bool value = false;
	if (_isOpen)
	{
		int received_bytes = 0;
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);	
		if (received_bytes > 0) {
			int data_index = CMD_INDEX + command.size();
			//printf("iqGetIP_Status reply:\n");
			//printf(" data_index = %d \n", data_index);
			//print_vector(reply);

			
			if (reply[data_index] == 0x01)
			{
				value = true;
				//printf("return true\n");
			} 
		}
	}
	return value;
}

void IcomIQPort::iqSetIP_Status(uint8_t vfo, bool status)
{
	std::vector<uint8_t> command = {0x29, vfo, 0x16, 0x65 }; // DIGI-SEL
	int state = 0;
	if (_isOpen)
	{
		command.push_back(status? 0x01 : 0x00);
			//printf("iqSetIP_Status command:\n");
			//print_vector(command);
		int received_bytes = 0;
		int data_index = CMD_INDEX + command.size();
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);	
		if (received_bytes >= data_index) {
			printf("iqSetIP_Status reply:\n");
			print_vector(reply);
		} else {	
			
			if (reply[CIV_ACK_NAK_OFFSET] == 0xFB)
			{
				//printf("IP-Status set %s\n", (status?"ON" :"OFF"));
				_IPPlus = status;
			} else
			{
				//printf("IP-Status set failed \n");
				// do not change setting
			}
		}
	}

}

int IcomIQPort::iqGetFrequency(uint8_t vfo)
{
	//fprintf(stderr,"Main Frequency\n");
	std::vector<uint8_t> command = {0x25, vfo }; 
	if (_isOpen)
	{
		int received_bytes = 0;
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);
		//fprintf(stderr,"Main Frequency: Received %d bytes\n ", received_bytes);
		if (received_bytes > 0) {
			//int data_multiplier = 1;
			//uint32_t value = 0;
			int data_index = CMD_INDEX + command.size();
			//fprintf(stderr,"Frequency data_index = %d \n", data_index);
			
			uint32_t value = read_bcd(reply[data_index])   * 1 +
					 read_bcd(reply[data_index+1]) * 100 +
					 read_bcd(reply[data_index+2]) * 10000 +
					 read_bcd(reply[data_index+3]) * 1000000 +
					 read_bcd(reply[data_index+4]) * 100000000;
					
			//fprintf(stderr,"Frequency = %d\n", value);
			return value;
		}
	}
	return 0;	
}

int IcomIQPort::iqSetFrequency(uint8_t vfo, uint32_t f)
{
	//fprintf(stderr,"Main Frequency\n");
	int value = -1;
	std::vector<uint8_t> command = {0x25, vfo }; 
	command.push_back(bcd_digits(f, 1));
    command.push_back(bcd_digits(f, 100));
    command.push_back(bcd_digits(f, 10000));
    command.push_back(bcd_digits(f, 1000000));
    command.push_back(bcd_digits(f, 100000000));
	
	int data_index = CMD_INDEX + command.size();
	//	printf("SetFrequency data_index = %d \n", data_index);
	//	printf("iqSetIP_Status command:\n");
	//	print_vector(command);
		
	if (_isOpen)
	{
		
		int received_bytes = 0;
		std::vector<uint8_t> reply;
		received_bytes = icomIQCommand(command, reply);
		//fprintf(stderr,"Main Frequency: Received %d bytes\n ", received_bytes);
		// we can get a fully qualified message or just and ACK or NAC
		// check for full message
		if (received_bytes >= data_index) {
			
		//	printf("SetFrequency Full reply:\n");
		//	print_vector(reply);
			value = read_bcd(reply[data_index])   * 1 +
					read_bcd(reply[data_index+1]) * 100 +
					read_bcd(reply[data_index+2]) * 10000 +
					read_bcd(reply[data_index+3]) * 1000000 +
					read_bcd(reply[data_index+4]) * 100000000;					
			//fprintf(stderr,"Frequency = %d\n", value);			
		} else if (received_bytes == IQ_ACK_NAK_MSG_LEN)
		{
		//	printf("SetFrequency ACK_NAK reply:\n");
		//	print_vector(reply);
			//got an ACK or NAK, assume a NAK
			if (reply[CIV_ACK_NAK_OFFSET] == 0xFB)
			{
				// got a ACK, return f
				value = (int) f;
			}
		}
	}
	return value;	
}


bool IcomIQPort::iqSetStreamPipe(uint8_t channelID, size_t streamBufferSize)
{
	bool success = true;
	FT_STATUS ft_status = FT_OK;
	ft_status = FT_SetStreamPipe(handle, false, false, channelID, streamBufferSize);
	if (ft_status != FT_OK){
		success = false;
		fprintf(stderr,"SetStreamPipe failed errorcode = %ld\n", ft_status);
	}
	
	return (int)success;
}

bool IcomIQPort::iqClearStreamPipe(uint8_t channelID)
{
	bool success = true;
	FT_STATUS ft_status = FT_OK;
	ft_status = FT_ClearStreamPipe(handle, false, false, channelID);
	if (ft_status != FT_OK){
		success = false;
		fprintf(stderr,"ClearStreamPipe failed errorcode = %ld\n", ft_status);
	}
	
	return (int)success;
}


int IcomIQPort::iqAbortPipe(uint8_t pipe)
{
	FT_STATUS status;
	status = FT_AbortPipe(handle, pipe);
	if (status != FT_OK) status = - status;
	return status;	
}


bool IcomIQPort::iqAsyncStart(uint8_t vfo)
{
	bool result = false;
	if (!_isOpen) {
		return result;
	};	
	if (_running) {
		// already running
		return result;
	};


	// check to see if we neet to enable data
	if (enableIQData(vfo))
	{
		
		//FT_STATUS ftStatus = FT_SetStreamPipe(handle, FALSE, FALSE, IQ_IN); 
		//if (ftStatus != FT_OK) {
			_read_thread = std::thread(&IcomIQPort::iqAsyncReadWorker, this);
			_running = true;
			result = true;
		//}

	}
	
	return result;
}
	
void IcomIQPort::iqAsyncStop() {
	FT_STATUS status;
	
	// set running to false
	_running = false;
	// check if we have a valid handle, then abort pipe
    if (handle != nullptr) {
        status = FT_AbortPipe(handle, IQ_IN);
		if (status != FT_OK) {
			std::cerr << "Failed to abort pipe. Status: " << status << std::endl;
			// Handle error as appropriate
		}
		// disable pipe ??
        status = FT_AbortPipe(handle, CMD_IN);
		if (status != FT_OK) {
			std::cerr << "Failed to abort pipe. Status: " << status << std::endl;
			// Handle error as appropriate
		}
        status = FT_AbortPipe(handle, CMD_OUT);
		if (status != FT_OK) {
			std::cerr << "Failed to abort pipe. Status: " << status << std::endl;
			// Handle error as appropriate
		}
    }	

	// if streams are used, consider adding ClearStreamPort
	// check if a thread exist
    if (_read_thread.joinable()) {
        _read_thread.join();
    }
	
	
	//FT_STATUS ftStatus = FT_ClearStreamPipe(handle, FALSE, FALSE, IQ_IN);
	disableIQData();
	//iqBuffer_.clear_ringbuffer();
}		
/*
std::vector<uint8_t> IcomIQPort::iqGetLatestData() { // check gnuradio stream buffers usage
	std::vector<uint8_t> latest_data; 
	
	std::unique_lock<std::mutex> lock(_data_mutex);
    //std::lock_guard<std::mutex> lock(_data_mutex);
	//bool queueEmpty = _data_queue.empty();
	//bool dataReady = _data_ready;
	_data_cv.wait(lock, [this]{ return (!_data_queue.empty() || _data_ready); });
	
    if (_data_queue.empty() && _data_ready) {
		_data_ready = false;
		lock.unlock();
        return latest_data; // Exit loop if finished
    };	
	// push all available data, typically 1 buffer
	while ((!_data_queue.empty()))
	{
		DataPacket packet = std::move(_data_queue.front());
		_data_queue.pop();
		latest_data = std::move(packet.dataVector);
	}
	_data_ready = false;
    lock.unlock(); // Unlock while processing	

    return latest_data;
}
*/

int IcomIQPort::iqInitializeOverlapped(void* overlapped_ )
{
	OVERLAPPED* vOverlapped = static_cast<OVERLAPPED*>(overlapped_);
	FT_STATUS ft_status;
	ZeroMemory(vOverlapped, sizeof(vOverlapped)); // Zero the structure
	// other option
	//vOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // Create a manual-reset event
	ft_status = FT_InitializeOverlapped(handle,vOverlapped);
	if (ft_status != FT_OK){
		ft_status = - ft_status;
	}
	return ft_status;
}

int IcomIQPort::iqReleaseOverlapped(void* overlapped_)
{
	OVERLAPPED* vOverlapped = static_cast<OVERLAPPED*>(overlapped_);
	FT_STATUS ft_status;
	ft_status = FT_ReleaseOverlapped(handle,vOverlapped);
	if (ft_status != FT_OK){
		ft_status = - ft_status;
	}
	return ft_status;	
}

int  IcomIQPort::iqReadBuf(std::complex<short> *data, size_t numSamples)
{
	//std::cout << " iqReadBuf called" << std::endl;
	int samplesRead = iqBuffer_.readbuf(data, numSamples);
	//std::cout << " iqBuffer returned" << std::endl;
	return samplesRead;
}

size_t IcomIQPort::iqGetSizeOfAvailableData(){
	size_t availableData = 0;
	availableData = iqBuffer_.available_data();
	return availableData;
}

void IcomIQPort::iqAsyncReadWorker() {

    FT_STATUS ft_status = FT_OK;
        // FT_ReadPipeAsync is the D3XX asynchronous read function
        // start by gettingthe overlap 
	uint8_t temp_buffer[IQ_BLOCK_LENGTH] = {0xff};
	DWORD bytes_read = 0;
	OVERLAPPED vOverlapped;
	ZeroMemory(&vOverlapped, sizeof(vOverlapped)); // Zero the structure
	//vOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // Create a manual-reset event

	ft_status = FT_InitializeOverlapped(handle,&vOverlapped);
	//queue the initial batch request
	ft_status = FT_ReadPipe(
		handle,
		IQ_IN,           // Endpoint address (e.g., BULK IN 2)
		temp_buffer,
		IQ_BLOCK_LENGTH,
		&bytes_read,
		&vOverlapped       // Use an appropriate timeout or manage with OS events
	);
	
	memset(temp_buffer, 0xff, sizeof(IQ_BLOCK_LENGTH));

    while (_running) {
		
		std::vector<uint8_t> data_buffer;
		DWORD bytes_read = 0;
        // FT_ReadPipeAsync is the D3XX asynchronous read function
        // start by gettingthe overlap 
		// wait for compleation
		//std::cerr << "FT_ReadPipeAsync waiting for compleation: " << ft_status << std::endl;
		ft_status = FT_GetOverlappedResult(handle, &vOverlapped, &bytes_read, true);
		//std::cerr << "FT_ReadPipeAsync completed: " << ft_status << std::endl;

		if (ft_status == FT_OK && bytes_read > 0)
		{

			// Move the data for a DataPacket
			
			// check temp_size to make sure the number of byes is a multiple of sample size
			if ((bytes_read % BYTES_PER_SAMPLE) != 0) {
				std::string errorMessage = "Byte data size not a multiple of: " + 
						std::to_string(BYTES_PER_SAMPLE);
				throw std::runtime_error(errorMessage);	
			}
			
			//data_buffer.dataVector.insert(data_buffer.dataVector.begin(), temp_buffer, temp_buffer + bytes_read);
			//data_buffer.dataLen = bytes_read;
			
			// save data in the ring buffer	
			iqBuffer_.writebuf(&temp_buffer[0], bytes_read);
			/*
			// prevent access by both threads
			{
				//std::lock_guard<std::mutex> lock(_data_mutex);
				_data_queue.push(std::move(data_buffer));
				//exiting removes lock
				_data_ready = true;
			}
			*/
            // clear temp buffer
			memset(temp_buffer, 0xff, sizeof(_buffer_size));
			// queue next read
			ft_status = FT_ReadPipe(
				handle,
				IQ_IN,           // Endpoint address (e.g., BULK IN 2)
				temp_buffer,
				IQ_BLOCK_LENGTH,
				&bytes_read,
				&vOverlapped       // Use an appropriate timeout or manage with OS events
			);
			// notify compleation			
			//_data_cv.notify_one(); // Notify any waiting consumer threads
		} else 	if (ft_status == FT_TIMEOUT) {
				//std::cerr << "FT_ReadPipeAsync timedout: " << ft_status << std::endl;
				ft_status = FT_ReadPipe(
					handle,
					IQ_IN,           // Endpoint address (e.g., BULK IN 2)
					temp_buffer,
					IQ_BLOCK_LENGTH,
					&bytes_read,
					&vOverlapped       // Use an appropriate timeout or manage with OS events
				);
		} else {
            std::cerr << "FT_ReadPipeAsync failed with status: " << ft_status << std::endl;
            // Handle error, maybe break the loop, or if timeout handle timeout
			this->iqAsyncStop(); // ??
            break;
        }
        // Note: The driver handles the asynchronous nature, this loop manages the submission/completion cycle.
    }
	FT_ReleaseOverlapped(handle, &vOverlapped);
}	