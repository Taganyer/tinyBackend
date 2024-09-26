//
// Created by taganyer on 3/23/24.
//

#ifndef NET_MONITORS_HPP
#define NET_MONITORS_HPP

#include <vector>
#include <pthread.h>
#include "Event.hpp"
#include "Net/error/error_mark.hpp"
#include "Base/Detail/config.hpp"
#include "Base/Detail/NoCopy.hpp"


namespace Net {

    class Monitor : Base::NoCopy {
    public:
        using EventList = std::vector<Event>;

        Monitor() = default;

        virtual ~Monitor() = default;

        virtual int get_aliveEvent(int timeoutMS, EventList &list) = 0;

        virtual bool add_fd(Event event) = 0;

        virtual void remove_fd(int fd) = 0;

        virtual void remove_all() = 0;

        virtual void update_fd(Event event) = 0;

        /// TODO 只在初始化时调用
        void set_tid(pthread_t tid) { _tid = tid; };

        [[nodiscard]] virtual uint64 fd_size() const = 0;

        [[nodiscard]] pthread_t tid() const { return _tid; };

    protected:
        pthread_t _tid = -1;

    public:
        error_mark error_ = { error_types::Null, 0 };

    };

}


#endif //NET_MONITORS_HPP
