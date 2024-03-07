//
// Created by taganyer on 24-2-29.
//

#include "Poller.hpp"
#include "../Base/Detail/CurrentThread.hpp"
#include "../Base/Log/Log.hpp"

using namespace Net;


void Poller::assert_in_right_thread(const char *message) const {
    if (unlikely(_tid != tid())) {
        G_ERROR << message << _tid << " running in wrong thread " << tid();
        assert(_tid != tid());
    }
}
