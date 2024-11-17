//
// Created by taganyer on 24-9-30.
//

#ifndef DISTRIBUTED_RAFTSTATEMACHINE_HPP
#define DISTRIBUTED_RAFTSTATEMACHINE_HPP

#ifdef DISTRIBUTED_RAFTSTATEMACHINE_HPP

namespace Dist {

    class RaftMessage;

    class RaftStateMachine {
    public:
        using Address = Net::InetAddress;

        RaftStateMachine() = default;

        virtual ~RaftStateMachine() = default;

        virtual bool can_be_leader(const Address &sender_address,
                                   const RaftMessage &message) = 0;

        virtual std::string leader_notify_message(const Address &follower_address) = 0;

        virtual bool need_synchronize_from(const Address &sender_address,
                                           const RaftMessage &message) = 0;

        virtual std::string ready_to_receiving(const Address &sender_address,
                                               const RaftMessage &message) = 0;

        virtual void ready_to_send_to(const Address &receiver_address,
                                      const RaftMessage &message) = 0;

        virtual void update_log() = 0;
    };
}

#endif

#endif //DISTRIBUTED_RAFTSTATEMACHINE_HPP
