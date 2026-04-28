

#ifndef IcomIQPort_STREAM_HPP
#define IcomIQPortI_STREAM_HPP

#include <iostream>
#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/windows/object_handle.hpp>


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

const uint8_t mainVFO = 0x00;
const uint8_t subVFO = 0x01;

#define NUM_STREAM_BUFFERS 8
#define STREAM_BUFFER_SIZE  131072
#define SYS_PAGE_SIZE  4096

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
    // Buffers for ping-pong or concurrent processing
    BufferPool* poolPtr;
    static const size_t BLOCK_SIZE = STREAM_BUFFER_SIZE; 

    ASYNC_CONTEXT  overlapped;
    object_handle event_monitor;
    uint8_t vfo_ = mainVFO;

    std::shared_ptr<spdlog::logger> _logger;

public:
    IcomIQPortStream(asio::io_context& ctx, std::string deviceSerialNum, const std::string& path)
        : io_ctx(ctx),  
          file(ctx, path, asio::stream_file::write_only | asio::stream_file::create),
          event_monitor(ctx) 
    {
        _logger = spdlog::get("main_logger");
        if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "DiskWritter initializing");}
        init(deviceSerialNum);
        if (isOpen()) {
            overlapped.ftHandle = ftHandle;
            poolPtr = new BufferPool(NUM_STREAM_BUFFERS, STREAM_BUFFER_SIZE);
            //read_buffer.resize(BLOCK_SIZE);
            //memset(&overlapped, 0, sizeof(OVERLAPPED));
            //overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            event_monitor.assign(overlapped.hEvent);
            if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "DiskWritter initialized");}
        } else {
            if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "DiskWritter initialization Failed - Aporting");}
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
        if( _logger) {SPDLOG_LOGGER_DEBUG(_logger, "Starting Reads, enabeling vfo {}", ((vfo == mainVFO)? "Main VFO" : "SubVFO"));}
        //overlapped.on_complete = &(this->handle_write_complete_for_disk);
        initiate_read();
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
        FT_STATUS status = FT_ReadPipeEx(ftHandle, IQ_IN, overlapped.buffer->bufferPointer, 
                                        BLOCK_SIZE, &bytesRead, &overlapped);

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
        if (FT_GetOverlappedResult(ftHandle, &overlapped, &transferred, FALSE) == FT_OK) {
            // Write to disk using Boost's async_write
            Buffer* currentBuffer = overlapped.buffer;
            auto diskStart = std::chrono::steady_clock::now();
            total_read += transferred; 
            if (mode == WriteMode::toDisk){
                asio::async_write(file, 
                        asio::buffer(currentBuffer->bufferPointer, transferred),
                        [self = shared_derived(), transferred, currentBuffer, diskStart] (boost::system::error_code ec, size_t) {
                            self->handle_write_complete_for_disk(ec, transferred, currentBuffer, diskStart);
                        }
                    );
            } else {
                if( _logger) {
                     SPDLOG_LOGGER_DEBUG(_logger,"Invalide writemode:");
                }
                exit(3);
            }
            
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            if( _logger) {
                SPDLOG_LOGGER_DEBUG(_logger, "Read Complete: {}, microseconds. Bytes Read = {}", duration.count(), transferred);
            }

            initiate_read();
        }
    }
    

   void handle_write_complete_for_disk(const boost::system::error_code& ec,
                    std::size_t bytes_transferred,
                    Buffer* currentBuffer,
                    std::chrono::steady_clock::time_point start_time )
    {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);       
        if (!ec) {
            total_write += bytes_transferred;
            if (_logger) {
                SPDLOG_LOGGER_DEBUG(_logger, "Write Complete: {}, microseconds. Bytes Read = {}", duration.count(), bytes_transferred);
            }
        } else {
            std::cerr << "Error: " << ec.message() << "\n";
        }
        poolPtr->release(currentBuffer);
    }

};


#endif // FTDI_READ_STREAM_HPP
