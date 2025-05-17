//
// Created by taganyer on 24-9-28.
//

#ifndef DIST_RAFTTRANSMITTER_HPP
#define DIST_RAFTTRANSMITTER_HPP

#ifdef DIST_RAFTTRANSMITTER_HPP

#include "RaftStateMachine.hpp"
#include "tinyBackend/Base/VersionNumber.hpp"
#include "tinyBackend/Net/UDP_Communicator.hpp"


namespace Dist {

    class RaftMessage;

    class RaftTransmitter {
    public:
        using Address = Net::InetAddress;

        using Version = Base::VersionNumber32;

        using Serial = Base::VersionNumber64;

        explicit RaftTransmitter(const Address& local_address, RaftStateMachine *machine,
                                 std::string name);

        void notify_follower(const Address& follower, const std::string& data);

        void respond_leader(const Address& leader);

        void request_vote(const Address& peer);

        void vote_to(const Address& peer, bool result);

        void request_logs(const Address& address, const std::string& data);

        void this_one_online(const Address& peer);

        void affirm_online(const Address& address);

        void this_one_offline(const Address& peer);

        void affirm_offline(const Address& address);

        Address receive(RaftMessage *message, Base::TimeInterval end_time) const;

    private:
        Net::UDP_Communicator _sender;

    public:
        Version version;

        Serial serial_number;

        RaftStateMachine *machine;

        std::string _name;

    };

}

#endif

#endif //DIST_RAFTTRANSMITTER_HPP
