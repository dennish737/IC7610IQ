/******************************************
 *
 * CIV Protocol constants for IC7610
 * CIVProtocol.hpp
 *****************************************/

#ifndef COMMON
#define COMMON
// Common constant
#include <cstdint>


// Icom CI-V header and acknowledge values commands
constexpr uint8_t CIV_PREAMBLE = 0xFE;	// Start Of Message
constexpr uint8_t CIV_EOM = 0xFD;		// End Of Message
constexpr uint8_t CIV_ACK = 0xFB; 		// Acknowledge response
constexpr uint8_t CIV_NAK = 0xFA; 		// Not acknowledge response

// Endpoints
constexpr uint8_t RADIO_ADDR = 0x98; 	// Default IC-7610 CIV address
constexpr uint8_t PC_ADDR = 0xE0;   	// PC  address (controller)
constexpr uint8_t IQ_IN = 0x84;			// IQ data pipe 84
constexpr uint8_t CMD_IN = 0x82;		// Control Command Address 7610->PC (in)
constexpr uint8_t CMD_OUT = 0x02;		// Control Command PC -> IC7610 iq

// Each sample consist of a complex Short I and Q sample (2bytes + 2 bytes = 4 bytes)
constexpr size_t BYTES_PER_SAMPLE = 4;
constexpr size_t SAMPLES_PER_BLOCK = 1024;
// use 4K for IQ buffer - 1024 IQ samples
constexpr size_t IQ_BLOCK_LENGTH = 4096;

constexpr size_t NUM_IQ_BLOCKS = 16;
constexpr size_t NUM_BLOCKS = 16;

// A BlockingRing buffer is used to pass data between the 'device' and Soapy stream. 
// to calculate the ring size, we need to know the sample size in bytes, the number of samples per block (for read)
// and the number of blocks
constexpr size_t RING_BYTE_SIZE = BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK * NUM_IQ_BLOCKS;
constexpr size_t RING_SAMPLE_SIZE = SAMPLES_PER_BLOCK * NUM_IQ_BLOCKS;
// protocal position indexes
#define CMD_INDEX 4
#define SUBCMD_INDEX 5
#define DATA_INDEX 6

//#define TIMEOUT 100 // ms
#define CMD_TIMEOUT 5000// ms
#define DATA_TIMEOUT 5000 // ms

#define ASYNC_BUFFERS 0


#define CIV_ACK_NAK_OFFSET 4
#define IQ_ACK_NAK_MSG_LEN 8

const int defaultBaudrate = 115200;
const int CIV_READ_BUFFER_SIZE = 128;

#endif