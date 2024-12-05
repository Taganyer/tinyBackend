//
// Created by taganyer on 3/14/24.
//

#ifndef NET_ACCEPTOR_HPP
#define NET_ACCEPTOR_HPP

#ifdef NET_ACCEPTOR_HPP

#include "Socket.hpp"

namespace Net {

    struct error_mark;

    class Acceptor : Base::NoCopy {
    public:
        static int ListenMax;

        using Message = std::pair<Socket, InetAddress>;

        explicit Acceptor(bool Ipv4, unsigned short target_port, const char *target_ip = nullptr);

        Acceptor(bool Ipv4, const InetAddress &target);

        explicit Acceptor(Socket &&socket);

        ~Acceptor();

        [[nodiscard]] Message accept_connection() const;

    private:
        Socket _socket;

    };

}

#endif

#endif //NET_ACCEPTOR_HPP
