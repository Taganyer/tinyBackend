//
// Created by taganyer on 24-10-4.
//

#ifndef BASE_DIOFILE_HPP
#define BASE_DIOFILE_HPP

#ifdef BASE_DIOFILE_HPP


#include <cstdio>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "config.hpp"
#include "NoCopy.hpp"

namespace Base {

    class DioFile : NoCopy {
    public:
        DioFile() = default;

        explicit DioFile(const char* path);

        DioFile(DioFile &&other) noexcept;

        ~DioFile();

        bool open(const char* path);

        bool close();

        uint64 read(uint64 size, void* dest) const;

        uint64 write(const void* str, size_t len);

        void flush_to_disk();

        bool resize_file(uint64 size);

        bool delete_file();

        int64 seek_cur(int64 step) { return lseek(_fd, step, SEEK_CUR); };

        int64 seek_beg(int64 step) { return lseek(_fd, step, SEEK_SET); };

        int64 seek_end(int64 step) { return lseek(_fd, step, SEEK_END); };

        operator bool() const { return is_open(); };

        [[nodiscard]] uint64 pos() const { return lseek(_fd, 0, SEEK_CUR); };

        [[nodiscard]] bool is_open() const { return _fd > 0; };

        [[nodiscard]] const std::string& get_path() const { return _path; };

        [[nodiscard]] bool get_stat(struct stat* ptr) const;

        [[nodiscard]] int get_fd() const {
            return _fd;
        };

    protected:
        int _fd = -1;

        std::string _path;

    };

    inline DioFile::DioFile(const char* path) {
        open(path);
    }

    inline DioFile::DioFile(DioFile &&other) noexcept : _fd(other._fd), _path(std::move(other._path)) {
        other._fd = -1;
        other._path.clear();
    }

    inline DioFile::~DioFile() {
        close();
    }

    inline bool DioFile::open(const char* path) {
        close();
        _fd = ::open(path, O_RDWR | O_CREAT | O_DIRECT, 0644);
        if (_fd < 0) return false;
        _path = path;
        return true;
    }

    inline bool DioFile::close() {
        if (_fd < 0) return true;
        if (::close(_fd) == 0) {
            _fd = -1;
            _path = std::string();
            return true;
        }
        return false;
    }

    inline uint64 DioFile::read(uint64 size, void* dest) const {
        int64 len = ::read(_fd, dest, size);
        return len > 0 ? len : 0;
    }

    inline uint64 DioFile::write(const void* str, size_t len) {
        int64 size = ::write(_fd, str, len);
        return size > 0 ? size : 0;
    }

    inline void DioFile::flush_to_disk() {
        fsync(_fd);
    }

    inline bool DioFile::resize_file(uint64 size) {
        return ftruncate(_fd, size) == 0;
    }

    inline bool DioFile::delete_file() {
        if (_fd < 0 || ::close(_fd) != 0) return false;
        _fd = -1;
        bool success = remove(_path.c_str()) == 0;
        _path = std::string();
        return success;
    }

    inline bool DioFile::get_stat(struct stat* ptr) const {
        return is_open() && fstat(_fd, ptr) == 0;
    }

}

#endif

#endif //DIOFILE_HPP
