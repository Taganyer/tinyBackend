//
// Created by taganyer on 25-3-3.
//

#ifndef NET_MESSAGEAGENT_HPP
#define NET_MESSAGEAGENT_HPP

#ifdef NET_MESSAGEAGENT_HPP

#include "monitors/Event.hpp"
#include "error/error_mark.hpp"
#include "Base/Buffer/InputBuffer.hpp"
#include "Base/Buffer/OutputBuffer.hpp"

namespace Net {

    class MessageAgent : Base::NoCopy {
    public:
        MessageAgent() = default;

        MessageAgent(MessageAgent &&other) noexcept :
            socket_event(other.socket_event), error(other.error) {
            other.socket_event = {};
            other.error = {};
        };

        virtual ~MessageAgent() = default;

        virtual int64 receive_message() = 0;

        virtual int64 send_message() = 0;

        virtual void close() = 0;

        [[nodiscard]] virtual const Base::InputBuffer& input() const = 0;

        [[nodiscard]] virtual const Base::OutputBuffer& output() const = 0;

        [[nodiscard]] virtual int fd() const = 0;

        [[nodiscard]] virtual bool agent_valid() const = 0;

        [[nodiscard]] virtual uint32 can_receive() const = 0;

        [[nodiscard]] virtual uint32 can_send() const = 0;

        Event socket_event {};

        error_mark error {};

    };
}

#endif

#endif //NET_MESSAGEAGENT_HPP
