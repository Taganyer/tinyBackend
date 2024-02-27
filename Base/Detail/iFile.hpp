//
// Created by taganyer on 24-2-26.
//

#ifndef BASE_IFILE_HPP
#define BASE_IFILE_HPP

#include "config.hpp"
#include "NoCopy.hpp"
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

        string read(uint64 size);

        uint64 read(uint64 size, void *dest);

        string getline();

        int64 getline(char *dest, size_t size);

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

}

namespace Base {
    inline iFile::iFile(const char *path, bool binary) {
        open(path, binary);
    }

    inline iFile::iFile(iFile &&other) noexcept: _file(other._file) {
        other._file = nullptr;
    }

    inline iFile::~iFile() {
        close();
    }

    inline bool iFile::open(const char *path, bool binary) {
        close();
        _file = fopen(path, binary ? "rb" : "r");
        return _file;
    }

    inline bool iFile::close() {
        if (!_file) return true;
        if (fclose(_file) == 0) _file = nullptr;
        return !_file;
    }

    inline int iFile::getChar() {
        return fgetc(_file);
    }

    inline int32 iFile::getInt32() {
        int32 ans = 0;
        fscanf(_file, "%*[^0-9]%d", &ans);
        return ans;
    }

    inline uint32 iFile::getUInt32() {
        uint32 ans = 0;
        fscanf(_file, "%*[^0-9]%u", &ans);
        return ans;
    }

    inline int64 iFile::getInt64() {
        int64 ans = 0;
        fscanf(_file, "%*[^0-9]%lld", &ans);
        return ans;
    }

    inline uint64 iFile::getUInt64() {
        uint64 ans = 0;
        fscanf(_file, "%*[^0-9]%llu", &ans);
        return ans;
    }

    inline double iFile::getDouble() {
        double ans = 0;
        fscanf(_file, "%*[^0-9.-]%lf", &ans);
        return ans;
    }

    inline string iFile::read(uint64 size) {
        string ans(size, '\0');
        auto _s = fread(ans.data(), 1, size, _file);
        ans.resize(_s);
        return ans;
    }

    inline uint64 iFile::read(uint64 size, void *dest) {
        return fread(dest, 1, size, _file);
    }

    inline string iFile::getline() {
        return get_until('\n');
    }

    inline int64 iFile::getline(char *dest, size_t size) {
        return getdelim(&dest, &size, '\n', _file);
    }

    inline string iFile::get_until(int target) {
        char *ptr = nullptr;
        size_t t = getdelim(&ptr, &t, target, _file);
        if (t && ptr[t - 1] == target) --t;
        if (t == -1) t = 0;
        string ans(ptr, t);
        free(ptr);
        return ans;
    }

    inline int64 iFile::get_until(char target, char *dest, size_t size) {
        return getdelim(&dest, &size, target, _file);
    }

    string iFile::getAll() {
        string ans;
        char arr[256];
        while (auto len = fread(arr, 1, 256, _file))
            ans.append(arr, len);
        return ans;
    }

    void iFile::skip_to(int ch) {
        int c = fgetc(_file);
        while (c != EOF && c != ch)
            c = fgetc(_file);
        if (c != EOF) seek_cur(-1);
    }

    int64 iFile::find(int ch) {
        auto pos = ftell(_file);
        int64 ans = -1;
        int c = fgetc(_file);
        while (c != EOF && c != ch)
            c = fgetc(_file);
        if (c != EOF) ans = ftell(_file);
        seek_beg(pos);
        return ans;
    }

}


#endif //BASE_IFILE_HPP
