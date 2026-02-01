//
// g++ -I../include findDevice.cpp  -o findDevice.exe -L ../libs -lftd3xx
//  ./findDevice.exe


#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include "ftd3xx.h"

#define CMD_OUT 0x02
#define CMD_IN  0x82
#define IQ_IN   0x84
#define TIMEOUT 100 // ms
#define READ_SIZE 256 * 1024

#define CMD_INDEX 4
#define SUBCMD_INDEX 5
#define DATA_INDEX 6

int debug = 1;

std::string get_device_list()
{
	FT_DEVICE_LIST_INFO_NODE nodes[16];
	DWORD count;
	int res;
 
	
	printf("get_device_list() called\n");
	
	if ((res = FT_CreateDeviceInfoList(&count)) != FT_OK) {
		printf("FT_CreateDeviceInfoList: %d\n", res);
		exit(1);
	}
	

	printf("FT_CreateDeviceInfoList succeeded\n");
		
	if ((res = FT_GetDeviceInfoList(nodes, &count)) != FT_OK) {
		printf("FT_GetDeviceInfoList: %d\n", res);
		exit(1);
	}
	
	printf("FT_GetDeviceInfoList succeeded. count=%d\n", (int)count);
	std::string serialNum = "";
	if (count > 0){
		printf("Output Device Information -----------\n");
		for (DWORD i = 0; i < count; i++)
		{
			printf("Device Flags--------\n");
			printf("Device[%d]\n", (int)i);
			printf("\tFlags: 0x%lx %s | Type: %ld | ID: 0x%08lx\n",
					(long unsigned int)nodes[i].Flags,
					nodes[i].Flags & FT_FLAGS_SUPERSPEED ? "[USB 3]" :
					nodes[i].Flags & FT_FLAGS_HISPEED ? "[USB 2]" :
					nodes[i].Flags & FT_FLAGS_OPENED ? "[OPENED]" : "",
					(long int)nodes[i].Type,
					(long unsigned int)nodes[i].ID);
			printf("Serial Number and Description ----------\n");
			printf("\tSerialNumber=%s\n", nodes[i].SerialNumber);
			printf("\tDescription=%s\n", nodes[i].Description);
			printf("------------------------------------------\n");
		}
		//printf("End Device Information --------------\n");
		//printf("Output Device Information for First Device -----------\n");
		serialNum = nodes[0].SerialNumber;
		//printf("serial number = %s \n", serialNum.c_str());
	} else {
		serialNum = "No Device Found\n";
	}
	return serialNum;
}


int main(int argc, char *argv[])
{
	std::string devnum;
	FT_HANDLE handle;
	FILE *fd = NULL;
	
	devnum = get_device_list();
	printf(" First Device Serial Number = %s \n", devnum.c_str());
	return 0;

}
	