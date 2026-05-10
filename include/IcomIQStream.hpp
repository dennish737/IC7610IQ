/********************************
 * IcomIQStream.hpp
 * This file defines the streaming class for the Icom 7610 IQ port. 
 * The FTDI D3xx devices driver provides a streaming capability, using the FT_ReadPipeEx(), Window Overlapped and FT_GetOverlappedResults(), to provide high 
 * speed data streaming. Once the data is captured from the IC7610, we need to save or process the data, by writting it to a file of sending it to a dataport.
 * We also need to convert the data format from native to a desired format consumed by other aplication, such as GnuRadio, FFT, and Water Fall applictions.
 * 
 * The Icom 7610 IO native output is an interleaved signed 16 bit I/Q, where the first 16 bits is the real part (I), andthe next 16 bit s is the imaginary (Q)
 * part ofthe signal. In c++ this can be interperted as two (2) shorts per sample, or one complex<short> per sample. The problem of using native format, is
 * that integer math lacks the precison for standard FFT Algorithms.
 *  
 * Traditinal FFT algorithms use normalized (values between -1.0 and 1.0) "double" (8 bytes/ 16 for complex) or "float" (4 bytes/ 8 bytes for complex). When
 * using normalized "short"  values, these decvimals roundto 0 or one, distroying the data.
 * 
 * Stratigy:
 *  For the native format, the buffer used to acquire the data can be the same buffer used for procesing the data. Doing so reduce the number of copy steps.
 * Thus we acquire the buffer at the start of the read, and release the buffer after the completion of the write. To maximize preformance we want to align
 * the buffers to sysdtem page boundaries.
 * 
 * However; if a non native format is used, say complex<real> or complex<double>, we need to transform the input data, to the correct format
 * for tha output. This will require a "copy" and transform from one buffer to another. The enumerated type determins the output type, with IC7610_SHORT
 * and IC7610_COMPLEX_SHORT being the native format. The IC7610_COMPLEX_FLOAT and IC7610_COMPLEX_DOUBLE will require seperate input and output buffers.
 * By default, the native mode is used
 ********************************/

#ifndef IcomIQPort_STREAM_HPP
#define IcomIQPortI_STREAM_HPP

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/windows/object_handle.hpp>
#include <boost/bind/bind.hpp>

// logging functions
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>


#include "BufferPool.hpp"
#include "IcomIqPort.hpp"
#include "ftd3xx.h"

#ifndef WINSOCK_API
    #include <windows.h>
#endif

using boost::asio::windows::object_handle;

namespace asio = boost::asio;

typedef std::complex<short> RawComplex;
typedef std::complex<float> NormFloatComplex;
typedef std::complex<double> NormDoubleComplex;
const uint8_t mainVFO = 0x00;
const uint8_t subVFO = 0x01;

#define NUM_STREAM_BUFFERS 16
// Number of sampeles = 16384
// Sample Size = 4
// Buffer Size = num_samples * sample_size = 65536
#define STREAM_BUFFER_SIZE  65536
#define SYS_PAGE_SIZE  4096

enum class IC7610FormatTypes : int{
	IC7610_INVALID = -1,
	IC7610_ISHORT = 0,           //Interleave Short is the native mode
	IC7610_COMPLEX_FLOAT = 1,   // 32 bit float for real and complex part. 8 bytes total
	IC7610_COMPLEX_DOUBLE = 2 // 64 bit double for real and complex part, 16 bytes total
};

enum class WriteMode {
    toDisk = 0,
    toTCP = 1
};

struct ASYNC_CONTEXT : OVERLAPPED {
	UINT id;
	HANDLE ftHandle;
    Buffer* buffer;          // Data buffer for this request
    DWORD byteCount;
	void (*on_complete)(const boost::system::error_code&, size_t, Buffer*, std::chrono::steady_clock::time_point);

    ASYNC_CONTEXT() {                           // HANDLE devHandle
        memset(this, 0, sizeof(OVERLAPPED));
		//FT_InitializeOverlapped(devHandle, this);
        //hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // Manual reset
        hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        //event_monitor.assign(overlapped.hEvent);
		//ftHandle = devHandle;
    };
    ~ASYNC_CONTEXT() { CloseHandle(hEvent); }
};

class IcomIQPortStream : public IcomIQPort {
    asio::io_context& io_ctx;
    asio::stream_file file;
    size_t total_read = 0;
    size_t total_write = 0;
    WriteMode mode = WriteMode::toDisk;
    IC7610FormatTypes outType = IC7610FormatTypes::IC7610_ISHORT;
    int numBuffersRequired = 1;
    // Buffers for ping-pong or concurrent processing
    BufferPool* poolPtr;                            // input buffer pool 
    //BufferPool* writePoolPtr;                           // output buffer pool - require if outout is not native format 

    ASYNC_CONTEXT  overlapped;
    object_handle event_monitor;
    uint8_t vfo_ = mainVFO;

    std::shared_ptr<spdlog::logger> _logger;

public:
    IcomIQPortStream(asio::io_context& ctx, std::string deviceSerialNum, const std::string& path, const char* out_type = "S16")
        : io_ctx(ctx),  
          file(ctx, path, asio::stream_file::write_only | asio::stream_file::create),
          event_monitor(ctx) 
    {
        //_logger = spdlog::get("main_logger");
        _logger = spdlog::default_logger();

        if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "Icom7610 stream initializing");}
        if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "file paath: {}", path);}
        // set the output type
		if (strcmp(out_type,"S16") == 0) {
			outType = IC7610FormatTypes::IC7610_ISHORT;
            numBuffersRequired = 1;
		} else if (strcmp(out_type, "CS16") == 0){
			outType = IC7610FormatTypes::IC7610_ISHORT;
            numBuffersRequired = 1;
		} else if (strcmp(out_type, "CF32") == 0) {
			outType = IC7610FormatTypes::IC7610_COMPLEX_FLOAT ;
            numBuffersRequired = 2;
		} else if (strcmp(out_type, "CF64") == 0) {
			outType = IC7610FormatTypes::IC7610_COMPLEX_DOUBLE ;
            numBuffersRequired = 4;
		} else {
            outType = IC7610FormatTypes::IC7610_INVALID; // generate no output
            if( _logger) {SPDLOG_LOGGER_ERROR(_logger, "Invalid Output Type: {}",4);}
            exit(4);           
        }    
        if(_logger) {SPDLOG_LOGGER_DEBUG(_logger,"Output Type = {}, Number of buffers required = {}", out_type, numBuffersRequired );}
        init(deviceSerialNum);
        if (isOpen()) {
            // we have successfullly open the FTDI d3xx port
            overlapped.ftHandle = ftHandle;
            // Input is always Interleaved complex
            poolPtr = new BufferPool(NUM_STREAM_BUFFERS, (STREAM_BUFFER_SIZE * 4));
            event_monitor.assign(overlapped.hEvent);
            if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "Icom7610 Stream initialized");}
        } else {
            if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "Icom7610 Stream initialization Failed - Aporting");}
            exit(2);
        }
    }

    ~IcomIQPortStream() {
        if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "Terminating DiskWritter");}
        if (overlapped.hEvent) CloseHandle(overlapped.hEvent);
    }

    std::shared_ptr<IcomIQPortStream> shared_derived() {        
        return std::static_pointer_cast<IcomIQPortStream>(shared_from_this());
    }

   void setMode(WriteMode wmode ) {mode = wmode;}

    void setVFO(uint8_t vfo) {vfo_ = vfo; }
    uint8_t getVFO() { return vfo_; }

    size_t getReadTotal() { return total_read;}
    size_t getWriteTotal() {return total_write;}

    void start(uint8_t vfo) {
        //auto self = std::static_pointer_cast<IcomIQPortStream>(shared_from_this());
        vfo_ = vfo;
        if(_logger) {_logger->info(" starting read. vfo : {}",((vfo == mainVFO)? "mainVFO" : "subVFO"));}
        if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "Starting Reads, enabeling vfo {}", ((vfo == mainVFO)? "Main VFO" : "SubVFO"));}
        //overlapped.on_complete = &(this->handle_write_complete_for_disk);
        initiate_read();
    }

 void processAndWriteCS16(             // native mode
        const Buffer* currentBuffer, ULONG transferred) {

        auto diskStart = std::chrono::steady_clock::now();
        

        asio::async_write(file, 
                    asio::buffer(currentBuffer->bufferPointer, transferred),
                    [this, currentBuffer, diskStart](const boost::system::error_code& ec, std::size_t bytes) {
                        handle_write_CS16_complete_for_disk(ec, bytes, currentBuffer, diskStart);
                    } );

                    /*
                        boost::bind(&handle_write_CS16_complete_for_disk,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            currentBuffer, diskStart));
                    */
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - diskStart);
        if( _logger) {
            SPDLOG_LOGGER_DEBUG(_logger, "Read Complete: {}, microseconds. Bytes Read = {}", duration.count(), transferred);
        }

}   



 void processAndWriteCF32( 
                     Buffer* currentBuffer, ULONG transfered) {
    // start the clock
    auto diskStart = std::chrono::steady_clock::now();
    // transdered represents the number of bytes transfered in native format (f bytes per sample). To store to Complex<float> we need 8 bytes per sample
    // 1. convert the number of bytes to the number of samples
    size_t num_samples = transfered/sizeof(std::complex<short>);

    // 2. make sure we have an even number of values 
    if ((num_samples % 2) != 0)
    {
        // warn the user thathe number of values is not n even number there will be data loss
        if( _logger) {
            SPDLOG_LOGGER_DEBUG(_logger, "Number of samples is not even = {}, possible data loss",num_samples);
        }
    }
    size_t num_values = num_samples/2;
    
   // 3. Acquire two buffers
    Buffer* raw1 = poolPtr->acquire();
    Buffer* raw2 = poolPtr->acquire();

    // Reinterpret buffers for filling
    NormFloatComplex* out1 = reinterpret_cast<NormFloatComplex*>(raw1->bufferPointer);
    NormFloatComplex* out2 = reinterpret_cast<NormFloatComplex*>(raw2->bufferPointer);

    RawComplex* input_data = reinterpret_cast<RawComplex*>(currentBuffer->bufferPointer);
    // 4. Fill buffers with normalized complex floats
    // Assuming input_data has  NUM_VALUES/2

    for (size_t i = 0; i < num_values; ++i) {
        out1[i] = NormFloatComplex(
            (float)input_data[i].real() / 32767.0f,
            (float)input_data[i].imag() / 32767.0f
        );
        out2[i] = NormFloatComplex(
            (float)input_data[i + num_values].real() / 32767.0f,
            (float)input_data[i + num_values].imag() / 32767.0f
        );
    }

    // 5. Combine buffers for scatter-gather write
     // Combine buffers for scatter-gather write
    std::vector<boost::asio::const_buffer> buffers = {
        boost::asio::buffer(raw1->bufferPointer, raw1->size),
        boost::asio::buffer(raw1->bufferPointer, raw2->size)
    };   

    // 6. release the current buffer
    poolPtr->release(currentBuffer);

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - diskStart);
        if( _logger) {
            SPDLOG_LOGGER_DEBUG(_logger, "Processing Complete: {}, microseconds. Bytes processed = {}", duration.count(), (num_samples*4) );
        }
    // write to file
    asio::async_write(file, buffers,
                    [this, raw1, raw2, diskStart](const boost::system::error_code& ec, std::size_t bytes) {
                        handle_write_CF32_complete_for_disk(ec, bytes, raw1,raw2, diskStart);
                    } );


}   



private:

    void initiate_read() {

        ULONG bytesRead = 0;
        //consoleLog->info("starting stream");
        if (!isIQDataEnabled()){
            if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "Enabeling Data");}
            enableIQData(vfo_);
        }
        overlapped.buffer = poolPtr->acquire();
        // FT_ReadPipeEx for Pipe 0x82 (standard bulk IN)
        if( _logger) { 
            SPDLOG_LOGGER_DEBUG(_logger, "Starting ReadPipe num_bytes = {}",  STREAM_BUFFER_SIZE); 
            if (overlapped.buffer == nullptr) { SPDLOG_LOGGER_DEBUG(_logger, "overlapped.buffer is null"); }
            if (overlapped.buffer->bufferPointer == nullptr) { SPDLOG_LOGGER_DEBUG(_logger, "Soverlapped.buffer->bufferPointer is null");}
        }

        FT_STATUS status = FT_ReadPipeEx(ftHandle, IQ_IN, overlapped.buffer->bufferPointer, 
                                        (size_t) STREAM_BUFFER_SIZE, &bytesRead, &overlapped);
        
        if( _logger) { SPDLOG_LOGGER_DEBUG(_logger, "Pipe Status {}",  status); }
        if (status == FT_IO_PENDING || status == FT_OK) {
            // Wait for the Windows event to signal via Boost
            event_monitor.async_wait([self = shared_derived()  ](boost::system::error_code ec) {
                if (!ec) self->handle_read_complete();
                else std::cerr << "Port closed or read error: " << ec.message() << std::endl;
            });
        } else {
            std::cerr << "FTDI Read failed, status: " << status << std::endl;
        }
    }

    void handle_read_complete() {
        ULONG transferred = 0;
        auto start = std::chrono::steady_clock::now();
        if( _logger) { SPDLOG_LOGGER_DEBUG(_logger, "Starting handle_read_complete"); }
        if (FT_GetOverlappedResult(ftHandle, &overlapped, &transferred, FALSE) == FT_OK) {
            // Write to disk using Boost's async_write
            Buffer* currentBuffer = overlapped.buffer;

            if (outType == IC7610FormatTypes::IC7610_ISHORT)
            {
                processAndWriteCS16( currentBuffer, transferred  );
            } else if (outType == IC7610FormatTypes::IC7610_COMPLEX_FLOAT)
            {
                processAndWriteCF32(currentBuffer, transferred);
            }

            total_read += transferred; 
            
            initiate_read();
        }
    }
    

   void handle_write_CS16_complete_for_disk(const boost::system::error_code& ec,
                    std::size_t bytes_transferred,
                    const Buffer* inBuffer , std::chrono::steady_clock::time_point start_time )
    {
        Buffer* currentBuffer = const_cast<Buffer*>(inBuffer);
        //auto end_time = std::chrono::steady_clock::now();
        //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);       
        if (!ec) {
            total_write += bytes_transferred;
            if (_logger) {
               // SPDLOG_LOGGER_DEBUG(_logger, "Write Complete: {}, microseconds. Bytes Read = {}", duration.count(), bytes_transferred);
                SPDLOG_LOGGER_DEBUG(_logger, "Write Complete:  Bytes Read = {}",  bytes_transferred);
            }
        } else {
            std::cerr << "Error: " << ec.message() << "\n";
        }
        poolPtr->release(currentBuffer);
    }

    void handle_write_CF32_complete_for_disk(const boost::system::error_code& ec,
                                std::size_t bytes_transferred,
                                const Buffer* buf1, const Buffer* buf2, 
                                std::chrono::steady_clock::time_point start_time)
    {
        Buffer* bufa = const_cast<Buffer*>(buf1);
        Buffer* bufb = const_cast<Buffer*>(buf2);
        if (ec) std::cerr << "Write error: " << ec.message() << std::endl;
        poolPtr->release(bufa);
        poolPtr->release(bufb); 
    }

};


#endif // FTDI_READ_STREAM_HPP
