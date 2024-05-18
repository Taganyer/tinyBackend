//
// Created by taganyer on 24-2-26.
//

#ifndef BASE_OFILE_HPP
#define BASE_OFILE_HPP

#ifdef BASE_OFILE_HPP

#include <cstdio>
#include <unistd.h>
#include "config.hpp"
#include "NoCopy.hpp"


namespace Base {

    class oFile : NoCopy {
    public:

        oFile() = default;

        oFile(const char *path, bool append = false, bool binary = false);

        oFile(oFile &&other) noexcept;

        ~oFile();

        bool open(const char *path, bool append = false, bool binary = false);

        bool close();

        int putChar(int ch);

        int putInt32(int32 i);

        int putUInt32(uint32 i);

        int putInt64(int64 i);

        int putUInt64(int64 i);

        int putDouble(double d);

        size_t write(const void *str, size_t len = -1);

        size_t put_line(const char *str, size_t len = -1);

        void flush();

        void flush_to_disk();

        template<typename ...Args>
        int formatPut(const char *f, Args ... args) {
            return fprintf(_file, f, args...);
        };

        operator bool() const { return is_open(); };

        [[nodiscard]] bool is_open() const { return _file; };

        int get_fd() {
            if (!_file) return -1;
            return fileno(_file);
        };

        FILE *get_fp() { return _file; };

    protected:

        FILE *_file = nullptr;

    };

}


namespace Base {

    inline oFile::oFile(const char *path, bool append, bool binary) {
        open(path, append, binary);
    }

    inline oFile::oFile(oFile &&other) noexcept: _file(other._file) {
        other._file = nullptr;
    }

    inline oFile::~oFile() {
        close();
    }

    inline bool oFile::open(const char *path, bool append, bool binary) {
        close();
        char mod[3]{'w', 'b', '\0'};
        if (append) mod[0] = 'a';
        if (!binary) mod[1] = '\0';
        _file = fopen(path, mod);
        return _file;
    }

    inline bool oFile::close() {
        if (!_file) return true;
        if (fclose(_file) == 0) _file = nullptr;
        return !_file;
    }

    inline int oFile::putChar(int ch) {
        return fputc(ch, _file);
    }

    inline int oFile::putInt32(int32 i) {
        return fprintf(_file, "%d", i);
    }

    inline int oFile::putUInt32(uint32 i) {
        return fprintf(_file, "%u", i);
    }

    inline int oFile::putInt64(int64 i) {
        return fprintf(_file, "%lld", i);
    }

    inline int oFile::putUInt64(int64 i) {
        return fprintf(_file, "%llu", i);
    }

    inline int oFile::putDouble(double d) {
        return fprintf(_file, "%lf", d);
    }

    inline size_t oFile::write(const void *str, size_t len) {
        if (len == -1) return fputs((const char *) str, _file);
        return fwrite(str, 1, len, _file);
    }

    inline size_t oFile::put_line(const char *str, size_t len) {
        size_t flag = write(str, len);
        if (fputc('\n', _file) != EOF) ++flag;
        return flag;
    }

    inline void oFile::flush() {
        fflush(_file);
    }

    inline void oFile::flush_to_disk() {
        fflush(_file);
        fsync(get_fd());
    }

}

#endif

#endif //BASE_OFILE_HPP
