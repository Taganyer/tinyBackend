//
// Created by taganyer on 24-2-21.
//

#include "Base/Exception.hpp"

#include "Base/Detail/CurrentThread.hpp"

namespace Base {


    Exception::Exception(std::string what) : message_(std::move(what)),
        stack_(CurrentThread::stackTrace(false)) {}
}
