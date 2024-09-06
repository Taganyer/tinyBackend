//
// Created by taganyer on 24-9-6.
//

#ifndef LOGSYSTEM_LOGERROR_HPP
#define LOGSYSTEM_LOGERROR_HPP

#ifdef LOGSYSTEM_LOGERROR_HPP

#include "Base/Exception.hpp"

namespace LogSystem {

    class LogError : public Base::Exception {
    public:
        LogError(int error_code, const std::string &message) :
            Exception(message), _error_code(error_code) {};

        [[nodiscard]] int error_code() const {
            return _error_code;
        };

    private:
        int _error_code = 0;
    };
}

#endif

#endif //LOGSYSTEM_LOGERROR_HPP
