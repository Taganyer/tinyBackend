//
// Created by taganyer on 24-9-29.
//

#ifndef DIST_RAFTAGENT_HPP
#define DIST_RAFTAGENT_HPP

#ifdef DIST_RAFTAGENT_HPP

#include "RaftTransmitter.hpp"

namespace Net {
    class InetAddress;
}

namespace Dist {

    class RaftInstance;

    class RaftMessage;

    class RaftAgent {
    public:
        using Address = RaftTransmitter::Address;

        using Version = RaftTransmitter::Version;

#define INSTANCE_ARG RaftInstance &instance
#define MESSAGE_ARG const RaftMessage &message
#define ADDRESS_ARG const Address &address

        static void handle_messages_from_follower(INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

        static void handle_messages_from_leader(INSTANCE_ARG, MESSAGE_ARG);

        static void handle_messages_from_peer(INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

        static void handle_leader_timeout(INSTANCE_ARG);

        static void handle_candidate_timeout(INSTANCE_ARG);

        static void handle_follower_timeout(INSTANCE_ARG);

        static bool handle_message_when_online(INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

        static bool handle_message_when_offline(INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

        static bool end_status_check(bool online, INSTANCE_ARG);

    private:
        static void respond_leader_check(INSTANCE_ARG, MESSAGE_ARG);

        static void vote_to_be_leader(INSTANCE_ARG);

        static void vote_to(INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

        static void accept_vote(INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

        static void synchronize_logs(INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

        static void new_one_online(INSTANCE_ARG, ADDRESS_ARG);

        static bool accept_online_ack(INSTANCE_ARG, ADDRESS_ARG);

        static void peer_offline(INSTANCE_ARG, ADDRESS_ARG);

        static bool accept_offline_ack(INSTANCE_ARG, ADDRESS_ARG);

        static void error_type(const char* fun, INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

        static bool check_outdated(INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

        static void assert_split_brain(INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

        static void become_candidate(INSTANCE_ARG);

        static void become_leader(INSTANCE_ARG);

        static void change_leader_to(INSTANCE_ARG, MESSAGE_ARG, ADDRESS_ARG);

#undef INSTANCE_ARG
#undef MESSAGE_ARG
#undef ADDRESS_ARG

    };

}

#endif

#endif //DIST_RAFTAGENT_HPP
