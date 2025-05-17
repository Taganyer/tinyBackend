//
// Created by taganyer on 3/22/24.
//

#include "../FilePool.hpp"
#include "tinyBackend/Base/Thread.hpp"
#include "tinyBackend/Base/SystemLog.hpp"
#include "tinyBackend/Net/functions/Interface.hpp"


using namespace Base;

using namespace Net;

const int FilePool::Default_timeWait = 10;

FilePool::FilePool() {
    thread_start();
}

FilePool::~FilePool() {
    if (running()) {
        shutdown();
        Lock l(_mutex);
        _con.wait(l, [this] { return shut; });
    }
}

void FilePool::add_file(int socket, oFile&& file, Callback callback,
                        off_t begin, uint64 total_size, uint64 block_size) {
    if (!running()) return;
    Lock l(_mutex);
    _prepare.emplace_back(socket, Data { begin, total_size, block_size,
                                         std::move(file), std::move(callback) });
    _con.notify_one();
}

void FilePool::shutdown() {
    run = false;
    _con.notify_one();
}

void FilePool::thread_start() {
    Thread thread([this] {
        std::vector<Event> events;
        while (run) {
            begin();
            if (!run) break;
            _sel.get_aliveEvent(Default_timeWait, events);
            for (auto const& e : events) {
                auto iter = _store.find(e.fd);
                assert(iter != _store.end());
                if (e.canWrite() && send(e.fd, iter->second)) {
                    _sel.remove_fd(e.fd, false);
                    _store.erase(iter);
                }
                if (e.hasError()) {
                    iter->second.callback({ error_types::ErrorEvent, -1 }, iter->second.ptr);
                    _sel.remove_fd(e.fd, false);
                    _store.erase(iter);
                }
            }
            events.clear();
        }
        thread_close();
    });

    thread.start();
}

void FilePool::begin() {
    Lock l(_mutex);
    _con.wait(l, [this] {
        return !_store.empty()
            || !_prepare.empty() || !run;
    });
    if (!_prepare.empty()) {
        for (auto& data : _prepare) {
            _sel.add_fd({ data.first, Event::Write | Event::Error });
            _store.emplace(std::move(data));
        }
        _prepare.clear();
    }
}

bool FilePool::send(int socket, FilePool::Data& data) {
    uint64 size = std::min(data.total, data.block);
    int64 ret = ops::sendfile(socket, data.file.get_fd(), &data.ptr, size);
    if (ops::sendfile(socket, data.file.get_fd(), &data.ptr, size) == -1) {
        G_ERROR << "FilePool: send " << data.file.get_fd() << " error " << errno;
        data.callback({ error_types::Sendfile, errno }, data.ptr);
    } else if ((data.total -= ret) == 0) {
        G_TRACE << "FilePool: send " << data.file.get_fd() << " success.";
        data.callback({ error_types::Null, 0 }, data.ptr);
    } else return false;
    return true;
}

void FilePool::thread_close() {
    Lock l(_mutex);
    G_INFO << "FilePool close," << _store.size() + _prepare.size() << " files force close.";
    for (auto const& [socket, data] : _store)
        data.callback({ error_types::UnexpectedShutdown, 0 }, data.ptr);
    for (auto const& [socket, data] : _prepare)
        data.callback({ error_types::UnexpectedShutdown, 0 }, data.ptr);
    _store.clear();
    _prepare.clear();
    _sel.remove_all();
    shut = true;
    _con.notify_all();
}
