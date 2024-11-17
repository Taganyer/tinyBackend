//
// Created by taganyer on 24-9-27.
//

#ifndef DIST_RAFTMESSAGE_HPP
#define DIST_RAFTMESSAGE_HPP

#ifdef DIST_RAFTMESSAGE_HPP

#include "Base/VersionNumber.hpp"
#include "Base/Time/Time_difference.hpp"

namespace Dist {

    class RaftMessage {
    public:
        enum Type : unsigned short {
            Invalid,
            LeaderCheck,
            FollowerAck,
            AskForVote,
            Vote,
            Request,
            Online,
            OnlineAck,
            Offline,
            OfflineAck
        };

        static const char* get_TypeName(Type type) {
            constexpr const char* name[] = {
                "[Invalid]",
                "[LeaderCheck]",
                "[FollowerAck]",
                "[AskForVote]",
                "[Vote]",
                "[Request]",
                "[Online]",
                "[OnlineAck]",
                "[Offline]",
                "[OfflineAck]"
            };
            return name[type];
        };

        using Version = Base::VersionNumber32;

        using Serial = Base::VersionNumber64;

        using LogVersion = Base::VersionNumber64;

        RaftMessage() = default;

#define MESSAGE_ARG Version _version, Serial serial
#define MESSAGE_INIT(_Type)  type = _Type; \
                        version = _version; \
                        serial_number = serial; \
                        update_time();

        void leader_check(MESSAGE_ARG) {
            MESSAGE_INIT(LeaderCheck)
        };

        void follower_ack(MESSAGE_ARG) {
            MESSAGE_INIT(FollowerAck)
        };

        void ask_for_vote(MESSAGE_ARG) {
            MESSAGE_INIT(AskForVote)
        };

        void vote(MESSAGE_ARG, bool val) {
            MESSAGE_INIT(Vote)
            choose = val;
        };

        void request_logs(MESSAGE_ARG) {
            MESSAGE_INIT(Request)
        };

        void online(MESSAGE_ARG) {
            MESSAGE_INIT(Online)
        };

        void affirm_online(MESSAGE_ARG) {
            MESSAGE_INIT(OnlineAck)
        };

        void offline(MESSAGE_ARG) {
            MESSAGE_INIT(Offline)
        };

        void affirm_offline(MESSAGE_ARG) {
            MESSAGE_INIT(OfflineAck);
        };

#undef MESSAGE_INIT
#undef MESSAGE_ARG

        void update_time() { time_of_departure = Base::Unix_to_now(); };

        bool add_extra_message(const void* data, uint64 size) {
            if (size > sizeof(extra_data) - extra_data_size)
                return false;
            char* dest = extra_data + extra_data_size;
            extra_data_size += size;
            for (auto ptr = (const char *) data;
                 size; --size, ++ptr, ++dest)
                *dest = *ptr;
            return true;
        };

        Type type = Invalid;

        uint8 extra_data_size = 0;

        bool choose = false;

        Version version;

        Serial serial_number;

        Base::Time_difference time_of_departure;

        char extra_data[40] {};

    };
}

#endif

#endif //DIST_RAFTMESSAGE_HPP
