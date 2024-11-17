//
// Created by taganyer on 24-9-26.
//

#include "../UDP_Communicator.hpp"
#include "Net/error/errors.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Net/functions/Interface.hpp"

using namespace Base;

using namespace Net;

UDP_Communicator::UDP_Communicator(InetAddress localAddress):
    _socket(localAddress.is_IPv4() ? AF_INET : AF_INET6, SOCK_DGRAM, 0) {
    if (!_socket.bind(localAddress)) {
        G_ERROR << "UDP_Acceptor " << _socket.fd() << " bind failed."
                << " error: " << ops::get_bind_error(errno);
    }
}

bool UDP_Communicator::set_timeout(Time_difference timeout) {
    timeval tv = timeout.to_timeval();
    return ops::set_socket_opt(_socket.fd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

UDP_Communicator::Message UDP_Communicator::receive(void* buf, unsigned size, int flag) {
    InetAddress client_address {};
    socklen_t addr_len = sizeof(client_address);
    long n = recvfrom(_socket.fd(), buf, size, flag,
                      (struct sockaddr *) &client_address, &addr_len);
    return { n, client_address };
}

long UDP_Communicator::send(const InetAddress &address, const void* buf, unsigned size, int flag) {
    if (address.is_IPv4())
        return sendto(_socket.fd(), buf, size, flag, (const sockaddr *) &address, sizeof(sockaddr_in));
    return sendto(_socket.fd(), buf, size, flag, (const sockaddr *) &address, sizeof(sockaddr_in6));
}
