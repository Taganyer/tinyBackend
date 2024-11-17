//
// Created by taganyer on 24-11-17.
//

#ifndef BASE_TIME_HPP
#define BASE_TIME_HPP

#ifdef BASE_TIME_HPP

#include <ctime>
#include <string>
#include "Base/Detail/config.hpp"

namespace Base {

    struct Time : tm {
        static constexpr char Time_format[] = "%4d%02d%02d-%02d:%02d:%02d";

        static constexpr int32 Time_format_len = 17;

        static constexpr char Time_us_format[] = "%4d%02d%02d-%02d:%02d:%02d.%06ld";

        static constexpr int32 Time_us_format_len = 24;

        long tm_us = -1;

        Time() = default;

        static Time now(bool UTC = false);

    };

    std::string to_string(const Time &time, bool show_us = true);

    void format(char* dest, const Time &time, bool show_us = true);

}

#endif

#endif //TIME_HPP
