//
// Created by taganyer on 24-9-29.
//

#include "../RaftAgent.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Distributed/raft/RaftMessage.hpp"
#include "Distributed/raft/RaftInstance.hpp"


using namespace Dist;

using namespace Base;

using namespace Net;

#define STATE_MECHINE instance._transmitter.machine

void RaftAgent::handle_messages_from_follower(RaftInstance &instance,
                                              const RaftMessage &message, const Address &address) {
    assert(instance._state == RaftInstance::Leader);
    if (instance._peers.find(address) != instance._peers.end()) {
        Time_difference now = Unix_to_now();
        G_INFO << instance.name_state() << ": Accepting follower " << address.toIpPort()
                << " response message delay: "
                << (now - instance._peers[address]).to_ms() << " ms.";
        instance._peers[address] = now;
    } else {
        assert(message.type == RaftMessage::Online
            || message.type == RaftMessage::Offline);
    }

    switch (message.type) {
    case RaftMessage::FollowerAck:
        break;
    case RaftMessage::Request:
        synchronize_logs(instance, message, address);
        break;
    case RaftMessage::AskForVote:
        vote_to(instance, message, address);
        break;
    case RaftMessage::Vote:
        accept_vote(instance, message, address);
        break;
    case RaftMessage::Online:
        new_one_online(instance, address);
        break;
    case RaftMessage::Offline:
        peer_offline(instance, address);
        break;
    case RaftMessage::LeaderCheck:
        if (message.version < instance._transmitter.version
            && STATE_MECHINE->can_be_leader(address, message)) {
            instance._transmitter.notify_follower(address,
                                                  STATE_MECHINE->leader_notify_message(address));
        } else {
            change_leader_to(instance, message, address);
            respond_leader_check(instance, message);
        }
        break;
    default:
        error_type(__FUNCTION__, instance, message, address);
        break;
    }
}

void RaftAgent::handle_messages_from_leader(RaftInstance &instance, const RaftMessage &message) {
    assert(instance._state != RaftInstance::Leader);
    if (check_outdated(instance, message, instance._leader))
        return;
    assert(instance._peers.find(instance._leader) != instance._peers.end());
    Time_difference now = Unix_to_now();
    G_INFO << instance.name_state() << ": Accepting leader " << instance._leader.toIpPort()
            << " response message delay: "
            << (now - instance._peers[instance._leader]).to_ms() << " ms.";
    instance._peers[instance._leader] = now;
    instance._last_flush_time = now;

    switch (message.type) {
    case RaftMessage::LeaderCheck:
        respond_leader_check(instance, message);
        break;
    case RaftMessage::AskForVote:
        vote_to(instance, message, instance._leader);
        break;
    case RaftMessage::Offline:
        peer_offline(instance, instance._leader);
        break;
    default:
        error_type(__FUNCTION__, instance, message, instance._leader);
        break;
    }
}

void RaftAgent::handle_messages_from_peer(RaftInstance &instance,
                                          const RaftMessage &message, const Address &address) {
    assert(instance._leader != address);
    if (check_outdated(instance, message, address))
        return;
    if (instance._peers.find(address) != instance._peers.end()) {
        Time_difference now = Unix_to_now();
        G_INFO << instance.name_state() << ": Accepting peer " << address.toIpPort()
                << " response message delay: "
                << (now - instance._peers[address]).to_ms() << " ms.";
        instance._peers[address] = now;
    } else {
        assert(message.type == RaftMessage::Online
            || message.type == RaftMessage::Offline);
    }

    switch (message.type) {
    case RaftMessage::AskForVote:
        vote_to(instance, message, address);
        break;
    case RaftMessage::Vote:
        accept_vote(instance, message, address);
        break;
    case RaftMessage::LeaderCheck:
        change_leader_to(instance, message, address);
        respond_leader_check(instance, message);
        break;
    case RaftMessage::Online:
        new_one_online(instance, address);
        break;
    case RaftMessage::Offline:
        peer_offline(instance, address);
        break;
    default:
        error_type(__FUNCTION__, instance, message, address);
        break;
    }
}

void RaftAgent::handle_leader_timeout(RaftInstance &instance) {
    for (const auto &[address, _] : instance._peers)
        instance._transmitter.notify_follower(address,
                                              STATE_MECHINE->leader_notify_message(address));
    instance._last_flush_time = Unix_to_now();
}

void RaftAgent::handle_candidate_timeout(RaftInstance &instance) {
    G_WARN << instance.name_state() << ": Candidate timeout.";
    vote_to_be_leader(instance);
}

void RaftAgent::handle_follower_timeout(RaftInstance &instance) {
    G_WARN << instance.name_state() << ": Leader " << instance._leader.toIpPort() << " timeout.";
    vote_to_be_leader(instance);
}

bool RaftAgent::handle_message_when_online(RaftInstance &instance,
                                           const RaftMessage &message, const Address &address) {
    switch (message.type) {
    case RaftMessage::OnlineAck:
        return accept_online_ack(instance, address);
    case RaftMessage::LeaderCheck:
        change_leader_to(instance, message, address);
        respond_leader_check(instance, message);
        break;
    case RaftMessage::AskForVote:
        vote_to(instance, message, address);
        break;
    case RaftMessage::Online:
        new_one_online(instance, address);
        instance._transmitter.this_one_online(address);
        break;
    case RaftMessage::Offline:
        instance._transmitter.this_one_online(address);
        peer_offline(instance, address);
        break;
    default:
        error_type(__FUNCTION__, instance, message, address);
        break;
    }
    return false;
}

bool RaftAgent::handle_message_when_offline(RaftInstance &instance,
                                            const RaftMessage &message, const Address &address) {
    switch (message.type) {
    case RaftMessage::OfflineAck:
        return accept_offline_ack(instance, address);
    case RaftMessage::Online:
        instance._transmitter.this_one_offline(address);
        new_one_online(instance, address);
        break;
    case RaftMessage::Offline:
        peer_offline(instance, address);
        instance._transmitter.this_one_offline(address);
        break;
    case RaftMessage::FollowerAck:
        if (instance._state == RaftInstance::Leader)
            break;
    default:
        error_type(__FUNCTION__, instance, message, address);
        break;
    }
    return false;
}

bool RaftAgent::end_status_check(bool online, RaftInstance &instance) {
    Time_difference now = Unix_to_now();
    bool success = true;
    for (auto &[addr, time] : instance._peers) {
        if (time != 0) {
            success = false;
            G_FATAL << instance.name_state() << (online ? ": Online" : ": Offline")
                    << " did not get confirm from " << addr.toIpPort();
            continue;
        }
        time = now;
    }
    G_INFO << instance.name_state() << (online ? ": Online" : ": Offline")
                    << (success ? " get all ack from peers." : " not get all ack from peers.");
    return success;
}

void RaftAgent::respond_leader_check(RaftInstance &instance, const RaftMessage &message) {
    assert(instance._state == RaftInstance::Follower);
    instance._transmitter.version = message.version;
    G_INFO << instance.name_state() << ": Get leader check message.";

    bool choose = STATE_MECHINE->need_synchronize_from(instance._leader, message);
    if (!choose) {
        instance._transmitter.respond_leader(instance._leader);
        return;
    }

    auto extra_data = STATE_MECHINE->ready_to_receiving(instance._leader, message);
    instance._transmitter.request_logs(instance._leader, extra_data);
    G_INFO << instance.name_state() << ": Want to synchronize leader logs: " << message.version.data();
}

void RaftAgent::vote_to_be_leader(RaftInstance &instance) {
    become_candidate(instance);
    G_INFO << instance.name_state() << ": Ask for votes";
    for (const auto &[address, _] : instance._peers)
        instance._transmitter.request_vote(address);
}

void RaftAgent::vote_to(RaftInstance &instance, const RaftMessage &message, const Address &address) {
    bool can_be_leader = true;
    if (message.version > instance._transmitter.version
        && ((can_be_leader = STATE_MECHINE->can_be_leader(address, message)))) {
        change_leader_to(instance, message, address);
        G_INFO << instance.name_state() << ": Agree to vote for " << address.toIpPort();
        instance._transmitter.vote_to(address, true);
    } else {
        if (!can_be_leader) {
            change_leader_to(instance, message, Address());
            G_WARN << instance.name_state() << ": Disagree to vote for" << address.toIpPort()
                    << " because RaftStateMachine check failed.";
        } else if (unlikely(instance._state == RaftInstance::Leader)) {
            G_WARN << instance.name_state() << ": " << address.toIpPort()
                    << " want to be leader but leader is not offline.";
        } else {
            G_INFO << instance.name_state() << ": Disagree to vote for " << address.toIpPort();
        }
        instance._transmitter.vote_to(address, false);
    }
}

void RaftAgent::accept_vote(RaftInstance &instance, const RaftMessage &message, const Address &address) {
    if (instance._state != RaftInstance::Candidate) {
        G_INFO << instance.name_state() << ": Get vote from " << address.toIpPort()
                << " but Instance has been become a " << RaftInstance::get_StateName(instance._state);
        return;
    }
    if (!message.choose) {
        G_INFO << instance.name_state() << ": Didn't get vote from " << address.toIpPort();
        return;
    }
    G_INFO << instance.name_state() << ": Get vote from " << address.toIpPort();
    if (++instance.votes >= (instance._peers.size() + 1) / 2 + 1) {
        become_leader(instance);
        for (const auto &[address, _] : instance._peers)
            instance._transmitter.notify_follower(address,
                                                  STATE_MECHINE->leader_notify_message(address));
    }
}

void RaftAgent::synchronize_logs(RaftInstance &instance,
                                 const RaftMessage &message, const Address &address) {
    STATE_MECHINE->ready_to_send_to(address, message);
    G_INFO << instance.name_state() << ": Leader synchronize "
                << address.toIpPort() << " logs.";
}

void RaftAgent::new_one_online(RaftInstance &instance, const Address &address) {
    if (likely(instance._peers.try_emplace(address, Unix_to_now()).second)) {
        G_INFO << instance.name_state() << ": New one online: " << address.toIpPort();
    } else {
        G_WARN << instance.name_state() << ": " << address.toIpPort() << " already exists.";
    }
    if (instance._state == RaftInstance::Leader) {
        instance._transmitter.notify_follower(address,
                                              STATE_MECHINE->leader_notify_message(address));
    } else {
        instance._transmitter.affirm_online(address);
    }
}

bool RaftAgent::accept_online_ack(RaftInstance &instance, const Address &address) {
    auto iter = instance._peers.find(address);
    if (unlikely(iter == instance._peers.end())) {
        G_WARN << instance.name_state()
                << ": Receive unknown OnlineAck from " << address.toIpPort();
        return false;
    }
    if (iter->second != 0) {
        iter->second = 0;
        return true;
    }
    return false;
}

void RaftAgent::peer_offline(RaftInstance &instance, const Address &address) {
    auto iter = instance._peers.find(address);
    if (iter == instance._peers.end()) {
        G_WARN << instance.name_state() << ": Peer " << address.toIpPort()
                << " get message timeout.";
    } else {
        instance._peers.erase(iter);
        G_INFO << instance.name_state() << ": Remove " << address.toIpPort();
    }

    instance._transmitter.affirm_offline(address);

    if (address == instance._leader) {
        G_WARN << instance.name_state() << ": Leader " << address.toIpPort() << " offline.";
        instance._leader = {};
    }
}

bool RaftAgent::accept_offline_ack(RaftInstance &instance, const Address &address) {
    auto iter = instance._peers.find(address);
    if (iter == instance._peers.end()) {
        G_WARN << instance.name_state()
                << ": Receive unknown OfflineAck from " << address.toIpPort();
        return false;
    };
    if (iter->second != 0) {
        iter->second = 0;
        G_INFO << instance.name_state()
                << ": Peer " << address.toIpPort() << " agree offline.";
        return true;
    }
    return false;
}

void RaftAgent::error_type(const char* fun, RaftInstance &instance,
                           const RaftMessage &message, const Address &address) {
    G_ERROR << instance.name_state()
            << ": Wrong message type " << RaftMessage::get_TypeName(message.type)
            << " in " << fun << " from " << address.toIpPort()
            << " instance version " << instance._transmitter.version.data()
            << " message version " << message.version.data();
}

bool RaftAgent::check_outdated(RaftInstance &instance,
                               const RaftMessage &message, const Address &address) {
    if (unlikely(message.version < instance._transmitter.version
        && message.type != RaftMessage::Online && message.type != RaftMessage::Offline)) {
        G_WARN << instance.name_state()
                << ": RaftAgent::handle_peer_message get outdated message from "
                << address.toIpPort() << " type " << RaftMessage::get_TypeName(message.type);
        return true;
    }
    return false;
}

void RaftAgent::assert_split_brain(RaftInstance &instance,
                                   const RaftMessage &message, const Address &address) {
    if (unlikely(message.version < instance._transmitter.version
        || (message.version == instance._transmitter.version
            && instance._state == RaftInstance::Leader))) {
        Global_Logger.flush();
        CurrentThread::emergency_exit(instance.name_state() + ": Split-brain happen between "
            + instance._leader.toIpPort() + " and " + address.toIpPort()
            + " message type: " + RaftMessage::get_TypeName(message.type));
    }
}

void RaftAgent::become_candidate(RaftInstance &instance) {
    G_INFO << instance.name_state() << ": Be a candidate.";
    instance._state = RaftInstance::Candidate;
    instance._leader = {};
    instance._last_flush_time = Unix_to_now();
    ++instance._transmitter.version;
    instance.votes = 1;
}

void RaftAgent::become_leader(RaftInstance &instance) {
    G_INFO << instance.name_state() << ": Run for the leadership.";
    instance._state = RaftInstance::Leader;
    instance._leader = {};
    instance._last_flush_time = Unix_to_now();
    instance.votes = 0;
}

void RaftAgent::change_leader_to(RaftInstance &instance,
                                 const RaftMessage &message, const Address &address) {
    assert_split_brain(instance, message, address);
    G_INFO << instance.name_state() << ": Change leader " << instance._leader.toIpPort()
            << " to " << address.toIpPort();
    instance._state = RaftInstance::Follower;
    instance._leader = address;
    instance._transmitter.version = message.version;
    if (address.valid())
        instance._last_flush_time = Unix_to_now();
    instance.votes = 0;
}

#undef STATE_MECHINE
