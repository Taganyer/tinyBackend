//
// Created by taganyer on 24-9-5.
//

#ifndef LOGSYSTEM_BLOCKFILE_HPP
#define LOGSYSTEM_BLOCKFILE_HPP

#ifdef LOGSYSTEM_BLOCKFILE_HPP

#include "ioFile.hpp"

namespace Base {

    class BlockFile {
    public:
        BlockFile() = default;

        explicit BlockFile(const char *path, bool append,
                           bool binary = true, uint64 block_size = (1 << 12));

        BlockFile(BlockFile&& other) noexcept;

        bool open(const char *path, bool append, bool binary);

        bool close();

        uint64 read(void *dest, uint64 index, uint64 count = 1);

        uint64 write_to_back(const void *data, uint64 size);

        uint64 update(const void *data, uint64 size, uint64 index);

        bool erase_back_blocks(uint64 count);

        bool resize_file_total_blocks(uint64 new_block_size);

        void flush() const { _file.flush(); };

        void flush_to_disk() const { _file.flush_to_disk(); };

        bool delete_file() { return _file.delete_file(); };

        [[nodiscard]] uint64 block_size() const { return _block_size; };

        [[nodiscard]] uint64 total_blocks() const { return _total_blocks; };

        [[nodiscard]] bool is_open() const { return _file.is_open(); };

        [[nodiscard]] const std::string& get_path() const { return _file.get_path(); };

    private:
        bool locating(uint64 index);

        uint64 padding_write(const void *data, uint64 size);

        bool write_error_handle();

        ioFile _file;

        int64 _offset = 0;

        uint64 _total_blocks = 0;

        uint64 _block_size = 0;

    };

}

#endif

#endif //LOGSYSTEM_BLOCKFILE_HPP
