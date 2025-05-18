//
// Created by taganyer on 24-10-19.
//

#include "../LinkLogInterpreter.hpp"
#include <map>
#include "tinyBackend/Base/GlobalObject.hpp"
#include "tinyBackend/Base/Detail/FileDir.hpp"
#include "tinyBackend/LogSystem/linkLog/LinkLogMessage.hpp"

using namespace LogSystem;

using namespace Base;

constexpr char filename_name[] = "link_filename.name_index";

constexpr char record_file_suffix[] = ".record";

constexpr char link_index_file_name[] = "link_index.link_index";

constexpr char node_deletion_file_name[] = "node_deletion.deletion_index";

constexpr char link_log_file_suffix[] = ".link_log";

static constexpr uint16 header_offset0 = sizeof(OperationType);

static constexpr uint16 header_offset1 = sizeof(OperationType) + sizeof(uint16);

static constexpr uint16 header_offset2 = sizeof(OperationType) + sizeof(uint16) + sizeof(TimeInterval);

static constexpr uint16 header_offset3 = sizeof(OperationType) + sizeof(uint16) + sizeof(TimeInterval) + sizeof(
    uint32);

#define CHECK(expr, error_handle) \
if (unlikely(!(expr))) { G_ERROR << "LinkLogStorage: " #expr " failed in " << __FUNCTION__; error_handle; }


static int record_file_filter(const dirent *entry) {
    int len = entry->d_reclen;
    constexpr int suffix_size = sizeof(record_file_suffix) - 1;
    if (len < Time::Time_us_format_len + suffix_size) return 0;
    auto ptr = entry->d_name + Time::Time_us_format_len;
    for (int i = 0; i < suffix_size; ++i)
        if (ptr[i] != record_file_suffix[i])
            return 0;
    return 1;
}

LinkLogEncoder::LinkLogEncoder(std::string dictionary_path) :
    _dictionary_path(std::move(dictionary_path)) {
    if (_dictionary_path.back() == '/')
        _dictionary_path.pop_back();
    DirentArray array(_dictionary_path.c_str(), record_file_filter);
    _dictionary_path.push_back('/');
    for (int i = 0; i < array.size(); ++i) {
        _file_names.emplace_back(_dictionary_path + array[i]->d_name);
    }
    if (array.size() <= 0) return;
    _record_file.open(_file_names.back().c_str(), true, true);
    struct stat st {};
    if (!_record_file.get_stat(&st)) {
        _record_file.close();
        return;
    }
    _current_size = st.st_size;
}

uint32 LinkLogEncoder::register_logger(LinkNodeType type_,
                                       const LinkServiceID& service_,
                                       const LinkNodeID& node_,
                                       const LinkNodeID& parent_node_,
                                       TimeInterval time_,
                                       TimeInterval parent_init_time_,
                                       TimeInterval expire_time_,
                                       void *dest, uint32 limit) {
    if (limit < sizeof(Register_Logger)) return 0;
    Register_Logger logger(type_, service_, node_, parent_node_,
                           time_, parent_init_time_, expire_time_);
    std::memcpy(dest, &logger, limit);
    return sizeof(Register_Logger);
}

uint32 LinkLogEncoder::create_logger(LinkNodeType type_,
                                     const LinkServiceID& service_,
                                     const LinkNodeID& node_,
                                     const LinkNodeID& parent_node_,
                                     TimeInterval init_time_,
                                     TimeInterval parent_init_time_,
                                     void *dest, uint32 limit) {
    if (limit < sizeof(Create_Logger)) return 0;
    Create_Logger logger(type_, service_, node_, parent_node_, init_time_, parent_init_time_);
    std::memcpy(dest, &logger, limit);
    return sizeof(Create_Logger);
}

uint32 LinkLogEncoder::end_logger(const LinkServiceID& service_,
                                  const LinkNodeID& node_,
                                  TimeInterval init_time_,
                                  TimeInterval end_time_,
                                  void *dest, uint32 limit) {
    if (limit < sizeof(End_Logger)) return 0;
    End_Logger logger(service_, node_, init_time_, end_time_);
    std::memcpy(dest, &logger, limit);
    return sizeof(End_Logger);
}

bool LinkLogEncoder::can_write_log(uint16 size, uint32 limit) {
    return size <= MAX_USHORT - sizeof(Link_Log_Header)
        && sizeof(Link_Log_Header) + size <= limit;
}

uint16 LinkLogEncoder::write_log(const LinkServiceID& service_,
                                 TimeInterval node_init_time_,
                                 const LinkNodeID& node_,
                                 TimeInterval time_,
                                 LogRank rank_,
                                 const void *data, uint16 size,
                                 void *dest) {
    Link_Log_Header header(size, service_, node_init_time_, node_, time_, rank_);
    std::memcpy(dest, &header, sizeof(Link_Log_Header));
    std::memcpy((char *) dest + sizeof(Link_Log_Header), data, size);
    return sizeof(Link_Log_Header) + header.log_size();
}

uint32 LinkLogEncoder::write_to_file(const void *data, uint32 size) {
    if ((!_record_file.is_open() || _current_size >= RECORD_FILE_LIMIT_SIZE)
    && !open_new_file())
        return 0;
    uint32 written = _record_file.write(data, size);
    _current_size += written;
    return written;
}

void LinkLogEncoder::flush() const {
    _record_file.flush_to_disk();
}

bool LinkLogEncoder::open_new_file() {
    if (_file_names.size() >= FILE_SIZE_LIMIT) {
        remove(_file_names.front().c_str());
        _file_names.erase(_file_names.begin());
    }
    TimeInterval now = Unix_to_now();
    std::string filename = get_record_file_name(now);
    _record_file.open(filename.c_str(), true, true);
    if (!_record_file.is_open()) return false;
    _current_size = 0;
    _file_names.emplace_back(std::move(filename));
    return true;
}

std::string LinkLogEncoder::get_record_file_name(TimeInterval file_init_time) const {
    std::string path = _dictionary_path
        + to_string(file_init_time, true)
        + record_file_suffix;
    return path;
}

LinkLogStorage::LinkLogStorage(std::string dictionary_path, ScheduledThread& scheduled_thread) :
    _dictionary_path(dictionary_path.back() == '/' ? std::move(dictionary_path) : dictionary_path + '/'),
    _filename_file(scheduled_thread, filename_file_name().c_str(), FILENAME_BUFFER_POOL_SIZE),
    _indexes(scheduled_thread, index_file_name().c_str(), LINK_INDEX_BUFFER_POOL_SIZE),
    _node_deletion(scheduled_thread, deletion_file_name().c_str(), NODE_DELETION_BUFFER_POOL_SIZE),
    _cache(*this) {
    auto [results] = _filename_file.search_from_end(1);
    if (results.empty()) {
        results.emplace_back();
        results.back().value = results.back().key = Unix_to_now();
        bool success = _filename_file.insert(results.back().key, results.back().value);
        assert(success);
    }
    std::string path = get_log_name(results.back().key);
    CHECK(_logs.open(path.c_str(), true), return);
    struct stat s {};
    CHECK(_logs.get_stat(&s), return)
    _records.emplace_back();
    _records.back().file = results.back().key;
    _records.back().latest_time = TimeInterval { results.back().value };
    _records.back().written = _records.back().index = s.st_size;
}

bool LinkLogStorage::create_logger(const LinkServiceID& service, const LinkNodeID& node,
                                   TimeInterval node_init_time) {
    Index_Key key(service, node_init_time, node);
    Index_Value val;
    val.latest_file() = TimeInterval { 0 };
    val.latest_index() = MAX_UINT;
    Lock l(_mutex);
    CHECK(_indexes.insert(key, val), return false);
    return true;
}

void LinkLogStorage::update_logger(const LinkServiceID& service,
                                   const LinkNodeID& node,
                                   TimeInterval parent_init_time,
                                   const LinkNodeID& parent,
                                   LinkNodeType type,
                                   TimeInterval node_init_time) {
    Index_Key key(service, node_init_time, node);
    Lock l(_mutex);
    auto val_ptr = _cache.get(key);
    val_ptr->parent_init_time() = parent_init_time;
    val_ptr->parent_node() = parent;
    val_ptr->type() = type;
    _cache.put(key, true);
}

bool LinkLogStorage::create_logger(const LinkServiceID& service,
                                   const LinkNodeID& node,
                                   TimeInterval parent_init_time,
                                   const LinkNodeID& parent,
                                   LinkNodeType type,
                                   TimeInterval node_init_time) {
    Index_Key key(service, node_init_time, node);
    Index_Value val(parent_init_time, parent, type, TimeInterval {}, MAX_UINT);
    Lock l(_mutex);
    CHECK(_indexes.insert(key, val), return false);
    return true;
}

void LinkLogStorage::end_logger(const LinkServiceID& service,
                                const LinkNodeID& node,
                                TimeInterval node_init_time) {
    Index_Key key(service, node_init_time, node);

    Lock l(_mutex);
    CHECK(_node_deletion.insert(Unix_to_now(), key),)
}

void LinkLogStorage::add_record(uint32 size) {
    Lock l(_mutex);
    if (_records.empty() || _records.back().index >= LOG_FILE_LIMIT_SIZE)
        _records.emplace_back();
    _records.back().index += size;
}

void LinkLogStorage::handle_a_log(Link_Log_Header& header, const RingBuffer& buf) {
    Lock l(_mutex);
    Index_Key& key = *(Index_Key *) &header.service();
    auto val_ptr = _cache.get(key);
    buf.change_written(header_offset1, &val_ptr->latest_file(), sizeof(TimeInterval));
    buf.change_written(header_offset2, &val_ptr->latest_index(), sizeof(uint32));
    val_ptr->latest_file() = _records.back().file;
    val_ptr->latest_index() = _records.back().index;
    _cache.put(key, true);
}

uint32 LinkLogStorage::write_to_file(const RingBuffer& buf, uint32 size) {
    assert(buf.readable_len() >= size);
    Lock l(_mutex);
    flush_cache();
    uint32 written = 0;
    for (int i = 0; i < _records.size() && size; ++i) {
        if (i > 0) open_new_log_file(_records[i], _records[i - 1].latest_time);
        auto can_write = std::min(_records[i].index - _records[i].written, size);
        auto arr = buf.read_array(can_write);
        auto written_ = _logs.write(arr);
        CHECK(written_ == can_write, return written)
        buf.read_advance(written_);
        size -= written_;
        written += written_;
        _records[i].written += written_;
        assert(_records[i].index == _records[i].written);
    }
    _logs.flush();
    return written;
}

void LinkLogStorage::delete_oldest_files(uint32 size) {
    Lock l(_mutex);
    auto [results] = _filename_file.search_from_begin(size);
    if (results.size() < 2) return;

    for (auto [k, v] : results) {
        std::string path = get_log_name(k);
        CHECK(remove(path.c_str()) == 0,)
    }
    _filename_file.erase(results.front().key,
                         TimeInterval { results.back().key.nanoseconds + 1 });
    for (auto [k, v] : results) {
        assert(_filename_file.find(k).key == 0);
    }
    G_INFO << "FileNameIndex erased " << results.size() << " old files.";

    TimeInterval end_time(results.back().value);
    results = {};
    uint32 delete_size = 0;
    auto [deletions] = _node_deletion.below(end_time, 256);
    while (!deletions.empty()) {
        for (auto [k, v] : deletions) {
            _cache.erase(v);
        }
        _node_deletion.erase(deletions.front().key,
                             TimeInterval { deletions.back().key.nanoseconds + 1 });
        for (auto [k, v] : deletions) {
            assert(_node_deletion.find(k).key == 0);
        }
        delete_size += deletions.size();
        if (deletions.size() < 256) break;
        deletions = {};
        auto [deletions] = _node_deletion.below(end_time, 256);
    }
    G_INFO << "Delete " << delete_size << " link nodes.";
}

LinkLogStorage::QuerySet*
LinkLogStorage::get_query_set(const LinkServiceID& id, const NodeFilter& filter) {
    Lock l(_mutex);
    flush_cache();
    LinkServiceID end(id);
    char *ptr = end.data() + sizeof(LinkServiceID) - 1;
    for (; ptr >= end.data(); --ptr) {
        if (*ptr != (char) 256) {
            ++*ptr;
            break;
        }
        *ptr = 0;
    }
    Index_Key k_begin, k_end;
    k_begin.service() = id;
    k_end.service() = end;

    auto point = new QuerySet();
    auto [results] = ptr >= end.data() ?
                         _indexes.find(k_begin, k_end) :
                         _indexes.above_or_equal(k_begin);
    if (results.empty()) {
        destroy_query_set(point);
        return nullptr;
    }
    for (auto& [k, v] : results) {
        assert((k > k_begin || k == k_begin) && k < k_end);
        if (filter && !filter(k, v) || v.latest_index() == MAX_UINT) continue;
        point->_log_location.emplace(v.latest_file(), v.latest_index());
    }
    return point;
}

void LinkLogStorage::destroy_query_set(void *set) {
    Global_Logger.flush();
    delete static_cast<QuerySet *>(set);
}

std::pair<uint32, LinkLogStorage::QuerySet *>
LinkLogStorage::query(void *dest, uint32 limit, QuerySet *point) {
    Lock l(_mutex);
    if (!point) return std::make_pair(0, nullptr);
    uint32 written = 0;
    auto ptr = (char *) dest;

    while (limit > 0 && !point->_log_location.empty()) {
        uint32 log_size = 0;
        int ret = querying_a_log(point, ptr, limit, log_size);
        if (ret < 0) return std::make_pair(written, nullptr);
        if (ret > 0) return std::make_pair(written, point);
        written += log_size;
        ptr += log_size;
        limit -= log_size;
    }

    if (!point->_log_location.empty())
        return std::make_pair(written, point);
    destroy_query_set(point);
    return std::make_pair(written, nullptr);
}

std::pair<uint32, LinkLogStorage::QuerySet *>
LinkLogStorage::query(RingBuffer& buf, QuerySet *point) {
    if (!point) return std::make_pair(0, nullptr);
    Lock l(_mutex);
    uint32 written = 0;

    while (buf.writable_len() > sizeof(Link_Log_Header)
    && !point->_log_location.empty()) {
        uint32 log_size = 0;
        int ret = querying_a_log(point, buf, log_size);
        if (ret < 0) return std::make_pair(written, nullptr);
        if (ret > 0) return std::make_pair(written, point);
        buf.write_advance(log_size);
        written += log_size;
    }

    if (!point->_log_location.empty())
        return std::make_pair(written, point);
    destroy_query_set(point);
    return std::make_pair(written, nullptr);
}

void LinkLogStorage::flush_log_file() {
    Lock l(_mutex);
    _logs.flush_to_disk();
}

void LinkLogStorage::flush_file_name() {
    Lock l(_mutex);
    _filename_file.impl().flush();
}

void LinkLogStorage::flush_index_file() {
    Lock l(_mutex);
    flush_cache();
    _indexes.impl().flush();
}

void LinkLogStorage::flush_deletion_file() {
    Lock l(_mutex);
    _node_deletion.impl().flush();
}

bool LinkLogStorage::CacheHelper::can_create(const Key& key) const {
    return _interpreter._cache.total_size() < CACHE_LIMIT_SIZE;
}

LinkLogStorage::CacheHelper::Value
LinkLogStorage::CacheHelper::create(const Key& key) const {
    auto [k, v] = _interpreter._indexes.find(key);
    CHECK(k == key,);
    return v;
}

void LinkLogStorage::CacheHelper::update(const Key& key, Value& value) const {
    CHECK(_interpreter._indexes.update(key, value),);
}

void LinkLogStorage::CacheHelper::erase(const Key& key) const {
    CHECK(_interpreter._indexes.erase(key),);
}

void LinkLogStorage::flush_cache() {
    if (_cache.need_update_size())
        _cache.update_all();
}

std::string LinkLogStorage::filename_file_name() const {
    return _dictionary_path + filename_name;
}

std::string LinkLogStorage::index_file_name() const {
    return _dictionary_path + link_index_file_name;
}

std::string LinkLogStorage::deletion_file_name() const {
    return _dictionary_path + node_deletion_file_name;
}

LinkLogStorage::LinkLogReadFile
LinkLogStorage::get_log_file(TimeInterval file_init_time) const {
    std::string path = get_log_name(file_init_time);
    LinkLogReadFile file(path.c_str(), true);
    return file;
}

std::string LinkLogStorage::get_log_name(TimeInterval file_init_time) const {
    std::string path = _dictionary_path
        + to_string(file_init_time, true)
        + link_log_file_suffix;
    return path;
}

void LinkLogStorage::open_new_log_file(const Record& record, TimeInterval end_time) {
    auto [results] = _filename_file.search_from_end(1);
    if (!results.empty()) {
        _filename_file.update(results.front().key, end_time);
    }
    std::string path = get_log_name(record.file);
    _logs = LinkLogWriteFile(path.c_str(), true, true);
    CHECK(_logs.is_open(), return)
    bool success = _filename_file.insert(record.file, record.latest_time);
    assert(success);
}

bool LinkLogStorage::prepare_read_a_log(QuerySet *point, char *msg) const {
    auto [file_name, index] = point->_log_location.top();
    if (file_name != point->_current_file) {
        point->_file = get_log_file(file_name);
        CHECK(point->_file.is_open(),
              destroy_query_set(point); return false)
        point->_pos = 0;
        point->_current_file = file_name;
    }

    CHECK(point->_file.seek_cur(index - point->_pos),
          destroy_query_set(point); return false)
    point->_pos = index;
    CHECK(point->_file.read(header_offset3, msg) == header_offset3,
          destroy_query_set(point); return false)
    point->_pos += header_offset3;
    return true;
}

int LinkLogStorage::querying_a_log(QuerySet *point, char *ptr,
                                   uint32 limit, uint32& written) const {
    char msg[header_offset3] {};
    if (!prepare_read_a_log(point, msg)) return -1;

    uint16 log_size = *(uint16 *) (msg + header_offset0);
    if (log_size + sizeof(Link_Log_Header) > limit) return 1;

    std::memcpy(ptr, msg, header_offset3);
    uint32 need_write = log_size + sizeof(Link_Log_Header) - header_offset3;
    CHECK(point->_file.read(need_write, ptr + header_offset3) == need_write,
          destroy_query_set(point); return -1)
    point->_pos += need_write;
    written = sizeof(Link_Log_Header) + log_size;

    point->_log_location.pop();
    if (*(uint32 *) (msg + header_offset2) != MAX_UINT) {
        point->_log_location.emplace(*(TimeInterval *) (msg + header_offset1),
                                     *(uint32 *) (msg + header_offset2));
    }
    return 0;
}

int LinkLogStorage::querying_a_log(QuerySet *point, RingBuffer& buf, uint32& written) const {
    char msg[header_offset3] {};
    if (!prepare_read_a_log(point, msg)) return -1;

    uint16 log_size = *(uint16 *) (msg + header_offset0);
    if (log_size + sizeof(Link_Log_Header) > buf.writable_len()) return 1;

    uint32 need_write = log_size + sizeof(Link_Log_Header) - buf.try_write(&msg, header_offset3, 0);
    CHECK(point->_file.read(buf.write_array(need_write, header_offset3)) == need_write,
          destroy_query_set(point); return -1)
    point->_pos += need_write;
    written = sizeof(Link_Log_Header) + log_size;

    point->_log_location.pop();
    if (*(uint32 *) (msg + header_offset2) != MAX_UINT) {
        point->_log_location.emplace(*(TimeInterval *) (msg + header_offset1),
                                     *(uint32 *) (msg + header_offset2));
    }
    return 0;
}
