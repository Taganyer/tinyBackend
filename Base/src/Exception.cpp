//
// Created by taganyer on 24-2-21.
//

#include "../Exception.hpp"
#include "tinyBackend/Base/Detail/CurrentThread.hpp"

namespace Base {

    Exception::Exception(std::string what) : message_(std::move(what)),
        stack_(CurrentThread::stackTrace()) {}

}
