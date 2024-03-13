//
// Created by taganyer on 24-2-29.
//

#include "Poller.hpp"
#include "../Base/Detail/CurrentThread.hpp"
#include "../Base/Log/Log.hpp"

using namespace Net;


void Poller::set_tid(pthread_t tid) {
    assert(_tid == -1);
    _tid = tid;
}

void Poller::assert_in_right_thread(const char *message) const {
    if (unlikely(_tid != Base::tid())) {
        G_ERROR << message << _tid << " running in wrong thread " << Base::tid();
        assert(_tid != Base::tid());
    }
}
