//
// Created by taganyer on 25-5-16.
//
#pragma once

#include <type_traits>

namespace Base {

    template <unsigned Size, unsigned Alignment = 1>
    class PlainAny {
    public:
        constexpr PlainAny() = default;

        constexpr PlainAny(const PlainAny&) = default;

        constexpr PlainAny& operator=(const PlainAny&) = default;

        template <typename Type>
        constexpr explicit PlainAny(const PlainAny& object) noexcept {
            static_cast<Type&>(*this) = object;
        };

        template <typename Type>
        constexpr PlainAny& operator=(const Type& object) noexcept {
            static_cast<Type&>(*this) = object;
            return *this;
        };

        template <typename Type>
        constexpr explicit operator Type() noexcept {
            check<Type>();
            return *static_cast<Type *>(static_cast<void *>(&st.data));
        };

        template <typename Type>
        constexpr explicit operator Type&() noexcept {
            check<Type>();
            return *static_cast<Type *>(static_cast<void *>(&st.data));
        };

        template <typename Type>
        constexpr explicit operator const Type&() const noexcept {
            check<Type>();
            return *static_cast<Type *>(static_cast<void *>(&st.data));
        };

    private:
        template <typename Type>
        __attribute__((always_inline)) constexpr void check() const {
            static_assert(sizeof(Type) <= sizeof(PlainAny), "Type size must be less than sizeof(PlainAny)");
            static_assert(std::is_trivially_copyable_v<Type>);
        };

        union type {
            unsigned char data[Size] {};

            struct __attribute__((__aligned__((Alignment)))) {} _align;
        } st;

    };

}
