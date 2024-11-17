//
// Created by taganyer on 24-11-13.
//

#ifndef BASE_FILEDIR_HPP
#define BASE_FILEDIR_HPP

#ifdef BASE_FILEDIR_HPP

#include <cstdlib>
#include <dirent.h>
#include "config.hpp"
#include "NoCopy.hpp"

namespace Base {

    class DirentArray : NoCopy {
    public:
        using Filter = int (*)(const dirent*);

        using Compare = int (*)(const dirent**, const dirent**);

        static constexpr Compare sort_by_alpha = alphasort;

        static constexpr Compare sort_by_version = versionsort;

        DirentArray() = default;

        explicit DirentArray(const char* path,
                             Filter filter = nullptr,
                             Compare cmp = sort_by_alpha) {
            _size = scandir(path, &_arr, filter, cmp);
        };

        DirentArray(DirentArray &&other) noexcept :
            _arr(other._arr), _size(other._size) {
            other._arr = nullptr;
            other._size = -1;
        };

        DirentArray& operator=(DirentArray &&other) noexcept {
            if (this != &other) {
                _arr = other._arr;
                _size = other._size;
                other._arr = nullptr;
                other._size = -1;
            }
            return *this;
        };

        ~DirentArray() { close(); };

        void close() {
            if (!valid()) return;
            for (int i = 0; i < _size; ++i)
                free(_arr[i]);
            free(_arr);
            _arr = nullptr;
            _size = -1;
        };

        dirent* operator[](uint32 index) const {
            if (index >= _size) return nullptr;
            return _arr[index];
        };

        [[nodiscard]] int size() const { return _size; };

        [[nodiscard]] bool valid() const { return _size >= 0 && _arr; };

    private:
        dirent** _arr = nullptr;

        int _size = -1;

        friend class FileDir;

    };

    class FileDir : NoCopy {
    public:
        FileDir() = default;

        explicit FileDir(const char* path) { open(path); };

        FileDir(FileDir &&other) noexcept :
            _dir(other._dir), _dirent(other._dirent), _size(other._size) {
            other._dir = nullptr;
            other._dirent = nullptr;
            other._size = -1;
        };

        FileDir& operator=(FileDir &&other) noexcept {
            if (this != &other) {
                _dir = other._dir;
                _dirent = other._dirent;
                _size = other._size;
                other._dir = nullptr;
                other._dirent = nullptr;
                other._size = -1;
            }
            return *this;
        };

        ~FileDir() { close(); };

        bool close() {
            if (!_dir) return true;
            if (closedir(_dir) == 0) {
                _dir = nullptr;
                _dirent = nullptr;
                _size = -1;
                return true;
            }
            return false;
        };

        bool open(const char* path) {
            close();
            _dir = opendir(path);
            return _dir;
        };

        dirent* next() {
            if (!valid()) return nullptr;
            return _dirent = readdir(_dir);
        };

        DirentArray local(DirentArray::Filter filter = nullptr,
                          DirentArray::Compare cmp = DirentArray::sort_by_alpha) const {
            DirentArray result;
            if (scandirat(fd(), ".", &result._arr, filter, cmp) < 0)
                return {};
            return result;
        };

        DirentArray relative(const char* relative_path,
                             DirentArray::Filter filter = nullptr,
                             DirentArray::Compare cmp = DirentArray::sort_by_alpha) const {
            DirentArray result;
            if (scandirat(fd(), relative_path, &result._arr, filter, cmp) < 0)
                return {};
            return result;
        };

        [[nodiscard]] dirent* read() const { return _dirent; };

        void rewind() const { if (valid()) rewinddir(_dir); };

        void seek(int64 pos) const { if (valid()) seekdir(_dir, pos); };

        [[nodiscard]] int64 pos() const {
            if (!valid()) return -1;
            return telldir(_dir);
        };

        [[nodiscard]] int fd() const {
            if (!valid()) return -1;
            return dirfd(_dir);
        };

        [[nodiscard]] bool valid() const { return _dir; };

    private:
        DIR* _dir = nullptr;

        dirent* _dirent = nullptr;

        int32 _size = -1;

    };

}

#endif

#endif //BASE_FILEDIR_HPP
