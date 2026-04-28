#ifndef BUFFER_MANAGER
#define BUFFER_MANAGER

#include <vector>
#include <memory>
#include "common.h"

/*

    The data comming from the IC7610  are interleaved I/Q short values , where 'I' is real part,
    and 'Q' is the complex part. When reading data, from the IC7610, each sample is 4 bytes (2 short values).
    Thus the best native formatate is interleaved short, or complex<short>. Because of the device used, we must read the 
    data in 'blocks', and blocks must be multiples of 512 bytes (256 short or 128 complex <short> values). It is also 
    recomended that the minimum block size is 4096 bytes or 1024 complex<short> (2048 short) samples. 
    
    From the IC7610 specifications,
    the sample rate is 1.92MHz or ~52.1 nSec per sample. Assuming a 4 bytes for each sample and using a 4096 
    byte buffer, a block of data would be available every ~533 uSec. The table below shows the approximate
    time required to collect n samples in milli-Seconds (mSec)
	
	Block capacty and collection time:
	   Num Samples     	Size in Bytes	Num 4K Blocks   Block Time
			128					512
			256					1024
			512					2048
			1024				4096			1               0.522 mSec
			2048				8192			2               1.665 mSec
			4096				16384			4               2.133 mSec
			8192				32768			8               4.267 mSec
			16384				65526			16              8.533 mSec
			32768				131072			32             17.067 mSec
            65536               262144          64             34.133 mSec
	
    Based on this data, a minimum block size for reads should be minimum of 4096 samples (16384 bytes) for a BLOCK_SIZE. 
    To maximize preformance and minimize 'copy time' we should aline our buffer on page boundaries (think DMA copy) and 
    set their size to a multiple of page size. The pagesize for x64 machines is typically 4096 bytes.

    Finally we need to set the number of buffers needed.

*/

#define BYTES_PER_SAMPLE 4
#define PAGE_SIZE 4096
#define MAX_SAMPLES 65536
// BUF_LEN = MAX_SAMPLES * BYTES_PER_SAMPLE
#define BUF_LEN 262144
#define NUM_BUF 15
 


struct BufferManager {
    public:
        BufferManager(): isInitialized(false), num_buffers(NUM_BUF), buffer_len(BUF_LEN), 
                         buf_head(0), buf_tail(0), buf_count(),
                         remainderHandle(-1), remainderSamps(0), remainderOffset(0), remainderBuff(nullptr) 
                         {}
        ~BufferManager() {clearBuffers(); }

        void clearBuffers()
        {
            for (short* p : arrayOfShortArrays) {
                delete[] p;
            }
            arrayOfShortArrays.clear();
            buf_count = 0;
            buf_tail = 0;
            buf_head = 0;
            remainderSamps = 0;
            remainderOffset = 0;
            remainderBuff = nullptr;
            remainderHandle = -1;
        }

        void allocateBuffers()
        {
            for ( size_t i = 0; i < num_buffers; ++i) {
                // allocate the array and initialize to zero
                short* newArray = new short[buffer_len]();
                
                // add the pointer to the vector
                arrayOfShortArrays.push_back(newArray);
            }
        }


    private:
        bool isInitialized = false;
        size_t num_buffers;
        size_t buffer_len;
        std::vector<short*> arrayOfShortArrays;

        uint32_t buf_head;
        uint32_t buf_tail;
        uint32_t buf_count;

        int32_t remainderHandle;
        size_t remainderSamps;
        size_t remainderOffset;
        short* remainderBuff;


        

}
#endif /*BufferMGR.hpp */