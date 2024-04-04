//
// Created by taganyer on 4/4/24.
//

#ifndef BASE_IOFILE_HPP
#define BASE_IOFILE_HPP


#include <cstdio>
#include <unistd.h>
#include "config.hpp"
#include "NoCopy.hpp"

namespace Base {

    class ioFile : NoCopy {
    public:

        ioFile() = default;

        ioFile(const char *path, bool append, bool binary);

        ioFile(ioFile &&other) noexcept;

        ~ioFile();

        bool open(const char *path, bool append, bool binary);

        bool close();

        uint64 read(uint64 size, void *dest);

        size_t write(const void *str, size_t len = -1);

        bool seek_cur(int64 step) { return fseek(_file, step, SEEK_CUR) == 0; };

        bool seek_beg(int64 step) { return fseek(_file, step, SEEK_SET) == 0; };

        bool seek_end(int64 step) { return fseek(_file, step, SEEK_END) == 0; };

        operator bool() const { return is_open() && !feof(_file); };

        [[nodiscard]] uint64 pos() const { return ftell(_file); };

        [[nodiscard]] bool is_open() const { return _file; };

        [[nodiscard]] bool is_end() const { return feof(_file); };

        int get_fd() {
            if (!_file) return -1;
            return fileno(_file);
        };

        FILE *get_fp() { return _file; };

    protected:

        FILE *_file = nullptr;

    };

    ioFile::ioFile(const char *path, bool append, bool binary) {
        open(path, append, binary);
    }

    ioFile::ioFile(ioFile &&other) noexcept: _file(other._file) {
        other._file = nullptr;
    }

    ioFile::~ioFile() {
        close();
    }

    bool ioFile::open(const char *path, bool append, bool binary) {
        close();
        char mod[4]{'w', 'b', '+', '\0'};
        if (append) mod[0] = 'a';
        if (!binary) {
            mod[1] = '+';
            mod[2] = '\0';
        }
        _file = fopen(path, mod);
        return _file;
    }

    bool ioFile::close() {
        if (!_file) return true;
        if (fclose(_file) == 0) _file = nullptr;
        return !_file;
    }

    uint64 ioFile::read(uint64 size, void *dest) {
        return fread(dest, 1, size, _file);
    }

    size_t ioFile::write(const void *str, size_t len) {
        if (len == -1) return fputs((const char *) str, _file);
        return fwrite(str, 1, len, _file);
    }

}

#endif //BASE_IOFILE_HPP
