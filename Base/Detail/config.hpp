//
// Created by taganyer on 24-2-5.
//

#ifndef BASE_CONFIG_HPP
#define BASE_CONFIG_HPP

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#define SIZEOF_LONG_ 4
#define IS_WIN

#elif defined(__linux__) || defined(__gnu_linux__) || defined(__APPLE__) || defined(__MACH__)
#define SIZEOF_LONG_ 8
#define IS_LINUX

#else
#error "Unknown platform"
#endif

using sizet = unsigned long long;

using diff = long long;

using int8 = char;

using int16 = short;

using int32 = int;

using int64 = long long;

using uint8 = unsigned char;

using uint16 = unsigned short;

using uint32 = unsigned int;

using uint64 = unsigned long long;

constexpr short MAX_SHORT = 32767;

constexpr short MIN_SHORT = -32768;

constexpr short MAX_USHORT = -1;

constexpr int MAX_INT = 2147483647;

constexpr int MIN_INT = -2147483648;

constexpr unsigned int MAX_UINT = static_cast<const unsigned int>(-1);

#if SIZEOF_LONG_ == 4
constexpr long MAX_LONG = 2147483647;

constexpr long MIN_LONG = -2147483648;

constexpr unsigned long MAX_ULONG = -1;

#elif SIZEOF_LONG_ == 8
constexpr long MAX_LONG = 9223372036854775807;

constexpr long MIN_LONG = -9223372036854775807L - 1;

constexpr unsigned long MAX_ULONG = static_cast<const unsigned long>(-1);

#else
#error "Unknown platform"
#endif

constexpr long long MAX_LLONG = 9223372036854775807;

constexpr long long MIN_LLONG = -9223372036854775807LL - 1;

constexpr unsigned long long MAX_ULLONG = static_cast<const unsigned long long int>(-1);

constexpr float MAX_FLOAT = 3.402823466e+38f;

constexpr float MIN_FLOAT = -3.402823466e+38f;

constexpr double MAX_DOUBLE = 1.7976931348623157e+308;

constexpr double MIN_DOUBLE = -1.7976931348623157e+308;

#ifdef __GNUC__

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#else

#define likely(x)      (x)
#define unlikely(x)    (x)

#endif

#endif //BASE_CONFIG_HPP
