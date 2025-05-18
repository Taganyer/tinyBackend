//
// Created by taganyer on 24-10-4.
//
#include "../Distributed_test.hpp"

#include <iostream>
#include <unistd.h>
#include <vector>
#include <tinyBackend/Base/GlobalObject.hpp>
#include <tinyBackend/Base/Thread.hpp>
#include <tinyBackend/Base/VersionNumber.hpp>
#include <tinyBackend/Base/Time/TimeInterval.hpp>
#include <tinyBackend/Distributed/raft/RaftInstance.hpp>
#include <tinyBackend/Distributed/raft/RaftMessage.hpp>
#include <tinyBackend/Distributed/raft/RaftStateMachine.hpp>
#include <tinyBackend/Net/InetAddress.hpp>

using namespace std;

using namespace Dist;

using namespace Test;

using namespace Net;

using namespace Base;

class RaftMachine final : public RaftStateMachine {
public:
    bool can_be_leader(const Address& sender_address, const RaftMessage& message) override {
        return true;
    };

    std::string leader_notify_message(const Address& follower_address) override {
        char arr[sizeof(VersionNumber64)] {};
        memcpy(arr, &version, sizeof(VersionNumber64));
        return arr;
    };

    bool need_synchronize_from(const Address& sender_address, const RaftMessage& message) override {
        VersionNumber64 v;
        memcpy(&v, message.extra_data, message.extra_data_size);
        return version < v;
    };

    std::string ready_to_receiving(const Address& sender_address, const RaftMessage& message) override {
        VersionNumber64 v;
        memcpy(&v, message.extra_data, message.extra_data_size);
        version = v;
        return "Respond Leader: " + sender_address.toIpPort();
    };

    void ready_to_send_to(const Address& receiver_address, const RaftMessage& message) override {
        cout << __FUNCTION__ << ": " << receiver_address.toIpPort()
            << " " << message.extra_data << endl;
    };

    void update_log() override {
        ++version;
        cout << "Leader update log version: " << version.data() << endl;
    };

private:
    VersionNumber64 version;

};

void Test::raft_test() {
    cout << "Testing Raft Test" << endl;
    constexpr int nodes_size = 5;
    vector<InetAddress> addresses;
    vector<RaftMachine> machines;
    vector<Thread> threads;

    for (int i = 0; i < nodes_size; ++i) {
        addresses.emplace_back(true, "127.0.0.1", 7000 + i);
        machines.emplace_back();
        G_INFO << addresses[i].toIpPort();
    }

    threads.reserve(nodes_size);
    for (int i = 0; i < nodes_size; ++i) {
        threads.emplace_back([i, &addresses, &machines] {
            RaftInstance instance(addresses[i], &machines[i]);
            for (int j = 0; j < nodes_size - 1; ++j) {
                if (i == j) continue;
                instance.add_peer(addresses[j]);
            }
            if (i == nodes_size - 1)
                instance.join_to_cluster();
            instance.start();

            sleep(3);
            for (int j = 0; j <= i * 2; ++j) {
                instance.synchronize_logs_to_followers();
                sleep(2);
            }
            instance.exit_cluster();
        });
    }

    TimeInterval begin = Unix_to_now();
    for (int i = 0; i < nodes_size; ++i) {
        threads[i].start();
    }

    for (int i = 0; i < nodes_size; ++i) {
        threads[i].join();
    }
    TimeInterval end = Unix_to_now();
    cout << "Raft Test completed in " << (end - begin).to_ms() << "ms" << endl;
}
