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
    struct Time_difference;
}

namespace Net {

    class UDP_Communicator {
    public:
        using Message = std::pair<long, InetAddress>;

        explicit UDP_Communicator(InetAddress localAddress);

        UDP_Communicator(UDP_Communicator &&other) = default;

        bool set_timeout(Base::Time_difference timeout);

        Message receive(void* buf, unsigned size, int flag = 0);

        long send(const InetAddress &address, const void* buf, unsigned size, int flag = 0);

        Socket& get_socket() { return _socket; };

    private:
        Socket _socket;

    };

}

#endif

#endif //NET_UDP_COMMUNICATOR_HPP
