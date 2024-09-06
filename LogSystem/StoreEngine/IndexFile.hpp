//
// Created by taganyer on 24-9-5.
//

#ifndef INDEXFILE_HPP
#define INDEXFILE_HPP

#ifdef INDEXFILE_HPP

#include <exception>
#include "BlockFile.hpp"
#include "Base/Buffer/BufferPool.hpp"
#include "Base/Container/BpTree.hpp"
#include "Base/Time/Time_difference.hpp"

namespace LogSystem {

    class IndexFile {
    public:
        IndexFile(const char* path, bool append, bool binary);

        IndexFile(BlockFile &&other) noexcept;

        bool open(const char* path, bool append, bool binary);

        bool close();

        bool insert(Base::Time_difference time, const void* data, uint size);

        bool update(Base::Time_difference time, const void* data, uint size);

        bool erase(Base::Time_difference time);

        bool erase(Base::Time_difference begin, Base::Time_difference end);

        void flush();

        [[nodiscard]] bool is_open() const;

    private:
        class Index_impl;

        BlockFile _file;

        Base::BPTree<Index_impl> IndexTree;
    };

    class IndexFile::Index_impl {
    public:
        Index_impl(const BlockFile& file);

    private:
        BlockFile* _file;

    public:
        using Key = Base::Time_difference;

        class Value;

        class Result;

        class ResultSet;

        class Iter;

        class IndexBlock;

        class DataBlock;

        Iter begin_block();

        void set_begin_block(Iter iter);

        void erase_block(Iter iter);

        DataBlock create_data_block();

        IndexBlock create_index_block();

    };

    class IndexFile::Index_impl::Result {
    public:
        Result();

        explicit Result(const Iter &iter);
    };

    class IndexFile::Index_impl::ResultSet {
    public:
        ResultSet();

        void add(const Iter &iter);
    };

    class IndexFile::Index_impl::Iter {
    public:
        Iter();

        Key key() const;

        Iter& operator++();

        Iter& operator--();

        Iter& operator=(const Iter &other);

        bool operator==(const Iter &other) const;

        bool operator!=(const Iter &other) const;

        Iter operator+(int n) const;

        Iter operator-(int n) const;
    };

    class IndexFile::Index_impl::IndexBlock {
    public:
        using Iter = Iter;

        using Key = Key;

        [[nodiscard]] Iter begin() const;

        [[nodiscard]] Iter end() const;

        [[nodiscard]] Iter self_iter() const;

        [[nodiscard]] bool can_insert(const Iter &iter) const;

        void insert(const Iter &pos, const Iter &target);

        void update(const Iter &pos, Key key);

        bool erase(const Iter &iter);

        void erase(const Iter &begin, const Iter &end);

        void merge(const IndexBlock &other);

        void average(IndexBlock &other);

        int merge_keep_average(const IndexBlock &other);

    };

    class IndexFile::Index_impl::DataBlock {
        using Iter = Iter;

        using Key = Key;

        using Value = Value;

        [[nodiscard]] Iter begin() const;

        [[nodiscard]] Iter end() const;

        [[nodiscard]] Iter self_iter() const;

        [[nodiscard]] bool can_insert(Key key, const Value &value) const;

        void insert(const Iter &pos, Key key, const Value &value);

        bool update(const Iter &pos, Key key, const Value &value);

        bool erase(const Iter &iter);

        void erase(const Iter &begin, const Iter &end);

        void merge(const DataBlock &other);

        void average(DataBlock &other);

        int merge_keep_average(const DataBlock &other);

    };

}

#endif

#endif //INDEXFILE_HPP
