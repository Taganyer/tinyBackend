//
// Created by taganyer on 24-5-11.
//

#ifndef BASE_TIMERLOOP_HPP
#define BASE_TIMERLOOP_HPP

#ifdef BASE_TIMERLOOP_HPP

#include <queue>
#include <memory>
#include <functional>
#include "Base/Condition.hpp"
#include "Base/Time/Time_difference.hpp"

namespace Base {

    /*
     * 不精准的定时队列，只能确保任务运行时间不会早于：插入时间 + n * 间隔时间。
     * 延时时间受竞态条件和队列任务数量和每个任务运行的时间，不建议插入运行时间过长的任务。
     */

    class TimerLoop : NoCopy {
    public:

        using Event = std::function<void()>;

        class EventID;

        class Node;

        TimerLoop() = default;

        ~TimerLoop();

        void loop();

        /// 阻塞等待，直到关闭定时运行线程，同时清空所有定时任务。
        void shutdown();

        /// 将事件加入队列，同时返回一个事件控制ID。interval 必须大于等于零。
        EventID put_event(Event fun, Time_difference interval);

        [[nodiscard]] bool looping() const { return _run; };

    private:

        using List = std::priority_queue<Node>;

        Mutex _mutex;

        Condition _con;

        volatile bool _run = false, _quit = false;

        List _list;

        Node get_event();

        void invoke_event();

    public:

        class Node {
        public:

            using Pair = std::pair<Time_difference, Event>;

            using Ptr = std::shared_ptr<Pair>;

            friend bool operator<(const Node &left, const Node &right) {
                return left._now <= right._now;
            };

        private:

            Time_difference _now = Unix_to_now();

            Ptr _ptr;

            Node() = default;

            Node(Event fun, Time_difference interval);

            bool update();

            bool invoke();

            friend class TimerLoop;

        };

        class EventID {
        public:

            EventID(const EventID &) = default;

            ~EventID() = default;

            /// 重设间隔时间，线程安全。interval 必须大于等于零。
            void reset(Time_difference interval);

            /// 取消任务，线程安全。
            void cancel();

            /// 判定任务是否在定时队列中。
            [[nodiscard]] bool expired() const;

        private:

            using Ptr = std::weak_ptr<Node::Pair>;

            TimerLoop *_loop;

            Ptr _ptr;

            EventID(TimerLoop *loop, const Node::Ptr &ptr) :
                    _loop(loop), _ptr(ptr) {};

            friend class TimerLoop;

        };

    };

}

#endif

#endif //BASE_TIMERLOOP_HPP
