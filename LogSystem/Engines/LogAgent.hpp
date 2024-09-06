//
// Created by taganyer on 24-7-23.
//

#ifndef ENGINES_LOGAGENT_HPP
#define ENGINES_LOGAGENT_HPP

#ifdef ENGINES_LOGAGENT_HPP

#include "EnginesEnum.hpp"
#include "Net/InetAddress.hpp"
#include "LogSystem/LogRank.hpp"
#include "LogSystem/ResultSet.hpp"
#include "Base/Detail/NoCopy.hpp"
#include "Base/Time/Time_difference.hpp"


namespace LogSystem {

    class LogAgent : private Base::NoCopy {
    public:
        explicit LogAgent(EnginesEnum engines_enum);

        virtual ~LogAgent() = 0;

        virtual uint64 write(LogRank rank, const Net::InetAddress &address,
                             void* message, uint64 size) = 0;

        // virtual ResultSet read(Base::Time_difference begin, Base::Time_difference end) = 0;



    };

}

#endif

#endif //ENGINES_LOGAGENT_HPP
