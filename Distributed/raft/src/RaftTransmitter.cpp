//
// Created by taganyer on 24-9-28.
//

#include "../RaftTransmitter.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Distributed/raft/RaftMessage.hpp"

using namespace Dist;

using namespace Base;

using namespace Net;

static constexpr long MESSAGE_SIZE = sizeof(RaftMessage);

#define FILLING_PARAMETER version, serial_number++

RaftTransmitter::RaftTransmitter(const Address &local_address,
                                 RaftStateMachine* machine, std::string name) :
    _sender(local_address), machine(machine), _name(std::move(name)) {}

void RaftTransmitter::notify_follower(const Address &follower, const std::string &data) {
    RaftMessage message;
    message.leader_check(FILLING_PARAMETER);
    if (!message.add_extra_message(data.data(), data.size())) {
        G_ERROR << _name << ": RaftTransmitter::probe_follower "
                << "add_extra_message failed";
        return;
    }
    if (_sender.send(follower, &message, MESSAGE_SIZE) != MESSAGE_SIZE) {
        G_ERROR << _name << ": RaftTransmitter::probe_follower "
                << follower.toIpPort() << " send failed";
    }
}

void RaftTransmitter::respond_leader(const Address &leader) {
    RaftMessage message;
    message.follower_ack(FILLING_PARAMETER);
    if (_sender.send(leader, &message, MESSAGE_SIZE) != MESSAGE_SIZE) {
        G_ERROR << _name << ": RaftTransmitter::respond_leader "
                << leader.toIpPort() << " send failed";
    }
}

void RaftTransmitter::request_vote(const Address &peer) {
    RaftMessage message;
    message.ask_for_vote(FILLING_PARAMETER);
    if (_sender.send(peer, &message, MESSAGE_SIZE) != MESSAGE_SIZE) {
        G_ERROR << _name << ": RaftTransmitter::request_vote "
                << peer.toIpPort() << " send failed";
    }
}

void RaftTransmitter::vote_to(const Address &peer, bool result) {
    RaftMessage message;
    message.vote(FILLING_PARAMETER, result);
    if (_sender.send(peer, &message, MESSAGE_SIZE) != MESSAGE_SIZE) {
        G_ERROR << _name << ": RaftTransmitter::vote_to "
                << peer.toIpPort() << " send failed";
    }
}

void RaftTransmitter::request_logs(const Address &address, const std::string &data) {
    RaftMessage message;
    message.request_logs(FILLING_PARAMETER);
    if (!message.add_extra_message(data.data(), data.size())) {
        G_ERROR << _name << ": RaftTransmitter::request_logs "
                << "add_extra_message failed";
        return;
    }
    if (_sender.send(address, &message, MESSAGE_SIZE) != MESSAGE_SIZE) {
        G_ERROR << _name << ": RaftTransmitter::request_logs "
                << address.toIpPort() << " send failed";
    }
}

void RaftTransmitter::this_one_online(const Address &peer) {
    RaftMessage message;
    message.online(FILLING_PARAMETER);
    if (_sender.send(peer, &message, MESSAGE_SIZE) != MESSAGE_SIZE) {
        G_ERROR << _name << ": RaftTransmitter::this_one_online "
                << peer.toIpPort() << " send failed";
    }
}

void RaftTransmitter::affirm_online(const Address &address) {
    RaftMessage message;
    message.affirm_online(FILLING_PARAMETER);
    if (_sender.send(address, &message, MESSAGE_SIZE) != MESSAGE_SIZE) {
        G_ERROR << _name << ": RaftTransmitter::affirm_online sent to"
                << address.toIpPort() << " failed";
    }
}

void RaftTransmitter::this_one_offline(const Address &peer) {
    RaftMessage message;
    message.offline(FILLING_PARAMETER);
    if (_sender.send(peer, &message, MESSAGE_SIZE) != MESSAGE_SIZE) {
        G_ERROR << _name << ": RaftTransmitter::this_one_offline "
                << peer.toIpPort() << " send failed";
    }
}

void RaftTransmitter::affirm_offline(const Address &address) {
    RaftMessage message;
    message.affirm_offline(FILLING_PARAMETER);
    if (_sender.send(address, &message, MESSAGE_SIZE) != MESSAGE_SIZE) {
        G_ERROR << _name << ": RaftTransmitter::affirm_offline sent to"
                << address.toIpPort() << " failed";
    }
}

RaftTransmitter::Address RaftTransmitter::receive(RaftMessage* message,
                                                  Time_difference end_time) {
    auto time = end_time - Unix_to_now();
    int choose = MSG_DONTWAIT;
    if (time > 0) {
        choose = MSG_WAITALL;
        _sender.set_timeout(time);
    }
    auto [len, addr] = _sender.receive(message, MESSAGE_SIZE, choose);
    if (len != MESSAGE_SIZE) return {};
    return addr;
}
