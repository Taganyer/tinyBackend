//
// Created by taganyer on 24-10-19.
//

#ifndef LOGSYSTEM_LINKLOGINTERPRETER_HPP
#define LOGSYSTEM_LINKLOGINTERPRETER_HPP

#ifdef LOGSYSTEM_LINKLOGINTERPRETER_HPP

#include <queue>
#include <functional>
#include "LinkLogOperation.hpp"
#include "Base/Detail/iFile.hpp"
#include "Base/Detail/oFile.hpp"
#include "Base/Buffer/RingBuffer.hpp"
#include "Base/BPTree_impls/BPTree.hpp"
#include "Base/BPTree_impls/BPTree_impl.hpp"

namespace LogSystem {

    class LinkLogEncoder {
    public:
        using RecordFile = Base::oFile;

        static constexpr uint32 RECORD_FILE_LIMIT_SIZE = 1 << 20;

        static constexpr uint32 FILE_SIZE_LIMIT = 10;

        explicit LinkLogEncoder(std::string dictionary_path);

        static uint32 register_logger(LinkNodeType type_,
                                      const LinkServiceID &service_,
                                      const LinkNodeID &node_,
                                      const LinkNodeID &parent_node_,
                                      Base::TimeInterval time_,
                                      Base::TimeInterval parent_init_time_,
                                      Base::TimeInterval expire_time_,
                                      void* dest, uint32 limit);

        static uint32 create_logger(LinkNodeType type_,
                                    const LinkServiceID &service_,
                                    const LinkNodeID &node_,
                                    const LinkNodeID &parent_node_,
                                    Base::TimeInterval init_time_,
                                    Base::TimeInterval parent_init_time_,
                                    void* dest, uint32 limit);

        static uint32 end_logger(const LinkServiceID &service_,
                                 const LinkNodeID &node_,
                                 Base::TimeInterval init_time_,
                                 Base::TimeInterval end_time_,
                                 void* dest, uint32 limit);

        static bool can_write_log(uint16 size, uint32 limit);

        static uint16 write_log(const LinkServiceID &service_,
                                Base::TimeInterval node_init_time_,
                                const LinkNodeID &node_,
                                Base::TimeInterval time_,
                                LogRank rank_,
                                const void* data, uint16 size,
                                void* dest);

        uint32 write_to_file(const void* data, uint32 size);

        void flush() const;

    private:
        uint64 _current_size = 0;

        RecordFile _record_file;

        std::string _dictionary_path;

        std::vector<std::string> _file_names;

        bool open_new_file();

        [[nodiscard]] std::string get_record_file_name(Base::TimeInterval file_init_time) const;

    };


    class LinkLogStorage {
    public:
        using LinkLogReadFile = Base::iFile;

        using LinkLogWriteFile = Base::oFile;

        using FilenameFile = Base::BPTree<Base::BPTree_impl<Base::TimeInterval, uint32>>;

        using LinkIndexFile = Base::BPTree<Base::BPTree_impl<Index_Key, Index_Value>>;

        using NodeDeletionFile = Base::BPTree<Base::BPTree_impl<Base::TimeInterval, Index_Key>>;

        static constexpr uint32 FILENAME_BUFFER_POOL_SIZE = 1 << 16;

        static constexpr uint32 LINK_INDEX_BUFFER_POOL_SIZE = 1 << 22;

        static constexpr uint32 NODE_DELETION_BUFFER_POOL_SIZE = 1 << 16;

        static constexpr uint32 LOG_FILE_LIMIT_SIZE = 1 << 28;

        explicit LinkLogStorage(std::string dictionary_path, Base::ScheduledThread &scheduled_thread);

        bool create_logger(const LinkServiceID &service,
                           const LinkNodeID &node,
                           Base::TimeInterval node_init_time);

        void update_logger(const LinkServiceID &service,
                           const LinkNodeID &node,
                           Base::TimeInterval parent_init_time,
                           const LinkNodeID &parent,
                           LinkNodeType type,
                           Base::TimeInterval node_init_time);

        bool create_logger(const LinkServiceID &service,
                           const LinkNodeID &node,
                           Base::TimeInterval parent_init_time,
                           const LinkNodeID &parent,
                           LinkNodeType type,
                           Base::TimeInterval node_init_time);

        void end_logger(const LinkServiceID &service,
                        const LinkNodeID &node,
                        Base::TimeInterval node_init_time);

        void add_record(uint32 size);

        void handle_a_log(Link_Log_Header &header, const Base::RingBuffer &buf);

        uint32 write_to_file(const Base::RingBuffer &buf, uint32 size);

        void delete_oldest_files(uint32 size);

        class QuerySet {
            friend class LinkLogStorage;

            using LocationSet = std::priority_queue<std::pair<Base::TimeInterval, uint32>>;

            LinkLogReadFile _file;

            int64 _pos = 0;

            LocationSet _log_location;

            Base::TimeInterval _current_file;

        };

        using NodeFilter = std::function<bool(const Index_Key &, const Index_Value &)>;

        QuerySet* get_query_set(const LinkServiceID &id,
                                const NodeFilter &filter = NodeFilter());

        static void destroy_query_set(void* set);

        std::pair<uint32, QuerySet *> query(void* dest, uint32 limit, QuerySet* point);

        std::pair<uint32, QuerySet *> query(Base::RingBuffer &buf, QuerySet* point);

        void flush_log_file();

        void flush_file_name();

        void flush_index_file();

        void flush_deletion_file();

    private:
        class CacheHelper {
        public:
            using Key = Index_Key;
            using Value = Index_Value;

            static constexpr uint32 CACHE_LIMIT_SIZE = 3000;

            explicit CacheHelper(LinkLogStorage &interpreter) :
                _interpreter(interpreter) {};

            [[nodiscard]] bool can_create(const Key &key) const;

            [[nodiscard]] Value create(const Key &key) const;

            void update(const Key &key, Value &value) const;

            void erase(const Key &key) const;

        private:
            LinkLogStorage &_interpreter;

        };

        struct Record {
            Base::TimeInterval file = Base::Unix_to_now(), latest_time = file;
            uint32 index = 0, written = 0;
        };

        Base::Mutex _mutex;

        LinkLogWriteFile _logs;

        std::vector<Record> _records;

        std::string _dictionary_path;

        FilenameFile _filename_file;

        LinkIndexFile _indexes;

        NodeDeletionFile _node_deletion;

        Base::LRUCache<CacheHelper> _cache;

        void flush_cache();

        [[nodiscard]] std::string filename_file_name() const;

        [[nodiscard]] std::string index_file_name() const;

        [[nodiscard]] std::string deletion_file_name() const;

        [[nodiscard]] LinkLogReadFile get_log_file(Base::TimeInterval file_init_time) const;

        [[nodiscard]] std::string get_log_name(Base::TimeInterval file_init_time) const;

        void open_new_log_file(const Record &record, Base::TimeInterval end_time);

        bool prepare_read_a_log(QuerySet* point, char* msg) const;

        int querying_a_log(QuerySet* point, char* ptr, uint32 limit, uint32 &written) const;

        int querying_a_log(QuerySet* point, Base::RingBuffer &buf, uint32 &written) const;

    };

}

#endif

#endif //LOGSYSTEM_LINKLOGINTERPRETER_HPP
