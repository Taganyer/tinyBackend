//
// Created by taganyer on 3/14/24.
//

#ifndef NET_ACCEPTOR_HPP
#define NET_ACCEPTOR_HPP


#include <functional>
#include "Socket.hpp"
#include "../Base/Detail/config.hpp"

namespace Net {

    struct error_mark;

    class Acceptor {
    public:

        static int ListenMax;

        using AcceptCallback = std::function<void(int, InetAddress)>;

        explicit Acceptor(Socket &&socket);

        void set_acceptCallback(AcceptCallback callback) { _accept = std::move(callback); };

        bool start_listen();

        int64 accept_connections(int64 times);

        void Async_stop_accept();

        [[nodiscard]] error_mark get_error() const;

        [[nodiscard]] bool accepting() const { return _state == Accepting; };

    private:

        static const int Accepting;

        static const int Ready;

        AcceptCallback _accept;

        Socket _socket;

        int _state = Ready;

        bool accept();

    };

}

#endif //NET_ACCEPTOR_HPP
