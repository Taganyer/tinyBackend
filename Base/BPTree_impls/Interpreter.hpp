//
// Created by taganyer on 24-9-9.
//

#ifndef BASE_INTERPRETER_HPP
#define BASE_INTERPRETER_HPP

#ifdef BASE_INTERPRETER_HPP

#include <cstring>
#include <type_traits>
#include "Base/Detail/config.hpp"

namespace Base {

    // 辅助模板，用于检测类型 T 是否具有 ObjectSize 成员
    template <typename T, typename = void>
    struct has_size_member : std::false_type {};

    template <typename T>
    struct has_size_member<T, std::void_t<decltype(T::ObjectSize)>> {};

    template <typename T, typename = void>
    struct get_size {
        constexpr static uint32 size = T::ObjectSize;

        static constexpr void write_to(void* dest, const T &obj) {
            obj.write_to(dest);
        };
    };

    template <typename T>
    struct get_size<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
        constexpr static uint32 size = sizeof(T);

        static constexpr void write_to(void* dest, const T &obj) {
            *(static_cast<T *>(dest)) = obj;
        };
    };

    template <typename T, typename = void>
    struct FixedSizeChecker {
        constexpr static bool is_fixed = false;

        static uint32 size_of(const T &obj) {
            return obj.get_size();
        };

        static void write_to(void* dest, const T &obj) {
            obj.write_to(dest);
        };
    };

    template <typename T>
    struct FixedSizeChecker<T, std::enable_if_t<std::is_arithmetic_v<T>
                                || has_size_member<T>::value>> {
        constexpr static bool is_fixed = true;

        static constexpr uint32 get_size(const T &) {
            return Base::get_size<T>::size;
        };

        static constexpr void write_to(void* dest, const T &obj) {
            Base::get_size<T>::write_to(dest, obj);
        };
    };

    class Interpreter {
    public:
        static constexpr uint32 BLOCK_SIZE = 1 << 12;

        static constexpr uint32 PTR_SIZE = sizeof(uint32);

        static void* move_ptr(void* buffer, uint32 distance) {
            return static_cast<char *>(buffer) + distance;
        };

        static const void* move_ptr(const void* buffer, uint32 distance) {
            return static_cast<const char *>(buffer) + distance;
        };

        static void write_ptr(void* pos, uint32 ptr) {
            *(uint32 *) pos = ptr;
        };

        static bool is_head_block(const void* buffer) {
            return (*(const uint8 *) buffer & 3) == 0;
        };

        static bool is_record_deleted_block(const void* buffer) {
            return (*(const uint8 *) buffer & 3) == 1;
        };

        static bool is_data_block(const void* buffer) {
            return (*(const uint8 *) buffer & 3) == 2;
        };

        static bool is_index_block(const void* buffer) {
            return (*(const uint8 *) buffer & 3) == 3;
        };

        struct HeadBlockHelper {
            [[nodiscard]] uint32* begin_block_index() const {
                return (uint32 *) move_ptr(buffer, 1);
            };

            [[nodiscard]] uint32* valid_blocks_size() const {
                return (uint32 *) move_ptr(buffer, 5);
            };

            [[nodiscard]] uint32* begin_record_block_index() const {
                return (uint32 *) move_ptr(buffer, 9);
            };

            [[nodiscard]] uint32* deleted_blocks_size() const {
                return (uint32 *) move_ptr(buffer, 13);
            };

            [[nodiscard]] uint32* get_pos_deleted_block(uint32 pos) const {
                if (have_extra_data())
                    return (uint32 *) move_ptr(buffer, 19 + *extra_data_size() + pos * 4);
                return (uint32 *) move_ptr(buffer, 17 + pos * 4);
            };

            [[nodiscard]] bool can_get_deleted_block_from_this() const {
                auto size = *deleted_blocks_size();
                return size > 0 && *begin_record_block_index() == 0;
            };

            [[nodiscard]] bool can_add_deleted_block() const {
                return rest_size() >= 4;
            };

            [[nodiscard]] bool have_extra_data() const {
                return *(uint8 *) buffer & 4;
            };

            [[nodiscard]] uint16* extra_data_size() const {
                return (uint16 *) move_ptr(buffer, 17);
            };

            [[nodiscard]] void* extra_data() const {
                return move_ptr(buffer, 19);
            };

            [[nodiscard]] uint32 rest_size() const {
                uint32 ret = BLOCK_SIZE - (have_extra_data() ? 2 + *extra_data_size() : 0) - 17;
                if (ret / 4 < *deleted_blocks_size()) return ret % 4;
                return ret - *deleted_blocks_size() * 4;
            }

            uint32 get_new_record_block_index() {
                return *get_pos_deleted_block(*deleted_blocks_size() - 1);
            }

            void add_deleted_block(uint32 index) {
                auto ptr = deleted_blocks_size();
                *get_pos_deleted_block(*ptr) = index;
                ++*ptr;
            };

            uint32 pop_deleted_block_index() {
                auto ptr = deleted_blocks_size();
                ptr = get_pos_deleted_block(--*ptr);
                uint32 result = *ptr;
                *ptr = 0;
                return result;
            };

            void set_to_head_block() {
                std::memset(buffer, 0, BLOCK_SIZE);
                *(uint8 *) buffer = 0;
            };

            void set_have_extra_data() {
                *(uint8 *) buffer |= 4;
            };

            void* buffer = nullptr;
        };

        struct RecordBlockHelper {
            [[nodiscard]] uint32* last_index() const {
                return (uint32 *) move_ptr(buffer, 1);
            };

            [[nodiscard]] uint16* deleted_size() const {
                return (uint16 *) move_ptr(buffer, 5);
            };

            [[nodiscard]] bool can_get_deleted_block() const {
                return *deleted_size() > 0;
            };

            [[nodiscard]] uint32* get_pos_deleted_block(uint16 index) const {
                return (uint32 *) move_ptr(buffer, 7 + index * 4);
            }

            [[nodiscard]] bool can_add_deleted_block() const {
                return *deleted_size() < (BLOCK_SIZE - 7) / 4;
            };

            void add_deleted_block(uint32 index) {
                auto ptr = deleted_size();
                *get_pos_deleted_block(*ptr) = index;
                ++*ptr;
            };

            uint32 pop_deleted_block_index() {
                auto ptr = deleted_size();
                auto p = get_pos_deleted_block(--*ptr);
                uint32 result = *p;
                *p = 0;
                return result;
            };

            uint32 get_new_record_block_index() {
                return *get_pos_deleted_block(*deleted_size() - 1);
            }

            void set_to_record_deleted_block() {
                std::memset(buffer, 0, BLOCK_SIZE);
                *(uint8 *) buffer = 1;
            };

            void* buffer = nullptr;
        };

        struct DataBlockHelper {
            [[nodiscard]] uint32* prev_index() const {
                return (uint32 *) move_ptr(buffer, 1);
            };

            [[nodiscard]] uint32* next_index() const {
                return (uint32 *) move_ptr(buffer, 5);
            };

            [[nodiscard]] uint16* get_size() const {
                return (uint16 *) move_ptr(buffer, 9);
            };

            static constexpr uint32 BeginPos = 11;

            void set_to_data_block() {
                std::memset(buffer, 0, BLOCK_SIZE);
                *(uint8 *) buffer = 2;
            };

            void* buffer = nullptr;
        };

        struct IndexBlockHelper {
            [[nodiscard]] uint16* get_size() const {
                return (uint16 *) move_ptr(buffer, 1);
            };

            static constexpr uint32 BeginPos = 3;

            void set_to_index_block() {
                std::memset(buffer, 0, BLOCK_SIZE);
                *(uint8 *) buffer = 3;
            };

            void* buffer = nullptr;
        };

    };

}

#endif

#endif //BASE_INTERPRETER_HPP
