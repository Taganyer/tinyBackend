//
// Created by taganyer on 3/28/24.
//

#ifndef LOGSYSTEM_LOGRANK_HPP
#define LOGSYSTEM_LOGRANK_HPP

#ifdef LOGSYSTEM_LOGRANK_HPP

#include <cstring>

namespace LogSystem {

    enum LogRank {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        EMPTY
    };

    inline int rank_toString(char *ptr, LogRank rank) {
        if (rank == EMPTY) return 0;
        constexpr char store[6][7]{
                "TRACE:",
                "DEBUG:",
                "INFO :",
                "WARN :",
                "ERROR:",
                "FATAL:",
        };
        std::memcpy(ptr, store[rank], 6);
        return 6;
    }

}

#endif

#endif //LOGSYSTEM_LOGRANK_HPP
