//
// Created by taganyer on 3/23/24.
//

#include "../Monitor.hpp"
#include "LogSystem/SystemLog.hpp"

using namespace Net;
using namespace Base;

void Monitor::set_tid(pthread_t tid) {
    assert(_tid == -1);
    _tid = tid;
}

void Monitor::assert_in_right_thread(const char* message) const {
    if (unlikely(_tid != Base::CurrentThread::tid())) {
        G_ERROR << message << _tid << " running in wrong thread " << Base::CurrentThread::tid();
        assert(_tid != Base::CurrentThread::tid());
    }
}
