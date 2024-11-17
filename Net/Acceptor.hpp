//
// Created by taganyer on 3/14/24.
//

#ifndef NET_ACCEPTOR_HPP
#define NET_ACCEPTOR_HPP

#ifdef NET_ACCEPTOR_HPP

#include "NetLink.hpp"

namespace Net {

    struct error_mark;

    class Acceptor : Base::NoCopy {
    public:
        static int ListenMax;

        using Message = std::pair<NetLink::LinkPtr, InetAddress>;

        explicit Acceptor(Socket &&socket);

        ~Acceptor();

        [[nodiscard]] Message accept_connection() const;

    private:
        Socket _socket;

    };

}

#endif

#endif //NET_ACCEPTOR_HPP
