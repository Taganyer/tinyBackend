//
// Created by taganyer on 3/28/24.
//

#ifndef BASE_SENDTHREAD_HPP
#define BASE_SENDTHREAD_HPP


#include "Sender.hpp"
#include "SendBuffer.hpp"
#include "../Condition.hpp"
#include "../Container/List.hpp"

namespace Base {

    class Log;

    class SendThread : NoCopy {
    private:

        struct SenderData {
            Sender::SenderPtr _sender;
            int waiting_buffers = 0;
            bool need_flush = true;
            bool shutdown = false;

            SenderData(const Sender::SenderPtr sender) : _sender(sender) {};
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
            SenderIter sender;
            BufferPtr buffer;
        };

    };

}

#endif //BASE_SENDTHREAD_HPP
