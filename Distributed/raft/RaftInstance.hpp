//
// Created by taganyer on 24-9-26.
//

#ifndef DIST_RAFTINSTANCE_HPP
#define DIST_RAFTINSTANCE_HPP

#ifdef DIST_RAFTINSTANCE_HPP

#include <atomic>
#include <unordered_map>
#include "tinyBackend/Base/Mutex.hpp"
#include "tinyBackend/Base/Thread.hpp"
#include "RaftTransmitter.hpp"
#include "tinyBackend/Base/Time/TimeInterval.hpp"

namespace Dist {

    class RaftInstance {
    public:
        static Base::TimeInterval LEADER_SENT_TIMEOUT;

        static Base::TimeInterval CANDIDATE_RECEIVE_TIMEOUT;

        static Base::TimeInterval FOLLOWER_RECEIVE_TIMEOUT;

        static Base::TimeInterval ONLINE_WAIT_TIMEOUT;

        static Base::TimeInterval OFFLINE_WAIT_TIMEOUT;

        using Address = RaftTransmitter::Address;

        using Version = RaftTransmitter::Version;

        enum State {
            Leader,
            Candidate,
            Follower
        };

        /// addr 将会用作 UDP 通信
        RaftInstance(const Net::InetAddress& addr, RaftStateMachine *state_machine,
                     std::string name = std::string());

        ~RaftInstance() { stop(); };

        /// 禁止加入 RaftInstance 自身的地址
        void add_peer(const Net::InetAddress& peer);

        void start();

        void stop();

        /// 当集群中没有本节点的地址时才需要调用。
        bool join_to_cluster();

        bool exit_cluster();

        void synchronize_logs_to_followers();

        const std::string& name() const { return _transmitter._name; };

        std::string name_state() const;

        bool running() const { return _running.load(std::memory_order_acquire); };

        State state() const { return _state; };

    private:
        using Peers = std::unordered_map<Net::InetAddress, Base::TimeInterval>;

        Base::Mutex _mutex;

        Peers _peers;

        State _state = Follower;

        Net::InetAddress _leader {};

        Base::TimeInterval _last_flush_time;

        RaftTransmitter _transmitter;

        int votes = 0;

        std::atomic_bool _running = false;

        Base::Thread _thread;

        friend class RaftThread;

        friend class RaftAgent;

        Base::TimeInterval follower_receive_timeout() const;

        void leader_thread_loop();

        void candidate_thread_loop();

        void follower_thread_loop();

        static const char* get_StateName(State state);

    };

}

#endif

#endif //DIST_RAFTINSTANCE_HPP
