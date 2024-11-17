//
// Created by taganyer on 24-10-17.
//

#ifndef BASE_VERSIONNUMBER_HPP
#define BASE_VERSIONNUMBER_HPP

#ifdef BASE_VERSIONNUMBER_HPP

#include "Detail/config.hpp"

namespace Base {

    class VersionNumber64 {
    public:
        VersionNumber64() = default;

        VersionNumber64(const VersionNumber64 &) = default;

        VersionNumber64& operator=(const VersionNumber64 &) = default;

        explicit VersionNumber64(uint64 version) : _version(version) {};

        [[nodiscard]] uint64 data() const { return _version; };

        VersionNumber64& operator++() {
            if (likely(_version != MAX_ULLONG)) ++_version;
            else _version = 0;
            return *this;
        };

        VersionNumber64 operator++(int) {
            VersionNumber64 temp = *this;
            if (likely(_version != MAX_ULLONG)) ++_version;
            else _version = 0;
            return temp;
        };

        VersionNumber64& operator+=(uint64 _step) {
            uint64 res = MAX_ULLONG - _version;
            if (likely(_step < res)) _version += _step;
            else _version = _step - res - 1;
            return *this;
        };

        friend VersionNumber64 operator+(VersionNumber64 version, uint64 _step) {
            uint64 res = MAX_ULLONG - version._version;
            if (likely(_step < res)) return VersionNumber64 { version._version + _step };
            return VersionNumber64 { _step - res - 1 };
        };

        friend bool operator==(VersionNumber64 left, VersionNumber64 right) {
            return left._version == right._version;
        };

        friend bool operator!=(VersionNumber64 left, VersionNumber64 right) {
            return left._version != right._version;
        };

        friend bool operator<(VersionNumber64 left, VersionNumber64 right) {
            if (left._version < right._version)
                return right._version - left._version <= MAX_ULLONG / 2;
            return left._version - right._version > MAX_ULLONG / 2;
        };

        friend bool operator>(VersionNumber64 left, VersionNumber64 right) {
            if (left._version > right._version)
                return left._version - right._version <= MAX_ULLONG / 2;
            return right._version - left._version > MAX_ULLONG / 2;
        };

        friend bool operator<=(VersionNumber64 left, VersionNumber64 right) {
            return left == right || left < right;
        };

        friend bool operator>=(VersionNumber64 left, VersionNumber64 right) {
            return left == right || left > right;
        };

    private:
        uint64 _version = 0;

    };

    class VersionNumber32 {
    public:
        VersionNumber32() = default;

        VersionNumber32(const VersionNumber32 &) = default;

        VersionNumber32& operator=(const VersionNumber32 &) = default;

        explicit VersionNumber32(uint32 version) : _version(version) {};

        [[nodiscard]] uint32 data() const { return _version; };

        VersionNumber32& operator++() {
            if (likely(_version != MAX_UINT)) ++_version;
            else _version = 0;
            return *this;
        };

        VersionNumber32 operator++(int) {
            VersionNumber32 temp = *this;
            if (likely(_version != MAX_UINT)) ++_version;
            else _version = 0;
            return temp;
        };

        VersionNumber32& operator+=(uint32 _step) {
            uint32 res = MAX_UINT - _version;
            if (likely(_step < res)) _version += _step;
            else _version = _step - res - 1;
            return *this;
        };

        friend VersionNumber32 operator+(VersionNumber32 version, uint32 _step) {
            uint32 res = MAX_UINT - version._version;
            if (likely(_step < res)) return VersionNumber32 { version._version + _step };
            return VersionNumber32 { _step - res - 1 };
        };

        friend bool operator==(VersionNumber32 left, VersionNumber32 right) {
            return left._version == right._version;
        };

        friend bool operator!=(VersionNumber32 left, VersionNumber32 right) {
            return left._version != right._version;
        };

        friend bool operator<(VersionNumber32 left, VersionNumber32 right) {
            if (left._version < right._version)
                return right._version - left._version <= MAX_UINT / 2;
            return left._version - right._version > MAX_UINT / 2;
        };

        friend bool operator>(VersionNumber32 left, VersionNumber32 right) {
            if (left._version > right._version)
                return left._version - right._version <= MAX_UINT / 2;
            return right._version - left._version > MAX_UINT / 2;
        };

        friend bool operator<=(VersionNumber32 left, VersionNumber32 right) {
            return left == right || left < right;
        };

        friend bool operator>=(VersionNumber32 left, VersionNumber32 right) {
            return left == right || left > right;
        };

    private:
        uint32 _version = 0;

    };

}

#endif

#endif //BASE_VERSIONNUMBER_HPP
