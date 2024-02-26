//
// Created by taganyer on 24-2-22.
//

#include "File.hpp"
#include <cstdlib>

namespace Base {

    iFile::iFile(const char *path, bool binary) {
        open(path, binary);
    }

    iFile::iFile(iFile &&other) noexcept: _file(other._file) {
        other._file = nullptr;
    }

    iFile::~iFile() {
        close();
    }

    bool iFile::open(const char *path, bool binary) {
        close();
        _file = fopen(path, binary ? "br" : "r");
        return _file;
    }

    bool iFile::close() {
        if (!_file) return true;
        if (fclose(_file) == 0) _file = nullptr;
        return !_file;
    }

    int iFile::getChar() {
        return fgetc(_file);
    }

    int32 iFile::getInt32() {
        int32 ans = 0;
        fscanf(_file, "%*[^0-9]%d", &ans);
        return ans;
    }

    uint32 iFile::getUInt32() {
        uint32 ans = 0;
        fscanf(_file, "%*[^0-9]%u", &ans);
        return ans;
    }

    int64 iFile::getInt64() {
        int64 ans = 0;
        fscanf(_file, "%*[^0-9]%lld", &ans);
        return ans;
    }

    uint64 iFile::getUInt64() {
        uint64 ans = 0;
        fscanf(_file, "%*[^0-9]%llu", &ans);
        return ans;
    }

    double iFile::getDouble() {
        double ans = 0;
        fscanf(_file, "%*[^0-9.-]%lf", &ans);
        return ans;
    }

    string iFile::getBytes(uint64 size) {
        string ans(size, '\0');
        auto _s = fread(ans.data(), 1, size, _file);
        ans.resize(_s);
        return ans;
    }

    uint64 iFile::getBytes(uint64 size, void *dest) {
        return fread(dest, 1, size, _file);
    }

    string iFile::getline() {
        return get_until('\n');
    }

    void iFile::getline(char *dest, uint64 size) {
        fgets(dest, size, _file);
    }

    string iFile::get_until(int target) {
        char *ptr = nullptr;
        size_t t = getdelim(&ptr, &t, target, _file);
        if (t && ptr[t - 1] == target) --t;
        string ans(ptr, t);
        free(ptr);
        return ans;
    }

    int64 iFile::get_until(char target, char *dest, size_t size) {
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


    oFile::oFile(const char *path, bool append, bool binary) {
        open(path, append, binary);
    }

    oFile::oFile(oFile &&other) noexcept: _file(other._file) {
        other._file = nullptr;
    }

    oFile::~oFile() {
        close();
    }

    bool oFile::open(const char *path, bool append, bool binary) {
        close();
        char mod[3]{'w', 'b', '\0'};
        if (append) mod[0] = 'a';
        if (!binary) mod[1] = '\0';
        _file = fopen(path, mod);
        return _file;
    }

    bool oFile::close() {
        if (!_file) return true;
        if (fclose(_file) == 0) _file = nullptr;
        return !_file;
    }

    int oFile::putChar(int ch) {
        return fputc(ch, _file);
    }

    int oFile::putInt32(int32 i) {
        return fprintf(_file, "%d", i);
    }

    int oFile::putUInt32(uint32 i) {
        return fprintf(_file, "%u", i);
    }

    int oFile::putInt64(int64 i) {
        return fprintf(_file, "%lld", i);
    }

    int oFile::putUInt64(int64 i) {
        return fprintf(_file, "%llu", i);
    }

    int oFile::putDouble(double d) {
        return fprintf(_file, "%lf", d);
    }

    size_t oFile::putBytes(const void *str, size_t len) {
        if (len == -1) return fputs((const char *) str, _file);
        return fwrite(str, 1, len, _file);
    }

    size_t oFile::put_line(const char *str, size_t len) {
        size_t flag = putBytes(str, len);
        flag += fputc('\n', _file) == EOF;
        return flag;
    }
}