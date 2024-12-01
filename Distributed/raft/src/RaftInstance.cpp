//
// Created by taganyer on 24-9-26.
//

#include "../RaftInstance.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Distributed/raft/RaftAgent.hpp"
#include "Distributed/raft/RaftMessage.hpp"


using namespace Dist;

using namespace Base;

using namespace Net;

TimeDifference RaftInstance::LEADER_SENT_TIMEOUT = 1_s;

TimeDifference RaftInstance::CANDIDATE_RECEIVE_TIMEOUT = 3_s;

TimeDifference RaftInstance::FOLLOWER_RECEIVE_TIMEOUT = 3_s;

TimeDifference RaftInstance::ONLINE_WAIT_TIMEOUT = 2_s;

TimeDifference RaftInstance::OFFLINE_WAIT_TIMEOUT = 2_s;

RaftInstance::RaftInstance(const InetAddress &addr,
                           RaftStateMachine* state_machine, std::string name) :
    _last_flush_time(Unix_to_now()),
    _transmitter(addr, state_machine, name.empty() ? addr.toIpPort() : std::move(name)) {}

void RaftInstance::add_peer(const InetAddress &peer) {
    assert(!running());
    if (unlikely(!_peers.emplace(peer, Unix_to_now()).second)) {
        G_ERROR << "RaftInstance add peer " << peer.toIpPort() << " failed";
    }
}

void RaftInstance::start() {
    assert(!running());
    _thread = Thread(std::string("Raft work Thread"), [this] {
        _running.store(true, std::memory_order_release);
        _state = Follower;
        while (running()) {
            switch (_state) {
            case Leader:
                leader_thread_loop();
                break;
            case Candidate:
                candidate_thread_loop();
                break;
            case Follower:
                follower_thread_loop();
                break;
            default:
                break;
            }
        }
        G_INFO << name_state() << " thread stopped";
    });
    _thread.start();
}

void RaftInstance::stop() {
    _running.store(false, std::memory_order_release);
    _thread.join();
}

bool RaftInstance::join_to_cluster() {
    assert(!running());
    int confirm_size = 0;

    for (int i = 0; i < 3 && confirm_size < _peers.size(); ++i) {
        for (auto &[addr, time] : _peers)
            if (time != 0) _transmitter.this_one_online(addr);
        _last_flush_time = Unix_to_now();
        while (_last_flush_time + ONLINE_WAIT_TIMEOUT > Unix_to_now()
            && confirm_size < _peers.size()) {
            RaftMessage message;
            Address address = _transmitter.receive(&message,
                                                   _last_flush_time + ONLINE_WAIT_TIMEOUT);
            if (RaftAgent::handle_message_when_online(*this, message, address))
                ++confirm_size;
        }
        if (_last_flush_time + ONLINE_WAIT_TIMEOUT < Unix_to_now()) {
            G_WARN << name_state() << ": RaftInstance join_to_cluster timeout";
        }
    }

    return RaftAgent::end_status_check(true, *this);
}

bool RaftInstance::exit_cluster() {
    stop();
    int confirm_size = 0;

    for (int i = 0; i < 3 && confirm_size < _peers.size(); ++i) {
        for (auto &[addr, time] : _peers)
            if (time != 0) _transmitter.this_one_offline(addr);
        _last_flush_time = Unix_to_now();
        while (_last_flush_time + OFFLINE_WAIT_TIMEOUT > Unix_to_now()
            && confirm_size < _peers.size()) {
            RaftMessage message;
            Address address = _transmitter.receive(&message,
                                                   _last_flush_time + OFFLINE_WAIT_TIMEOUT);
            if (RaftAgent::handle_message_when_offline(*this, message, address))
                ++confirm_size;
        }
        if (_last_flush_time + OFFLINE_WAIT_TIMEOUT < Unix_to_now()) {
            G_WARN << name_state() << ": RaftInstance join_to_cluster timeout";
        }
    }

    return RaftAgent::end_status_check(false, *this);
}

void RaftInstance::synchronize_logs_to_followers() {
    assert(running());
    Lock l(_mutex);
    if (_state == Leader) {
        _transmitter.machine->update_log();
        for (const auto &[address, _] : _peers)
            _transmitter.notify_follower(address,
                                         _transmitter.machine->leader_notify_message(address));
        _last_flush_time = Unix_to_now();
    }
}

std::string RaftInstance::name_state() const {
    std::string ret;
    ret.reserve(_transmitter._name.size() + 11);
    ret += _transmitter._name;
    ret += get_StateName(_state);
    return ret;
}

TimeDifference RaftInstance::follower_receive_timeout() const {
    TimeDifference hundredth { FOLLOWER_RECEIVE_TIMEOUT / 100 };
    return TimeDifference { _last_flush_time.nanoseconds
        + hundredth.nanoseconds * (_last_flush_time.nanoseconds % 25 + 76) };
}

void RaftInstance::leader_thread_loop() {
    RaftMessage message;
    Address address = _transmitter.receive(&message,
                                           _last_flush_time + LEADER_SENT_TIMEOUT);
    Lock l(_mutex);
    if (address != Address()) {
        RaftAgent::handle_messages_from_follower(*this, message, address);
    }
    if (_last_flush_time + LEADER_SENT_TIMEOUT < Unix_to_now()) {
        RaftAgent::handle_leader_timeout(*this);
    }
}

void RaftInstance::candidate_thread_loop() {
    RaftMessage message;
    Address address = _transmitter.receive(&message,
                                           _last_flush_time + CANDIDATE_RECEIVE_TIMEOUT);
    Lock l(_mutex);
    if (address != Address()) {
        RaftAgent::handle_messages_from_peer(*this, message, address);
    } else {
        RaftAgent::handle_candidate_timeout(*this);
    }
}

void RaftInstance::follower_thread_loop() {
    RaftMessage message;
    Address address = _transmitter.receive(&message,
                                           follower_receive_timeout());
    Lock l(_mutex);
    if (address != Address()) {
        if (address == _leader) RaftAgent::handle_messages_from_leader(*this, message);
        else RaftAgent::handle_messages_from_peer(*this, message, address);
    } else {
        RaftAgent::handle_follower_timeout(*this);
    }
}

const char* RaftInstance::get_StateName(State state) {
    constexpr const char* state_names[] = {
        "(Leader)",
        "(Candidate)",
        "(Follower)"
    };
    return state_names[state];
};
