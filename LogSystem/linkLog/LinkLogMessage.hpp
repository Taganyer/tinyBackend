//
// Created by taganyer on 24-10-17.
//

#ifndef LOGSYSTEM_LINKLOGMESSAGE_HPP
#define LOGSYSTEM_LINKLOGMESSAGE_HPP

#ifdef LOGSYSTEM_LINKLOGMESSAGE_HPP

#include <bits/stl_algo.h>
#include "LinkLogOperation.hpp"

namespace LogSystem {

    template <typename... Types>
    static constexpr auto max_size() {
        return std::max({ sizeof(Types)... });
    };

    class LinkLogMessage {
    public:
        enum Type {
            Invalid,
            ClearBuffer,
            TimeOut,
            ShutDownServer,
            CentralOffline,
            NodeOffline
        };

        static const char* getTypename(Type type) {
            constexpr const char *name[] {
                "[Invalid]",
                "[ClearBuffer]",
                "[TimeOut]",
                "[ShutDownServer]",
                "[CentralOffline]",
                "[NodeOffline]"
            };
            return name[type];
        };

        struct ClearBuffer_ {
            uint32 size = 0;
            bool flushed = false;
            uint32 begin = 0;
            void *buf_ptr = nullptr;
        };

        struct TimeOut_ {
            Error_Logger time_out;
        };

        struct NodeOffline_ {
            bool sent = false;
        };

        Type type = Invalid;

    private:
        static constexpr uint32 size = max_size<ClearBuffer_, TimeOut_,
                                                NodeOffline_>();

    public:
        char arr[size] {};

        LinkLogMessage() = default;

        LinkLogMessage(const LinkLogMessage&) = default;

        explicit LinkLogMessage(Type type) : type(type) {};

        template <typename T>
        T& get() { return reinterpret_cast<T&>(arr); };

        template <typename T>
        const T& get() const { return reinterpret_cast<const T&>(arr); };
    };

}

#endif

#endif //LOGSYSTEM_LINKLOGMESSAGE_HPP
