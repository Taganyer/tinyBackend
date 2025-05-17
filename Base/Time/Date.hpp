//
// Created by taganyer on 24-2-12.
//

#ifndef BASE_DATE_HPP
#define BASE_DATE_HPP

#include <string>
#include "tinyBackend/Base/Detail/config.hpp"

struct tm;

namespace Base {

    struct Date_data {
        int32 year = 0;
        int32 month = 0;
        int32 day = 0;

        Date_data() = default;

        Date_data(int32 y, int32 m, int32 d) : year(y), month(m), day(d) {};
    };

    class Date {
    public:

        static const char *Date_format;

        static const int32 Date_format_len;

        static const int32 JulianDayOf1970_01_01;

        static int32 get_julianDayNumber(int32 year, int32 month, int32 day);

        static Date_data get_Date_data(int32 julianDayNumber);

        Date() = default;

        explicit Date(const Date_data &data);

        explicit Date(const tm &time);

        explicit Date(int32 julianDayNumber) : _julianDayNumber(julianDayNumber) {};

        Date(int32 year, int32 month, int32 day);

        [[nodiscard]] Date_data get_date() const;

        [[nodiscard]] int32 get_julianDayNumber() const { return _julianDayNumber; };

        [[nodiscard]] std::string to_string() const;

        void format(char *dest) const;

        operator bool() const { return _julianDayNumber; };

    private:

        int32 _julianDayNumber = 0;

    };

#define Date_operator(symbol) \
    inline bool operator symbol(const Date &left, const Date &right) { \
        return left.get_julianDayNumber() < right.get_julianDayNumber(); \
    }

    Date_operator(<)

    Date_operator(<=)

    Date_operator(>)

    Date_operator(>=)

    Date_operator(==)

    Date_operator(!=)

#undef Date_operator

    inline int32 operator-(const Date &left, const Date &right) {
        return left.get_julianDayNumber() - right.get_julianDayNumber();
    }

    inline Date operator+(const Date &date, int32 days) {
        return Date(date.get_julianDayNumber() + days);
    }

    inline Date operator-(const Date &date, int32 days) {
        return Date(date.get_julianDayNumber() - days);
    }

    std::string date_to_string(const Date_data &data);

    void date_format(char *dest, const Date_data &data);

}

#endif //BASE_DATE_HPP
