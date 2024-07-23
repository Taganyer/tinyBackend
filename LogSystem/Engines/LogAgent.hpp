//
// Created by taganyer on 24-7-23.
//

#ifndef ENGINES_LOGAGENT_HPP
#define ENGINES_LOGAGENT_HPP
#ifdef ENGINES_LOGAGENT_HPP

#include "EnginesEnum.hpp"
#include "Net/InetAddress.hpp"
#include "Base/Detail/NoCopy.hpp"
#include "Base/Detail/config.hpp"


namespace LogSystem {

    class LogAgent : private Base::NoCopy {
    public:
        explicit LogAgent(EnginesEnum engines_enum);

        uint64 write(const Net::InetAddress &address);

    };

}

#endif

#endif //ENGINES_LOGAGENT_HPP
