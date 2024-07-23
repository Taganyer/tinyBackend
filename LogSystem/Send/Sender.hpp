//
// Created by taganyer on 3/30/24.
//

#ifndef BASE_SENDER_HPP
#define BASE_SENDER_HPP

#include <memory>
#include "Base/Detail/oFile.hpp"

namespace LogSystem {

    class Sender : public std::enable_shared_from_this<Sender> {
    public:

        using SenderPtr = std::shared_ptr<Sender>;

        using SenderWeak = std::weak_ptr<Sender>;

        virtual ~Sender() = default;

        virtual void send(const void *buffer, uint64 size) = 0;

        virtual void force_flush() = 0;

        friend class SendThread;

    };

}

#endif //BASE_SENDER_HPP
