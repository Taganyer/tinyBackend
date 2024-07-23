//
// Created by taganyer on 3/12/24.
//

#include "../FileDescriptor.hpp"
#include "Net/error/errors.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Net/functions/Interface.hpp"


using namespace Net;


FileDescriptor::~FileDescriptor() {
    if (_fd > 0 && !ops::close(_fd))
        G_FATAL << "fd " << _fd << ' ' << ops::get_close_error(errno);
}
