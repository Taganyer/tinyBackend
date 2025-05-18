//
// Created by taganyer on 24-5-21.
//

#include "../net_test.hpp"

#include <iostream>

#include <tinyBackend/Base/Condition.hpp>
#include <tinyBackend/Base/File.hpp>
#include <tinyBackend/Base/GlobalObject.hpp>
#include <tinyBackend/Base/Thread.hpp>
#include <tinyBackend/Net/Acceptor.hpp>
#include <tinyBackend/Net/Channel.hpp>
#include <tinyBackend/Net/InetAddress.hpp>
#include <tinyBackend/Net/TcpMessageAgent.hpp>
#include <tinyBackend/Net/UDP_Communicator.hpp>
#include <tinyBackend/Net/error/errors.hpp>
#include <tinyBackend/Net/functions/Interface.hpp>
#include <tinyBackend/Net/monitors/Event.hpp>
#include <tinyBackend/Net/reactor/Reactor.hpp>

using namespace Net;

using namespace Base;

using namespace std;


static void server_read(MessageAgent& agent) {
    auto& input = agent.input();
    auto& output = agent.output();
    uint32 written = output.write(input, input.readable_len());
    assert(written > 0);
    assert(input.readable_len() == 0);
    agent.send_message();
}

static void server_close(std::atomic<int>& count, Condition& con) {
    if (count.fetch_sub(1) == 1) {
        con.notify_one();
        cout << "server close" << endl;
    }
}

static void echo_server(Acceptor& acceptor, int client_size, Condition& con, atomic<int>& count) {
    Mutex mutex;

    Reactor reactor(1_min);
    reactor.start(Reactor::EPOLL, 1000);
    cout << "common Reactor." << endl;

    for (int i = 0; i < client_size; ++i) {
        auto [socket, address] = acceptor.accept_connection();

        bool success = socket.setReuseAddr(true);
        assert(success);

        auto agent_ptr = std::make_unique<TcpMessageAgent>(std::move(socket), 1024, 1024);
        Channel channel;

        channel.set_readCallback([] (MessageAgent& agent) mutable {
            server_read(agent);
        });

        channel.set_errorCallback([] (MessageAgent& agent) {
            G_FATAL << '[' << get_error_type_name(agent.error.types) << ']';
            return true;
        });

        channel.set_closeCallback([&count, &con] (MessageAgent&) mutable {
            server_close(count, con);
        });

        Event event { agent_ptr->fd() };
        event.set_read();
        reactor.add_channel(std::move(agent_ptr), std::move(channel), event);
    }

    cout << "accept end." << endl;
    Lock l(mutex);
    con.wait(l);
    cout << "wait end" << endl;
}

static void echo_client(InetAddress& server_address, Condition& con, atomic<int>& count) {
    Socket client_socket(AF_INET, SOCK_STREAM);
    bool success = client_socket.connect(server_address);
    assert(success);
    G_INFO << "client fd: " << client_socket.fd();

    iFile in("", true);

    while (in) {
        auto line = in.getline();
        if (line.empty()) continue;

        auto len = ops::write(client_socket.fd(), line.c_str(), line.size());
        if (len < 0) cout << ops::get_write_error(errno) << endl;

        int64 size = 0;
        char buffer[256] {};
        while (len > size) {
            auto t = ops::read(client_socket.fd(), buffer + size, 256 - size);
            if (t < 0) {
                cout << CurrentThread::thread_name() << ops::get_read_error(errno);
                break;
            }
            size += t;
        }
        buffer[len] = '\0';
        G_INFO << buffer << ' ' << CurrentThread::thread_name();
    }
    if (count.fetch_sub(1) == 1) {
        con.notify_one();
    }
}

void Test::echo() {
    InetAddress server_address(true, "127.0.0.1", 8888);
    Acceptor acceptor(true, server_address.port());

    // Global_Logger.set_rank(LogSystem::WARN);

    int cpu_core_to_client = 10, multiple = 100;
    int client_size = cpu_core_to_client * multiple;

    Condition con;
    atomic<int> count = client_size * 2;

    for (int i = 0; i < cpu_core_to_client; ++i) {
        string name("Client core ");
        name += to_string(i);
        Thread thread(name, [&server_address, &count, &con, multiple] {
            for (int j = 0; j < multiple; ++j) {
                echo_client(server_address, con, count);
            }
        });
        thread.start();
    }

    auto time = chronograph(echo_server, acceptor, client_size, con, count);
    G_FATAL << "================================ total: "
            << cpu_core_to_client * multiple << " clients cost " << time.to_ms()
            << "ms ===============================";
    cout << "total: " << cpu_core_to_client * multiple << " clients cost " << time.to_ms() << "ms" << endl;
}

void Test::InetAddress_test() {
    // InetAddress host = InetAddress::getLocalHost();
    // cout << host.toIpPort() << endl;
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8888);
    InetAddress address4(address);
    cout << address4.toIpPort() << endl;
    sockaddr_in6 address6;
    address6.sin6_family = AF_INET6;
    address6.sin6_addr = in6addr_any;
    address6.sin6_port = htons(8888);

    InetAddress address6a(address6);
    cout << address6a.toIpPort() << endl;
}

void Test::UDP_test() {
    // InetAddress add1(true, "127.0.0.1"), add2(true, "127.0.0.1");
    InetAddress add1(false, "::1"), add2(false, "::1");

    Thread client([&add1, &add2] {
        UDP_Communicator cl(add2);
        add2 = InetAddress::get_InetAddress(cl.get_socket().fd());
        cout << add2.toIpPort() << endl;
        sleep(3);
        char buf[256] = "hello server!";
        char buf2[256] {};
        for (int i = 0; i < 10; ++i) {
            auto [get, addr] = cl.receive(buf2, 256);
            assert(add1 == addr);
            cout << "client get: " << get << " " << buf2 << endl;
            buf2[0] = '\0';
            long sent = cl.sendto(add1, buf, strlen(buf) + 1);
            cout << "client sent: " << sent << endl;
        }
    });
    client.start();
    sleep(1);

    cout << "server start" << endl;
    UDP_Communicator server(add1);
    add1 = InetAddress::get_InetAddress(server.get_socket().fd());
    cout << add1.toIpPort() << endl;
    char buffer[256] = "hello client!";
    char buffer2[256] {};
    for (int i = 0; i < 10; ++i) {
        long sent = server.sendto(add2, buffer, strlen(buffer) + 1);
        cout << "server sent: " << sent << endl;
        auto [get, addr] = server.receive(buffer2, 256);
        assert(add2 == addr);
        cout << "server get: " << get << " " << buffer2 << endl;
        buffer2[0] = '\0';
    }
    client.join();
}

void Test::TCP_test() {
    InetAddress add(true, "127.0.0.1", 8888), add2(true, "127.0.0.1", 8889);
    Thread client([&add2, add] {
        Socket client_socket(AF_INET, SOCK_STREAM);
        bool success = client_socket.bind(add2);
        assert(success);
        TimeInterval begin = Unix_to_now();
        sleep(1);
        success = client_socket.connect(add);
        TimeInterval end = Unix_to_now();
        cout << "client connect: " << (end - begin).to_ms() << "ms" << endl;
        assert(success);
        char msg[] = "hello server!";
        success = ops::write(client_socket.fd(), msg, sizeof(msg)) == sizeof(msg);
        assert(success);
        begin = Unix_to_now();
        client_socket.close();
        end = Unix_to_now();
        cout << "client close: " << (end - begin).to_ms() << "ms" << endl;
    });
    Socket server_socket(AF_INET, SOCK_STREAM);
    bool success = server_socket.bind(add);
    assert(success);
    success = server_socket.tcpListen(5);
    assert(success);
    client.start();
    sleep(1);
    TimeInterval begin = Unix_to_now();
    auto sock = server_socket.tcpAccept(add);
    TimeInterval end = Unix_to_now();
    cout << "server accept: " << (end - begin).to_ms() << "ms" << endl;
    assert(sock.valid());
    sleep(2);
    char msg[64] {};
    int64 size = ops::read(sock.fd(), msg, 64);
    assert(size > 0);
    cout << "server recv: " << msg << endl;
    sleep(1);
    sock.close();
    client.join();
    unordered_map<string, string> map;
}
