//
// Created by taganyer on 3/28/24.
//

#ifndef LOGSYSTEM_LOGRANK_HPP
#define LOGSYSTEM_LOGRANK_HPP

#ifdef LOGSYSTEM_LOGRANK_HPP

#include <cstring>

namespace LogSystem {

    enum LogRank : unsigned char {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        EMPTY
    };

    inline const char* LogRankName(LogRank rank) {
        constexpr const char* names[] = {
            "TRACE",
            "DEBUG",
            "INFO",
            "WARN",
            "ERROR",
            "FATAL",
            "EMPTY"
        };
        return names[rank];
    }

    inline int rank_toString(char* ptr, LogRank rank) {
        constexpr const char* store[] {
            "TRACE:",
            "DEBUG:",
            "INFO :",
            "WARN :",
            "ERROR:",
            "FATAL:",
            "EMPTY:"
        };
        std::memcpy(ptr, store[rank], 6);
        return 6;
    }

}

#endif

#endif //LOGSYSTEM_LOGRANK_HPP
