//
// Created by taganyer on 3/8/24.
//

#include "Reactor.hpp"

#include "poller/EpollPoller.hpp"

#include "poller/PollPoller.hpp"

#include "NetLink.hpp"

#include "Channel.hpp"

using namespace Net;

using namespace Base;


Reactor::Reactor(bool epoll) {
    if (epoll) {
        _poller = new EpollPoller();
    } else {
        _poller = new PollPoller();
    }
}

Reactor::~Reactor() {
    close();
    assert(_poller->events_size() == 0);
    delete _poller;
}

void Reactor::add_NetLink(NetLink &netLink) {
    assert(running());
    assert(netLink.valid() && !netLink.has_channel());
    get_loop()->put_event([ptr = netLink.get_data(), m = _manger] {
        auto data = ptr.lock();
        if (data) {
            Channel *channel = Channel::create_Channel(data->FD->fd(), data, *m);
            m->add_channel(channel);
            data->_channel = channel;
        }
    });
}

void Reactor::start(int poller_timeoutMS, int channel_timeoutS) {
    assert(!running());

    Thread thread([this, poller_timeoutMS, channel_timeoutS] {
        _poller->set_tid(Base::tid());
        EventLoop loop;
        _loop = &loop;
        ChannelsManger manger(&loop, _poller);
        _manger = &manger;

        loop.set_distributor([poller_timeoutMS, &manger, poller = _poller] {
            manger.update();
            poller->poll(poller_timeoutMS < 0 ? -1 : poller_timeoutMS, manger.activeList);
            auto size = (int64) manger.activeList.size();
            for (int i = 0; i < size; ++i) {
                Channel *channel = manger.activeList[i];
                channel->invoke();
            }
            manger.activeList.erase(manger.activeList.begin(), manger.activeList.begin() + size);
        });

        manger.set_timeout(channel_timeoutS < 0 ? 0 : channel_timeoutS * Base::SEC_);

        loop.loop();

        _loop = nullptr;
        _manger = nullptr;
    });

    thread.start();
}

void Reactor::close() {
    if (running()) {
        _loop->put_event([this] {
            _loop->shutdown();
        });
    }
}


