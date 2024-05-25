//
// Created by taganyer on 3/22/24.
//

#ifndef NET_FILEPOOL_HPP
#define NET_FILEPOOL_HPP

#ifdef NET_FILEPOOL_HPP

#include <map>
#include <functional>
#include "Base/Condition.hpp"
#include "Base/Detail/oFile.hpp"
#include "Net/monitors/Selector.hpp"


namespace Net {

    class Controller;

    struct error_mark;

    /// 大文件发送线程，提供了错误处理回调函数。
    class FilePool {
    public:
        static const int Default_timeWait;

        /*
         * 传入函数会在文件传输完毕后或发生错误时调用。
         * error_mark 可能传入的值有：
         *      error_types::Null
         *      error_types::Sendfile
         *      error_types::ErrorEvent
         *      error_types::UnexpectedShutdown
         * off_t 代表文件目前要发送的位置。
         */
        using Callback = std::function<void(error_mark, off_t)>;

        FilePool();

        ~FilePool();

        /// 传入的最好是非阻塞的套接字，否则可能会导致线程阻塞，影响其他文件发送。
        void add_file(int socket, Base::oFile &&file, Callback callback,
                      off_t begin, uint64 total_size, uint64 block_size);

        void shutdown();

        [[nodiscard]] bool running() const { return run; };

    private:
        struct Data {
            off_t ptr;
            uint64 total;
            uint64 block;
            Base::oFile file;
            Callback callback;
        };

        Base::Condition _con;

        Base::Mutex _mutex;

        Selector _sel;

        std::multimap<int, Data> _store;

        std::vector<std::pair<int, Data>> _prepare;

        volatile bool run = false, shut = false;

        void thread_start();

        void begin();

        static bool send(int socket, Data &data);

        void thread_close();

    };

}

#endif

#endif //NET_FILEPOOL_HPP
