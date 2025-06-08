//
// Created by taganyer on 25-3-3.
//

#ifndef NET_MESSAGEAGENT_HPP
#define NET_MESSAGEAGENT_HPP

#ifdef NET_MESSAGEAGENT_HPP

#include <cassert>
#include "error/error_mark.hpp"
#include "monitors/Event.hpp"
#include "tinyBackend/Base/Buffer/InputBuffer.hpp"
#include "tinyBackend/Base/Buffer/OutputBuffer.hpp"
#include "tinyBackend/Base/Detail/CurrentThread.hpp"

namespace Net {

    class MessageAgent : Base::NoCopy {
    public:
        MessageAgent() = default;

        MessageAgent(MessageAgent&& other) noexcept :
            socket_event(other.socket_event), error(other.error) {
            other.socket_event = {};
            other.error = {};
        };

        virtual ~MessageAgent() = default;

        virtual int64 receive_message() = 0;

        virtual int64 send_message() = 0;

        virtual void close() = 0;

        /// 设置 MessageAgent 在当前线程可以运行。
        void set_running_thread() {
            _running_thread = Base::CurrentThread::tid();
        };

        /// 此函数的主要目的是警告并检测可能存在的不合理的多线程访问情况。
        void assert_thread_safe() const {
            assert(Base::CurrentThread::tid() == _running_thread || _running_thread == 0);
        };

        [[nodiscard]] virtual const Base::InputBuffer& input() const = 0;

        [[nodiscard]] virtual const Base::OutputBuffer& output() const = 0;

        [[nodiscard]] virtual int fd() const = 0;

        [[nodiscard]] virtual bool agent_valid() const = 0;

        [[nodiscard]] virtual uint32 can_receive() const = 0;

        [[nodiscard]] virtual uint32 can_send() const = 0;

        Event socket_event {};

        error_mark error {};

    protected:
        pthread_t _running_thread {};

    };
}

#endif

#endif //NET_MESSAGEAGENT_HPP
