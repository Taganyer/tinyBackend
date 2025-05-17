//
// Created by taganyer on 3/12/24.
//

#include "../FileDescriptor.hpp"
#include "Net/functions/Interface.hpp"
#include "Base/SystemLog.hpp"
#include "Net/error/errors.hpp"


using namespace Net;


FileDescriptor::~FileDescriptor() {
    if (_fd > 0 && !ops::close(_fd))
        G_FATAL << "FileDescriptor " << _fd << ' ' << ops::get_close_error(errno) << errno;
}
