/***********************************************************************
 * SoapySDR registration interface for IC7610
 * Note - commens are from SoapySDR Documentation
 **********************************************************************/


#include "SoapyIC7610SDR.hpp"
//#include "IcomCIVPort.hpp"
#include "IcomIQPort.hpp"
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Logger.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <libserialport.h> // Assumes you have the serial library set up
#include "ftd3xx.h"

#include <windows.h>

#include <atomic>


static std::vector<SoapySDR::Kwargs> results;



/***********************************************************************
 * Functions to Find available devices IQ Ports
 * find_IC7610SDR: searches the available usb ports for the IQ Port and 
 * 		port number used to control the 7610 radio. This is done by using the VID and PID
 *
 *
 * Note this is a "free" function defined at a global scope
 **********************************************************************/
static std::vector<SoapySDR::Kwargs> find_IC7610SDR(const SoapySDR::Kwargs &args) 
{
    //(void)args;
	// The FT_DEVICE_LIST_INFO_NODE is a stucture consisting of information about the 
	// device (Flags, Type, ID, serial number, description and handle
	// change number of devices (currently 16) to a constant
    FT_DEVICE_LIST_INFO_NODE nodes[16];
    DWORD count;
    int res;
	
	//std::string civ_name = IcomCIVPort::findIcomCtrlPort();
	//if (civ_name.empty())
	//	SoapySDR_logf(SOAPY_SDR_ERROR, "Unable to find IC7610 control port");
	
    std::vector<SoapySDR::Kwargs> results;
	// First check for a IC7610 Serial Port
	
	// Create a list of devices
	// 1) get the number of devices that are available: returned in count
    if ((res = FT_CreateDeviceInfoList(&count)) != FT_OK) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "FT_CreateDeviceInfoList failed: %d\n", res);
        return SoapySDR::KwargsList();
    }
	
	// 2) allocate space and get the device information for each device found:

    if ((res = FT_GetDeviceInfoList(nodes, &count)) != FT_OK) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "FT_GetDeviceInfoList failed: %d\n", res);
        return SoapySDR::KwargsList();
    }

    if (count == 0) {
        SoapySDR_logf(SOAPY_SDR_INFO, "No FTDI devices found");
    }

    for (DWORD i = 0; i < count; i++) {
        SoapySDR_logf(SOAPY_SDR_INFO, "Device[%d]", i);
        SoapySDR_logf(SOAPY_SDR_INFO, "\tFlags: 0x%x %s | Type: %d | ID: 0x%08X",
                nodes[i].Flags,
                nodes[i].Flags & FT_FLAGS_SUPERSPEED ? "[USB 3]" :
                nodes[i].Flags & FT_FLAGS_HISPEED ? "[USB 2]" :
                nodes[i].Flags & FT_FLAGS_OPENED ? "[OPENED]" : "",
                nodes[i].Type,
                nodes[i].ID);
        SoapySDR_logf(SOAPY_SDR_INFO, "SerialNumber=%s\n", nodes[i].SerialNumber);
        SoapySDR_logf(SOAPY_SDR_INFO, "Description=%s\n", nodes[i].Description);

        SoapySDR::Kwargs deviceInfo;
        deviceInfo["label"] = std::string(nodes[i].Description) + " :: " + nodes[i].SerialNumber;
        deviceInfo["product"] = nodes[i].Description;
        deviceInfo["serial"] = nodes[i].SerialNumber;
        deviceInfo["manufacturer"] = "Icom";

        deviceInfo["vfo"] = "MAIN";

        //filtering by serial
        if (args.count("serial") != 0 and args.at("serial") != nodes[i].SerialNumber) continue;

        results.push_back(deviceInfo);
    }
	// free(nodes)
    return results;
}

static SoapySDR::Device *make_IC7610SDR(const SoapySDR::Kwargs &args)
{
    (void)args;
    //create an instance of the device object given the args
    //here we will translate args into something used in the constructor
    return new SoapyIC7610SDR(args);
}


// the registration for the driver. 
static SoapySDR::Registry register_ic7610("SoapyIC7610SDR", &find_IC7610SDR, &make_IC7610SDR, SOAPY_SDR_ABI_VERSION);