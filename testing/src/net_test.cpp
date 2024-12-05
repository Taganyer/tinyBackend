//
// Created by taganyer on 24-5-21.
//

#include "testing/net_test.hpp"

#include <iostream>

#include "Net/Acceptor.hpp"
#include "Net/Controller.hpp"
#include "Net/InetAddress.hpp"
#include "Net/monitors/Event.hpp"

#include "Base/File.hpp"
#include "Net/error/errors.hpp"
#include "Net/functions/Interface.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Net/UDP_Communicator.hpp"
#include "Net/reactor/Reactor.hpp"

using namespace Net;

using namespace Base;

using namespace std;


static void server_read(const Controller &con, RingBuffer &buffer) {
    uint32 written = con.send(buffer.readable_array());
    assert(written > 0);
    buffer.read_advance(buffer.readable_len());
    assert(buffer.readable_len() == 0);
}

static void server_close(std::atomic<int> &count, Condition &con) {
    if (count.fetch_sub(1) == 1) {
        con.notify_one();
        cout << "server close" << endl;
    }
}

static void echo_server(Acceptor &acceptor, int client_size, Condition &con, atomic<int> &count) {
    Mutex mutex;

    Reactor reactor(1_min);
    reactor.start(Reactor::EPOLL, 1000);
    cout << "common Reactor." << endl;

    for (int i = 0; i < client_size; ++i) {
        auto [socket, address] = acceptor.accept_connection();

        bool success = socket.setReuseAddr(true);
        assert(success);

        auto link = NetLink::create_NetLinkPtr(std::move(socket));

        link->set_readCallback([] (const Controller &controller) mutable {
            server_read(controller, controller.input_buffer());
        });

        link->set_errorCallback([] (error_mark error, const Controller &controller) {
            G_FATAL << '[' << get_error_type_name(error.types) << ']';
            return true;
        });

        link->set_closeCallback([&count, &con] (const Controller &controller) mutable {
            server_close(count, con);
        });

        Event event { link->fd() };
        event.set_read();
        // event.set_write();
        reactor.add_NetLink(link, event);
    }

    cout << "accept end." << endl;
    Lock l(mutex);
    con.wait(l);
    cout << "wait end" << endl;
}

static void echo_client(InetAddress &server_address, Condition &con, atomic<int> &count) {
    Socket client_socket(AF_INET, SOCK_STREAM);
    bool success = client_socket.tcpConnect(server_address);
    assert(success);
    G_INFO << "client fd: " << client_socket.fd();

    iFile in("/home/taganyer/Code/Clion_project/test/resource/poem.txt", true);

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

void Test::UDP_test() {
#ifdef USE_IPV4
    InetAddress add1(true, "127.0.0.1"), add2(true, "127.0.0.1");
#else
    InetAddress add1(false, "::1"), add2(false, "::1");
#endif
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
            long sent = cl.send(add1, buf, strlen(buf) + 1);
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
        long sent = server.send(add2, buffer, strlen(buffer) + 1);
        cout << "server sent: " << sent << endl;
        auto [get, addr] = server.receive(buffer2, 256);
        assert(add2 == addr);
        cout << "server get: " << get << " " << buffer2 << endl;
        buffer2[0] = '\0';
    }
    client.join();
}
