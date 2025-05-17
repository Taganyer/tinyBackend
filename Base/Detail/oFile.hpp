//
// Created by taganyer on 24-2-26.
//

#ifndef BASE_OFILE_HPP
#define BASE_OFILE_HPP

#include <cstdio>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include "NoCopy.hpp"
#include "BufferArray.hpp"


namespace Base {

    class oFile : NoCopy {
    public:
        oFile() = default;

        explicit oFile(const char *path, bool append = false, bool binary = false);

        oFile(oFile&& other) noexcept;

        oFile& operator=(oFile&& other) noexcept;

        ~oFile();

        bool open(const char *path, bool append = false, bool binary = false);

        bool close();

        uint64 write(const void *str, size_t len = -1) const;

        template <std::size_t size>
        int64 write(const BufferArray<size>& array) const;

        uint64 put_line(const char *str, size_t len = -1) const;

        [[nodiscard]] int putChar(int ch) const;

        [[nodiscard]] int putInt32(int32 i) const;

        [[nodiscard]] int putUInt32(uint32 i) const;

        [[nodiscard]] int putInt64(int64 i) const;

        [[nodiscard]] int putUInt64(int64 i) const;

        [[nodiscard]] int putDouble(double d) const;

        void flush() const;

        void flush_to_disk() const;

        [[nodiscard]] bool resize_file(uint64 size) const;

        bool delete_file();

        template <typename... Args>
        int formatPut(const char *f, Args... args) {
            return fprintf(_file, f, args...);
        };

        operator bool() const { return is_open(); };

        [[nodiscard]] bool is_open() const { return _file; };

        [[nodiscard]] const std::string& get_path() const { return _path; };

        [[nodiscard]] int get_fd() const {
            if (!_file) return -1;
            return fileno(_file);
        };

        [[nodiscard]] FILE* get_fp() const { return _file; };

        [[nodiscard]] bool get_stat(struct stat *ptr) const;

    protected:
        FILE *_file = nullptr;

        std::string _path;

    };

}


namespace Base {

    inline oFile::oFile(const char *path, bool append, bool binary) {
        open(path, append, binary);
    }

    inline oFile::oFile(oFile&& other) noexcept: _file(other._file) {
        other._file = nullptr;
    }

    inline oFile& oFile::operator=(oFile&& other) noexcept {
        close();
        _file = other._file;
        other._file = nullptr;
        _path = std::move(other._path);
        return *this;
    }

    inline oFile::~oFile() {
        close();
    }

    inline bool oFile::open(const char *path, bool append, bool binary) {
        close();
        char mod[3] { 'w', 'b', '\0' };
        if (append) mod[0] = 'a';
        if (!binary) mod[1] = '\0';
        _file = fopen(path, mod);
        if (_file)
            _path = path;
        return _file;
    }

    inline bool oFile::close() {
        if (!_file) return true;
        if (fclose(_file) == 0) {
            _file = nullptr;
            _path = std::string();
            return true;
        }
        return false;
    }

    inline uint64 oFile::write(const void *str, size_t len) const {
        if (len == -1) return fputs((const char *) str, _file);
        return fwrite(str, 1, len, _file);
    }

    template <std::size_t N>
    int64 oFile::write(const BufferArray<N>& array) const {
        return ::writev(get_fd(), array.data(), N);
    }

    inline uint64 oFile::put_line(const char *str, size_t len) const {
        size_t flag = write(str, len);
        if (fputc('\n', _file) != EOF) ++flag;
        return flag;
    }

    inline int oFile::putChar(int ch) const {
        return fputc(ch, _file);
    }

    inline int oFile::putInt32(int32 i) const {
        return fprintf(_file, "%d", i);
    }

    inline int oFile::putUInt32(uint32 i) const {
        return fprintf(_file, "%u", i);
    }

    inline int oFile::putInt64(int64 i) const {
        return fprintf(_file, "%lld", i);
    }

    inline int oFile::putUInt64(int64 i) const {
        return fprintf(_file, "%llu", i);
    }

    inline int oFile::putDouble(double d) const {
        return fprintf(_file, "%lf", d);
    }

    inline void oFile::flush() const {
        fflush(_file);
    }

    inline void oFile::flush_to_disk() const {
        fflush(_file);
        fsync(get_fd());
    }

    inline bool oFile::resize_file(uint64 size) const {
        return ftruncate(get_fd(), size) == 0;
    }

    inline bool oFile::delete_file() {
        if (!_file || fclose(_file) != 0) return false;
        _file = nullptr;
        bool success = remove(_path.c_str()) == 0;
        _path = std::string();
        return success;
    }

    inline bool oFile::get_stat(struct stat *ptr) const {
        return is_open() && fstat(get_fd(), ptr) == 0;
    }

}

#endif //BASE_OFILE_HPP
