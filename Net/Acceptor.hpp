//
// Created by taganyer on 3/14/24.
//

#ifndef NET_ACCEPTOR_HPP
#define NET_ACCEPTOR_HPP

#ifdef NET_ACCEPTOR_HPP

#include <map>
#include <memory>

#include "Socket.hpp"
#include "NetLink.hpp"
#include "Base/Mutex.hpp"

namespace Net {

    struct error_mark;

    class Acceptor : public std::enable_shared_from_this<Acceptor>, private Base::NoCopy {
    public:

        static int ListenMax;

        using Message = std::pair<NetLink::LinkPtr, InetAddress>;

        using AcceptorPtr = std::shared_ptr<Acceptor>;

        using WeakPtr = std::weak_ptr<Acceptor>;

        static AcceptorPtr create_AcceptorPtr(Socket &&socket);

        ~Acceptor();

        Message accept_connection(bool NoDelay);

        bool add_LinkPtr(NetLink::LinkPtr link_ptr);

        NetLink::LinkPtr get_LinkPtr(int fd);

        void remove_Link(int fd);

    private:

        using Links = std::map<int, NetLink::LinkPtr>;

        Socket _socket;

        Base::Mutex _mutex;

        Links _links;

        explicit Acceptor(Socket &&socket);

    };

}

#endif

#endif //NET_ACCEPTOR_HPP
