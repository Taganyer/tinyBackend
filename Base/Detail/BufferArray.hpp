//
// Created by taganyer on 24-10-24.
//

#ifndef BASE_BUFFERARRAY_HPP
#define BASE_BUFFERARRAY_HPP

#ifdef BASE_BUFFERARRAY_HPP

#include <array>
#include <bits/types/struct_iovec.h>
#include "config.hpp"

namespace Base {

    template <std::size_t N>
    using BufferArray = std::array<iovec, N>;
}

namespace std {

    template <std::size_t N>
    Base::BufferArray<N> operator+(const Base::BufferArray<N> &arr, uint64 step) {
        Base::BufferArray<N> result(arr);
        for (int i = 0; i < N && step; ++i) {
            if (result[i].iov_len < step) {
                step -= result[i].iov_len;
                result[i].iov_base = (char *) result[i].iov_base + result[i].iov_len;
                result[i].iov_len = 0;
            } else {
                result[i].iov_base = (char *) result[i].iov_base + step;
                result[i].iov_len -= step;
                step = 0;
            }
        }
        return result;
    }

    template <std::size_t N>
    Base::BufferArray<N>& operator+=(Base::BufferArray<N> &arr, uint64 step) {
        for (int i = 0; i < N && step; ++i) {
            if (arr[i].iov_len < step) {
                step -= arr[i].iov_len;
                arr[i].iov_base = (char *) arr[i].iov_base + arr[i].iov_len;
                arr[i].iov_len = 0;
            } else {
                arr[i].iov_base = (char *) arr[i].iov_base + step;
                arr[i].iov_len -= step;
                step = 0;
            }
        }
        return arr;
    }

    template <std::size_t N>
    uint64 size_of(const Base::BufferArray<N> &arr) {
        uint64 size = 0;
        for (const auto &i : arr) {
            size += i.iov_len;
        }
        return size;
    }

}

#endif

#endif //BASE_BUFFERARRAY_HPP
