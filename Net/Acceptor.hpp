//
// Created by taganyer on 3/14/24.
//

#ifndef NET_ACCEPTOR_HPP
#define NET_ACCEPTOR_HPP

#ifdef NET_ACCEPTOR_HPP

#include <utility>
#include "Socket.hpp"

namespace Net {

    struct error_mark;

    class Acceptor : Base::NoCopy {
    public:
        static int ListenMax;

        using Message = std::pair<Socket, InetAddress>;

        explicit Acceptor(bool Ipv4, unsigned short target_port, const char *target_ip = nullptr);

        explicit Acceptor(const InetAddress& target);

        explicit Acceptor(Socket&& socket);

        Acceptor(Acceptor&& other) noexcept : _socket(std::move(other._socket)) {};

        ~Acceptor();

        void close();

        [[nodiscard]] Message accept_connection() const;

        [[nodiscard]] const Socket& socket() const { return _socket; };

    private:
        Socket _socket;

    };

}

#endif

#endif //NET_ACCEPTOR_HPP
