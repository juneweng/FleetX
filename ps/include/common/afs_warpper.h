#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include "ps_string.h"
#include "proto/ps.pb.h"

namespace paddle {
namespace ps {
struct FsDataConverter {
    std::string converter;
    std::string deconverter;
};

struct FsChannelConfig {
    std::string path;         // path of file
    std::string converter;    // data converter
    std::string deconverter;
};

class FsReadChannel {
public:
    FsReadChannel() : _buffer_size(0) {}
    FsReadChannel(uint32_t buffer_size) : _buffer_size(buffer_size) {}
    virtual ~FsReadChannel() {}
    FsReadChannel(FsReadChannel&&) = delete;
    FsReadChannel(const FsReadChannel&) = delete;
    int open(std::shared_ptr<FILE> fp, const FsChannelConfig& config) {
        _file = fp;
        return 0;
    }
    inline int close() {
        _file.reset();
        return 0;
    }

    inline uint32_t read_line(std::string& line_data) {
        line_data.clear();
        char buffer = '\0';
        size_t read_count = 0;
        while (1 == fread(&buffer, 1, 1, _file.get()) && buffer != '\n') {
            ++read_count;
            line_data.append(&buffer, 1);
        }
        if (read_count == 0 && buffer != '\n') {
            return -1;
        }
        return 0;
    }
private:
    uint32_t _buffer_size;
    FsChannelConfig _config;
    std::shared_ptr<FILE> _file;
};
class FsWriteChannel {
public:
    FsWriteChannel() : _buffer_size(0) {}
    FsWriteChannel(uint32_t buffer_size) : _buffer_size(buffer_size) {}
    virtual ~FsWriteChannel() {}
    FsWriteChannel(FsWriteChannel&&) = delete;
    FsWriteChannel(const FsWriteChannel&) = delete;
    
    int open(std::shared_ptr<FILE> fp, const FsChannelConfig& config) {
        _file = fp;
        if (_buffer_size != 0) {
            _buffer = std::shared_ptr<char>(new char[_buffer_size]);

            CHECK(0 == setvbuf(&*_file, _buffer.get(), _IOFBF, _buffer_size));
        }
        return 0;
    }
    
    inline void flush() {
        //TODO 
        return;
    }
    
    inline int close() {
        flush();
        _file.reset();
        return 0;
    }

    inline uint32_t write_line(const char* data, uint32_t size) {
        size_t write_count = fwrite_unlocked(data, 1, size, _file.get());
        if (write_count != size) {
            return -1;
        }
        write_count = fwrite_unlocked("\n", 1, 1, _file.get());
        if (write_count != 1) {
            return -1;
        }
        return 0;
    }
    inline uint32_t write_line(const std::string& data) {
        return write_line(data.c_str(), data.size());
    }

private:
    uint32_t _buffer_size;
    FsChannelConfig _config;
    std::shared_ptr<FILE> _file;
    std::shared_ptr<char> _buffer;
};

class AfsClient {
public:
    AfsClient() {};
    virtual ~AfsClient() {};
    AfsClient(AfsClient&&) = delete;
    AfsClient(const AfsClient&) = delete;

    int initialize(const FsClientParameter& fs_client_param);
    int initialize(const std::string& hadoop_bin,
                   const std::string& uri,
                   const std::string& user,
                   const std::string& passwd,
                   int buffer_size_param = (1L<<25));
    int initialize(const std::string& hadoop_bin,
                   const std::string& uri,
                   const std::string& ugi,
                   int buffer_size_param = (1L<<25));

    //open file in 'w' or 'r'
    std::shared_ptr<FsReadChannel> open_r(const FsChannelConfig& config, uint32_t buffer_size = 0); 
    std::shared_ptr<FsWriteChannel> open_w(const FsChannelConfig& config, uint32_t buffer_size = 0); 

    //remove file in path, path maybe a reg, such as 'part-000-*'
    void remove(const std::string& path);
    void remove_dir(const std::string& dir);

    //list files in path, path maybe a dir with reg
    std::vector<std::string> list(const std::string& path);

    //exist or not
    bool exist(const std::string& dir);    
};

}
}