//
// Created by taganyer on 3/24/24.
//

#include <sys/poll.h>
#include "../Event.hpp"

using namespace Net;

const int Event::NoEvent = 0;

const int Event::Read = POLLIN;

const int Event::Urgent = POLLPRI;

const int Event::Write = POLLOUT;

const int Event::Error = POLLERR;

const int Event::Invalid = POLLNVAL;

const int Event::HangUp = POLLHUP;
