//
// Created by taganyer on 24-9-26.
//

#ifndef NET_UDP_COMMUNICATOR_HPP
#define NET_UDP_COMMUNICATOR_HPP

#ifdef NET_UDP_COMMUNICATOR_HPP

#include <utility>
#include "Socket.hpp"
#include "InetAddress.hpp"

namespace Base {
    struct TimeInterval;
}

namespace Net {

    class UDP_Communicator {
    public:
        using Message = std::pair<long, InetAddress>;

        explicit UDP_Communicator(const InetAddress &localAddress);

        UDP_Communicator(UDP_Communicator &&other) = default;

        [[nodiscard]] bool connect(const InetAddress &address) const;

        [[nodiscard]] bool set_timeout(Base::TimeInterval timeout) const;

        Message receive(void* buf, unsigned size, int flag = 0) const;

        long send(const void* buf, unsigned size, int flag = 0) const;

        long sendto(const InetAddress &address, const void* buf, unsigned size, int flag = 0) const;

        Socket& get_socket() { return _socket; };

    private:
        Socket _socket;

    };

}

#endif

#endif //NET_UDP_COMMUNICATOR_HPP
