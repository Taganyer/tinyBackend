//
// Created by taganyer on 4/4/24.
//

#ifndef BASE_IOFILE_HPP
#define BASE_IOFILE_HPP


#include <cstdio>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include "config.hpp"
#include "NoCopy.hpp"

namespace Base {

    class ioFile : NoCopy {
    public:
        ioFile() = default;

        ioFile(const char* path, bool append, bool binary);

        ioFile(ioFile &&other) noexcept;

        ~ioFile();

        bool open(const char* path, bool append, bool binary);

        bool close();

        uint64 read(uint64 size, void* dest) const;

        size_t write(const void* str, size_t len = -1);

        void flush();

        void flush_to_disk();

        bool resize_file(uint64 size);

        bool delete_file();

        bool seek_cur(int64 step) { return fseek(_file, step, SEEK_CUR) == 0; };

        bool seek_beg(int64 step) { return fseek(_file, step, SEEK_SET) == 0; };

        bool seek_end(int64 step) { return fseek(_file, step, SEEK_END) == 0; };

        operator bool() const { return is_open() && !feof(_file); };

        [[nodiscard]] uint64 pos() const { return ftell(_file); };

        [[nodiscard]] bool is_open() const { return _file; };

        [[nodiscard]] bool is_end() const { return feof(_file); };

        [[nodiscard]] const std::string& get_path() const { return _path; };

        [[nodiscard]] bool get_stat(struct stat* ptr) const;

        [[nodiscard]] int get_fd() const {
            if (!_file) return -1;
            return fileno(_file);
        };

        [[nodiscard]] FILE* get_fp() const { return _file; };

    protected:
        FILE* _file = nullptr;

        std::string _path;

    };

    inline ioFile::ioFile(const char* path, bool append, bool binary) {
        open(path, append, binary);
    }

    inline ioFile::ioFile(ioFile &&other) noexcept : _file(other._file) {
        other._file = nullptr;
    }

    inline ioFile::~ioFile() {
        close();
    }

    inline bool ioFile::open(const char* path, bool append, bool binary) {
        close();
        char mod[4] { 'w', 'b', '+', '\0' };
        if (append) mod[0] = 'r';
        if (!binary) {
            mod[1] = '+';
            mod[2] = '\0';
        }
        _file = fopen(path, mod);
        if (append && !_file) {
            mod[0] = 'w';
            _file = fopen(path, mod);
        }
        if (_file)
            _path = path;
        return _file;
    }

    inline bool ioFile::close() {
        if (!_file) return true;
        if (fclose(_file) == 0) {
            _file = nullptr;
            _path = std::string();
            return true;
        }
        return false;
    }

    inline uint64 ioFile::read(uint64 size, void* dest) const {
        return fread(dest, 1, size, _file);
    }

    inline size_t ioFile::write(const void* str, size_t len) {
        if (len == -1) return fputs((const char *) str, _file);
        return fwrite(str, 1, len, _file);
    }

    inline void ioFile::flush() {
        fflush(_file);
    }

    inline void ioFile::flush_to_disk() {
        fflush(_file);
        fsync(get_fd());
    }

    inline bool ioFile::resize_file(uint64 size) {
        return ftruncate(get_fd(), size) == 0;
    }

    inline bool ioFile::delete_file() {
        if (!_file || fclose(_file) != 0) return false;
        _file = nullptr;
        bool success = remove(_path.c_str()) == 0;
        _path = std::string();
        return success;
    }

    inline bool ioFile::get_stat(struct stat* ptr) const {
        return is_open() && fstat(get_fd(), ptr) == 0;
    }

}

#endif //BASE_IOFILE_HPP
