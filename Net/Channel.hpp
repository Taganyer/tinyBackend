//
// Created by taganyer on 25-3-3.
//

#ifndef NET_CHANNEL_HPP
#define NET_CHANNEL_HPP

#ifdef NET_CHANNEL_HPP

#include <functional>
#include "MessageAgent.hpp"

namespace Net {

    /// Channel 并不是线程安全的，如果要使用异步函数，确保其不会调用 MessageAgent 的任何函数，
    /// 并且不操作 Base::InputBuffer 和 Base::OutputBuffer。
    class Channel {
    public:
        using ReadCallback = std::function<void(MessageAgent&)>;

        using WriteCallback = std::function<void(MessageAgent&)>;

        /*
         * 返回值表示是否调用 CloseCallback
         * 可能返回的错误有：
         *                error_types::Read,
         *                error_types::Write,
         *                error_types::Epoll_ctl,
         *                error_types::ErrorEvent,
         *                error_types::TimeoutEvent
        */
        using ErrorCallback = std::function<void(MessageAgent&)>;

        using CloseCallback = std::function<void(MessageAgent&)>;

        void set_readCallback(ReadCallback event) { _readFun = std::move(event); };

        void set_writeCallback(WriteCallback event) { _writeFun = std::move(event); };

        void set_errorCallback(ErrorCallback event) { _errorFun = std::move(event); };

        void set_closeCallback(CloseCallback event) { _closeFun = std::move(event); };

        void invoke_event(MessageAgent& agent) const;

    private:
        ReadCallback _readFun;

        WriteCallback _writeFun;

        ErrorCallback _errorFun;

        CloseCallback _closeFun;

        void handle_read(MessageAgent& agent) const;

        void handle_write(MessageAgent& agent) const;

        void handle_error(MessageAgent& agent) const;

        void handle_close(MessageAgent& agent) const;

    };
}

#endif

#endif //NET_CHANNEL_HPP
