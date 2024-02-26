//
// Created by taganyer on 24-2-22.
//

#ifndef BASE_FILE_HPP
#define BASE_FILE_HPP

#include "Detail/config.hpp"
#include "Detail/NoCopy.hpp"
#include <cstdio>
#include <string>


namespace Base {
    using std::string;

    class iFile : NoCopy {
    public:

        iFile() = default;

        iFile(const char *path, bool binary = false);

        iFile(iFile &&other) noexcept;

        ~iFile();

        bool open(const char *path, bool binary = false);

        bool close();

        int getChar();

        int32 getInt32();

        uint32 getUInt32();

        int64 getInt64();

        uint64 getUInt64();

        double getDouble();

        string getBytes(uint64 size);

        uint64 getBytes(uint64 size, void *dest);

        string getline();

        void getline(char *dest, uint64 size = 256);

        string get_until(int target);

        int64 get_until(char target, char *dest, size_t size = 256);

        string getAll();

        void skip_to(int ch);

        int64 find(int ch);

        bool seek_cur(int64 step) { return fseek(_file, step, SEEK_CUR) == 0; };

        bool seek_beg(int64 step) { return fseek(_file, step, SEEK_SET) == 0; };

        bool seek_end(int64 step) { return fseek(_file, step, SEEK_END) == 0; };

        operator bool() const { return is_open() && !feof(_file); };

        [[nodiscard]] uint64 pos() const { return ftell(_file); };

        [[nodiscard]] bool is_open() const { return _file; };

        [[nodiscard]] bool is_end() const { return feof(_file); };

        [[nodiscard]] uint64 size() const {
            if (!_file) return 0;
            auto pos = ftell(_file);
            fseek(_file, 0, SEEK_END);
            auto _size = ftell(_file) - pos;
            fseek(_file, pos, SEEK_SET);
            return _size;
        };

        int get_fd() {
            if (!_file) return -1;
            return fileno(_file);
        };

        FILE *get_fp() { return _file; };

    private:

        FILE *_file = nullptr;

    };

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

        size_t putBytes(const void *str, size_t len = -1);

        size_t put_line(const char *str, size_t len = -1);

        template<typename ...Args>
        int formatPut(const char *f, Args &... args) {
            return fprintf(_file, f, args...);
        };

        operator bool() const { return is_open(); };

        [[nodiscard]] bool is_open() const { return _file; };

        int get_fd() {
            if (!_file) return -1;
            return fileno(_file);
        };

        FILE *get_fp() { return _file; };

    private:

        FILE *_file = nullptr;

    };

}


#endif //BASE_FILE_HPP
