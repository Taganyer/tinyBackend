//
// Created by taganyer on 24-9-9.
//

#ifndef BASE_IMPL_SCHEDULER_HPP
#define BASE_IMPL_SCHEDULER_HPP

#ifdef BASE_IMPL_SCHEDULER_HPP

#include "Interpreter.hpp"
#include "BPTreeErrors.hpp"
#include "Base/LRUCache.hpp"
#include "Base/ScheduledThread.hpp"
#include "Base/Detail/BlockFile.hpp"
#include "Base/Buffer/BufferPool.hpp"


namespace Base {

    class Impl_Scheduler : public Scheduler {
    public:
        using Buffer = BufferPool::Buffer;

        Impl_Scheduler(ScheduledThread &scheduled_thread, const char* filename, uint64 memory_size);

        Impl_Scheduler(ScheduledThread &scheduled_thread, BlockFile &&file, uint64 memory_size);

        ~Impl_Scheduler() override = default;

        void invoke(void* arg) override;

        void force_invoke() override;

        Mutex _mutex;

        ScheduledThread* _thread;

        std::pair<uint32, void *> get_block();

        void* get_block(uint32 index, bool read_data);

        void put_block(uint32 index, bool need_update);

        uint32 total_blocks() const { return _cache._helper._total_blocks; };

    private:
        struct BlockMessage {
            Buffer buffer;
            uint32 instantiated_size = 0;
            bool need_update = false;
        };

        class LRU_Helper {
        public:
            using Key = uint32;
            using Value = BufferPool::Buffer;

            LRU_Helper(const char* filename, uint64 memory_size);

            LRU_Helper(BlockFile &&file, uint64 memory_size);

            [[nodiscard]] bool can_create(Key) const;

            Value create(Key index);

            void update(Key index, Value &buffer);

        private:
            BufferPool _memory_pool;

            BlockFile _file;

            uint32 _total_blocks = 0;

            bool need_read_from_file = false;

            friend class Impl_Scheduler;

        };

        LRUCache<LRU_Helper> _cache;
    };
}

#endif

#endif //BASE_IMPL_SCHEDULER_HPP
