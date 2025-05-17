//
// Created by taganyer on 24-2-26.
//

#ifndef BASE_IFILE_HPP
#define BASE_IFILE_HPP

#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <sys/uio.h>
#include "NoCopy.hpp"
#include "BufferArray.hpp"


namespace Base {

    using std::string;

    class iFile : NoCopy {
    public:
        iFile() = default;

        explicit iFile(const char *path, bool binary = false);

        iFile(iFile&& other) noexcept;

        iFile& operator=(iFile&& other) noexcept;

        ~iFile();

        bool open(const char *path, bool binary = false);

        bool close();

        [[nodiscard]] string read(uint64 size) const;

        uint64 read(uint64 size, void *dest) const;

        template <std::size_t size>
        int64 read(const BufferArray<size>& array) const;

        [[nodiscard]] int getChar() const;

        [[nodiscard]] int32 getInt32() const;

        [[nodiscard]] uint32 getUInt32() const;

        [[nodiscard]] int64 getInt64() const;

        [[nodiscard]] uint64 getUInt64() const;

        [[nodiscard]] double getDouble() const;

        [[nodiscard]] string getline() const;

        int64 getline(char *dest, size_t size) const;

        [[nodiscard]] string get_until(int target) const;

        int64 get_until(char target, char *dest, size_t size = 256) const;

        [[nodiscard]] string getAll() const;

        void skip_to(int ch) const;

        [[nodiscard]] int64 find(int ch) const;

        bool delete_file();

        bool seek_cur(int64 step) const { return fseek(_file, step, SEEK_CUR) == 0; };

        bool seek_beg(int64 step) const { return fseek(_file, step, SEEK_SET) == 0; };

        bool seek_end(int64 step) const { return fseek(_file, step, SEEK_END) == 0; };

        operator bool() const { return is_open() && !feof(_file); };

        [[nodiscard]] uint64 pos() const { return ftell(_file); };

        [[nodiscard]] bool is_open() const { return _file; };

        [[nodiscard]] bool is_end() const { return feof(_file); };

        [[nodiscard]] const std::string& get_path() const { return _path; };

        [[nodiscard]] int get_fd() const {
            if (!_file) return -1;
            return fileno(_file);
        };

        [[nodiscard]] FILE* get_fp() const { return _file; };

        [[nodiscard]] bool get_stat(struct stat *ptr) const;

        [[nodiscard]] uint64 size() const;

    protected:
        FILE *_file = nullptr;

        std::string _path;

    };

}

namespace Base {
    inline iFile::iFile(const char *path, bool binary) {
        open(path, binary);
    }

    inline iFile::iFile(iFile&& other) noexcept: _file(other._file),
        _path(std::move(other._path)) {
        other._file = nullptr;
    }

    inline iFile& iFile::operator=(iFile&& other) noexcept {
        close();
        _file = other._file;
        other._file = nullptr;
        _path = std::move(other._path);
        return *this;
    }

    inline iFile::~iFile() {
        close();
    }

    inline bool iFile::open(const char *path, bool binary) {
        close();
        _file = fopen(path, binary ? "rb" : "r");
        if (_file)
            _path = path;
        return _file;
    }

    inline bool iFile::close() {
        if (!_file) return true;
        if (fclose(_file) == 0) {
            _file = nullptr;
            _path = std::string();
            return true;
        }
        return false;
    }

    inline string iFile::read(uint64 size) const {
        string ans(size, '\0');
        auto _s = fread(ans.data(), 1, size, _file);
        ans.resize(_s);
        return ans;
    }

    inline uint64 iFile::read(uint64 size, void *dest) const {
        return fread(dest, 1, size, _file);
    }

    template <std::size_t N>
    int64 iFile::read(const BufferArray<N>& array) const {
        return ::readv(get_fd(), array.data(), N);
    }

    inline int iFile::getChar() const {
        return fgetc(_file);
    }

    inline int32 iFile::getInt32() const {
        int32 ans = 0;
        fscanf(_file, "%*[^0-9]%d", &ans);
        return ans;
    }

    inline uint32 iFile::getUInt32() const {
        uint32 ans = 0;
        fscanf(_file, "%*[^0-9]%u", &ans);
        return ans;
    }

    inline int64 iFile::getInt64() const {
        int64 ans = 0;
        fscanf(_file, "%*[^0-9]%lld", &ans);
        return ans;
    }

    inline uint64 iFile::getUInt64() const {
        uint64 ans = 0;
        fscanf(_file, "%*[^0-9]%llu", &ans);
        return ans;
    }

    inline double iFile::getDouble() const {
        double ans = 0;
        fscanf(_file, "%*[^0-9.-]%lf", &ans);
        return ans;
    }

    inline string iFile::getline() const {
        return get_until('\n');
    }

    inline int64 iFile::getline(char *dest, size_t size) const {
        return getdelim(&dest, &size, '\n', _file);
    }

    inline string iFile::get_until(int target) const {
        char *ptr = nullptr;
        size_t t = getdelim(&ptr, &t, target, _file);
        if (t && ptr[t - 1] == target) --t;
        if (t == -1) t = 0;
        string ans(ptr, t);
        free(ptr);
        return ans;
    }

    inline int64 iFile::get_until(char target, char *dest, size_t size) const {
        return getdelim(&dest, &size, target, _file);
    }

    inline string iFile::getAll() const {
        string ans(size(), '\0');
        auto len = fread(ans.data(), 1, ans.size(), _file);
        ans.resize(len);
        return ans;
    }

    inline void iFile::skip_to(int ch) const {
        int c = fgetc(_file);
        while (c != EOF && c != ch)
            c = fgetc(_file);
        if (c != EOF) seek_cur(-1);
    }

    inline int64 iFile::find(int ch) const {
        auto pos = ftell(_file);
        int64 ans = -1;
        int c = fgetc(_file);
        while (c != EOF && c != ch)
            c = fgetc(_file);
        if (c != EOF) ans = ftell(_file);
        seek_beg(pos);
        return ans;
    }

    inline bool iFile::delete_file() {
        if (!_file || fclose(_file) != 0) return false;
        _file = nullptr;
        bool success = remove(_path.c_str()) == 0;
        _path = std::string();
        return success;
    }

    inline bool iFile::get_stat(struct stat *ptr) const {
        return is_open() && fstat(get_fd(), ptr) == 0;
    }

    inline uint64 iFile::size() const {
        struct stat st {};
        if (!get_stat(&st)) return 0;
        return st.st_size;
    }

}


#endif //BASE_IFILE_HPP
