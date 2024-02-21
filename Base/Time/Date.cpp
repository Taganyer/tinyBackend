//
// Created by taganyer on 24-2-12.
//

#include "Date.hpp"
#include <ctime>

namespace Base {

    const char *Date::Date_format = "%4d_%02d_%02d";

    const int32 Date::Date_format_len = 10;

    int32 Date::get_julianDayNumber(int32 year, int32 month, int32 day) {
        int32 a = (14 - month) / 12;
        int32 y = year + 4800 - a;
        int32 m = month + 12 * a - 3;
        return day + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400 - 32045;
    }

    Date_data Date::get_Date_data(int32 julianDayNumber) {
        int32 a = julianDayNumber + 32044;
        int32 b = (4 * a + 3) / 146097;
        int32 c = a - ((b * 146097) / 4);
        int32 d = (4 * c + 3) / 1461;
        int32 e = c - ((1461 * d) / 4);
        int32 m = (5 * e + 2) / 153;
        int32 day = e - ((153 * m + 2) / 5) + 1;
        int32 month = m + 3 - 12 * (m / 10);
        int32 year = b * 100 + d - 4800 + (m / 10);
        return {year, month, day};
    }

    const int32 Date::JulianDayOf1970_01_01 = get_julianDayNumber(1970, 1, 1);

    Date::Date(const Date_data &data) :
            _julianDayNumber(get_julianDayNumber(data.year, data.month, data.day)) {}

    Date::Date(const tm &time) :
            _julianDayNumber(get_julianDayNumber(
                    time.tm_year + 1900, time.tm_mon + 1, time.tm_mday)) {}

    Date::Date(int32 year, int32 month, int32 day) :
            _julianDayNumber(get_julianDayNumber(year, month, day)) {}

    Date_data Date::get_date() const {
        return get_Date_data(_julianDayNumber);
    }

    std::string Date::to_string() const {
        return date_to_string(get_date());
    }

    void Date::format(char *dest) const {
        date_format(dest, get_date());
    }

    std::string date_to_string(const Date_data &data) {
        std::string date(Date::Date_format_len, '\0');
        std::sprintf(date.data(), Date::Date_format,
                     data.year, data.month, data.day);
        return date;
    }

    void date_format(char *dest, const Date_data &data) {
        std::sprintf(dest, Date::Date_format,
                     data.year, data.month, data.day);
    }

}
