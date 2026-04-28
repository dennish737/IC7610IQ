// The Buffer Pool Implementation
#ifndef BUFFER_POOL_HPP
#define BUFFER_POOL_HPP

#include <vector>
#include <mutex>
#include <stack>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h" // For color console
#include <memory>

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
	   Num Samples     	Size in Bytes	Num Pages   Block Time
			128				512
			256				1024
			512				2048
			1024			4096			1               ~2.133 mSec
			2048			8192			2               ~4.267 mSec
			4096			16384			4               ~8.533 mSec
			8192			32768			8              ~17.067 mSec
			16384			65536			16             ~34.133 mSec
			32768			131072			32             ~68.267 mSec
            65536           262144          64            ~136.533 mSec
	
    For maximum preformance and to minimize 'copy time' we should aline our buffer on page boundaries (think DMA copy) and 
    set their size to a multiple of page size. For linux, Windows and Mac, Intel or ARM systems, the page boundary is 4096. 
    
    Based on this data, a minimum block size for reads should be four pages, which is 4096 samples or 16384 bytes (4 * PAGE_SIZE). 

*/

#define NUM_BUFFERS 8
#define BUFFER_SIZE 16384
#define SYS_PAGE_SIZE 4096

struct Buffer {
public:
    uint8_t* bufferPointer;
    uint8_t bufferId;
    ULONG totalUsed;
    size_t size;
};

class BufferPool {
private:
    std::stack<Buffer*> freeBuffers;
    std::vector<Buffer*> bufferList; // used to prevent memory leaks
    std::mutex mutex;
    size_t bufferSize;
    std::shared_ptr<spdlog::logger> _logger;

public:
    BufferPool(size_t count, size_t size) : bufferSize(size) {
        _logger = spdlog::get("main_logger");
        if (_logger){
            SPDLOG_LOGGER_DEBUG(_logger, "Initializing BufferPool Class: num buffer = {}, buffer size = {}", count, size);
        }
        for (size_t i = 0; i < count; ++i) {
            Buffer* bufPtr = new Buffer();
            bufPtr->bufferId = i;
            bufPtr->totalUsed = 0;
            bufPtr-> size = size;
            // Use _aligned_malloc for better performance with USB 3.0 DMA
            bufPtr->bufferPointer = (uint8_t*)_aligned_malloc(size, SYS_PAGE_SIZE);
            freeBuffers.push(bufPtr);
            bufferList.push_back(bufPtr);
        }
    }

    Buffer* acquire() {
        std::lock_guard<std::mutex> lock(mutex);
        if (freeBuffers.empty()) {
            if(_logger) {SPDLOG_LOGGER_DEBUG(_logger, "Pool exusted returning null"); }
            return nullptr; // Pool exhausted
        }
        Buffer* buf = freeBuffers.top();
        buf->totalUsed = 0;
        SPDLOG_LOGGER_DEBUG(_logger, "Buffer {} acquired", buf->bufferId);
        freeBuffers.pop();
        return buf;
    }

    void release(Buffer* buf) {
        std::lock_guard<std::mutex> lock(mutex);
        buf->totalUsed = 0;
        freeBuffers.push(buf);
        if (_logger) { SPDLOG_LOGGER_DEBUG(_logger, "Buffer {} released", buf->bufferId);}
    }

    ~BufferPool() {
        if (_logger) { SPDLOG_LOGGER_DEBUG(_logger, "BufferPool closing");}
        while (!freeBuffers.empty()) {
            Buffer* bufPtr = freeBuffers.top();
            auto it = std::find(bufferList.begin(), bufferList.end(), bufPtr);
            if (it != bufferList.end()) {
                bufferList.erase(it);
            }
            _aligned_free(bufPtr->bufferPointer);
            freeBuffers.pop();
        }
        // clean up any buffers that were acquired but not released
        if (!bufferList.empty() ) {
            for (Buffer* ptr : bufferList) {   
                   _aligned_free(ptr->bufferPointer) ;    
            }
        bufferList.clear();
    }

    }
};

#endif