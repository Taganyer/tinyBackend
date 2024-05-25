//
// Created by taganyer on 3/14/24.
//

#ifndef NET_ACCEPTOR_HPP
#define NET_ACCEPTOR_HPP

#ifdef NET_ACCEPTOR_HPP

#include <map>

#include "Socket.hpp"
#include "NetLink.hpp"
#include "Base/Mutex.hpp"

namespace Net {

    struct error_mark;

    class Acceptor : private Base::NoCopy {
    public:

        static int ListenMax;

        using Message = std::pair<NetLink::LinkPtr, InetAddress>;

        explicit Acceptor(Socket &&socket);

        ~Acceptor();

        Message accept_connection(bool NoDelay);

        void remove_Link(int fd);

    private:

        using Links = std::map<int, NetLink::LinkPtr>;

        Socket _socket;

        Base::Mutex _mutex;

        Links _links;

    };

}

#endif

#endif //NET_ACCEPTOR_HPP
