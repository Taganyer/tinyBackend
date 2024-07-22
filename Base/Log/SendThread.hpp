//
// Created by taganyer on 3/28/24.
//

#ifndef BASE_SENDTHREAD_HPP
#define BASE_SENDTHREAD_HPP

#ifdef BASE_SENDTHREAD_HPP

#include "Sender.hpp"
#include "SendBuffer.hpp"
#include "Base/Condition.hpp"
#include "Base/Container/List.hpp"

namespace Base {

    class Log;

    class SendThread : NoCopy {
    private:
        class SenderData {
        public:
            SenderData(Sender::SenderPtr sender) : _sender(std::move(sender)) {};

        private:
            Sender::SenderPtr _sender;
            int waiting_buffers = 0;
            bool need_flush = true;
            bool shutdown = false;

            friend class SendThread;
        };

        using SenderQueue = List<SenderData>;

        using BufferQueue = List<SendBuffer>;

        using BufferPtr = BufferQueue::Iter;

    public:
        using SenderIter = SenderQueue::Iter;

        struct Data;

        static const Time_difference FLUSH_TIME;

        SendThread();

        ~SendThread();

        void add_sender(const Sender::SenderPtr &sender, Data &data);

        /// 对象销毁时要保证所有日志已经完全写入
        void remove_sender(Data &data);

        void put_buffer(Data &data);

        /// 会阻塞直到强制刷新结束（刷新注册到线程中的所有 Sender 调用此函数前的所有内容），
        /// 之后无法进行发送并且无法恢复。
        void shutdown_thread();

        [[nodiscard]] bool closed() const { return shutdown; };

    private:
        volatile bool shutdown = false;
        volatile bool running = false;

        Mutex _mutex;

        Condition _condition;

        Time_difference _next_flush_time;

        SenderQueue _senders;

        BufferQueue _buffers;

        std::vector<BufferPtr> _empty;

        std::vector<Data> _ready;

        std::vector<Data> _invoking;

        BufferPtr get_empty();

        void get_readyQueue();

        void wait_if_no_sender();

        bool waiting(Time_difference endTime);

        void get_need_flush(std::vector<SenderIter> &need_flush);

        void flush_need(std::vector<SenderIter> &need_flush);

        void send();

        void closing();

    public:
        struct Data {
        private:
            SenderIter sender;

        public:
            BufferPtr buffer;

            friend class SendThread;
        };

    };

}

#endif

#endif //BASE_SENDTHREAD_HPP
