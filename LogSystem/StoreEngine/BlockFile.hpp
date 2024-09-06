//
// Created by taganyer on 24-9-5.
//

#ifndef LOGSYSTEM_BLOCKFILE_HPP
#define LOGSYSTEM_BLOCKFILE_HPP

#ifdef LOGSYSTEM_BLOCKFILE_HPP

#include "Base/Detail/ioFile.hpp"

namespace LogSystem {

    class BlockFile {
    public:
        static constexpr uint64 BlockSize = 1 << 12;

        BlockFile() = default;

        BlockFile(const char* path, bool append, bool binary);

        BlockFile(BlockFile &&other) noexcept;

        bool open(const char* path, bool append, bool binary);

        bool close();

        uint64 read(void* dest, uint64 index, uint64 count = 1);

        uint64 write_to_back(const void* data, uint64 size);

        uint64 update(const void* data, uint64 size, uint64 index);

        bool erase_back_blocks(uint64 count);

        void flush() { _file.flush(); };

        void flush_to_disk() { _file.flush_to_disk(); };

        [[nodiscard]] uint64 blocks_size() const { return _total_blocks; };

        [[nodiscard]] bool is_open() const { return _file.is_open(); };

    private:
        bool locating(uint64 index);

        uint64 padding_write(const void* data, uint64 size);

        bool write_error_handle();

        Base::ioFile _file;

        int64 _offset = 0;

        uint64 _total_blocks = 0;

    };
}

#endif

#endif //LOGSYSTEM_BLOCKFILE_HPP
