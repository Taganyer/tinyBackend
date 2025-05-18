//
// Created by taganyer on 24-7-23.
//

#include "../logSystem_test.hpp"

#include <iostream>
#include <memory>
#include <vector>
#include <tinyBackend/Base/GlobalObject.hpp>
#include <tinyBackend/Base/Thread.hpp>
#include <tinyBackend/Base/Time/TimeInterval.hpp>
#include <tinyBackend/LogSystem/linkLog/Identification.hpp>
#include <tinyBackend/LogSystem/linkLog/LinkLogCenter.hpp>
#include <tinyBackend/LogSystem/linkLog/LinkLogger.hpp>
#include <tinyBackend/LogSystem/linkLog/LinkLogHandler.hpp>
#include <tinyBackend/LogSystem/linkLog/LinkLogOperation.hpp>
#include <tinyBackend/LogSystem/linkLog/LinkLogServer.hpp>
#include <tinyBackend/Net/InetAddress.hpp>
#include <tinyBackend/Net/error/error_mark.hpp>

using namespace std;
using namespace Base;
using namespace Net;
using namespace LogSystem;

namespace Test {

    void log_test() {
#ifdef GLOBAL_OBJETS
        string file(PROJECT_ROOT_DIR "/resource/poem.txt");
        vector<Thread> pool;
        SystemLog log1(Global_ScheduledThread, Global_BufferPool, GLOBAL_LOG_PATH, INFO);
        SystemLog log2(Global_ScheduledThread, Global_BufferPool, GLOBAL_LOG_PATH, INFO);
        SystemLog log3(Global_ScheduledThread, Global_BufferPool, GLOBAL_LOG_PATH, INFO);
        for (int i = 1; i < 10; ++i) {
            pool.emplace_back([&file, &log1, &log2, &log3, i]() {
                iFile in(file.c_str(), true);
                while (in) {
                    auto t = in.getline();
                    if (i % 3 == 0)
                        log1.push(INFO, t.data(), t.size());
                    else if (i % 3 == 1)
                        log2.push(INFO, t.data(), t.size());
                    else
                        log3.push(INFO, t.data(), t.size());
                }
            });
        }
        TimeInterval begin = Unix_to_now();
        for (auto& t : pool) {
            t.start();
        }
        for (auto& t : pool) {
            t.join();
        }
        cout << (Unix_to_now() - begin).to_ms() << "ms" << endl;
#endif
    }


    class ServerHandler : public LinkLogServerHandler {
    public:
        void create_head_logger(LinkServiceID service, LinkNodeID node,
                                TimeInterval time) override {
            G_INFO << __FUNCTION__ << ' ' << service.data() << '|'
                    << node.data() << " init_time "
                    << to_string(time, true);
        };

        void register_logger(LinkServiceID service, LinkNodeID node,
                             LinkNodeID parent_node, LinkNodeType type,
                             TimeInterval time, TimeInterval expire_time) override {
            G_INFO << __FUNCTION__ << ' ' << service.data() << '|' << node.data()
                    << " parent " << parent_node.data() << " type " << NodeTypeName(type)
                    << " time " << to_string(time, true);
        };

        void create_logger(LinkServiceID service, LinkNodeID node,
                           TimeInterval init_time) override {
            G_INFO << __FUNCTION__ << ' ' << service.data() << '|'
                    << node.data() << " init_time "
                    << to_string(init_time, true);
        };

        void logger_end(LinkServiceID service, LinkNodeID node,
                        TimeInterval end_time) override {
            G_INFO << __FUNCTION__ << ' ' << service.data() << '|'
                    << node.data() << " end_time "
                    << to_string(end_time, true);
        };

        void handling_error(LinkServiceID service, LinkNodeID node,
                            TimeInterval time, LinkErrorType type) override {
            G_INFO << __FUNCTION__ << ' ' << service.data() << '|' << node.data()
                    << " time " << to_string(time, true)
                    << " type " << getLinkErrorTypeName(type);
        };

        void center_online(const Address& center_address) override {
            G_INFO << __FUNCTION__;
        };

        void center_offline() override {
            G_INFO << __FUNCTION__;
        };

        void center_error(error_mark mark) override {
            G_INFO << __FUNCTION__;
        };

    };

    class CenterHandler : public LinkLogCenterHandler {
    public:
        void create_head_logger(const Address& address, LinkServiceID service, LinkNodeID node,
                                TimeInterval time) override {
            G_DEBUG << __FUNCTION__ << ' ' << address.toIpPort()
                    << ' ' << service.data() << '|'
                    << node.data() << " init_time "
                    << to_string(time, true);
        };

        void register_logger(const Address& address, LinkServiceID service, LinkNodeID node,
                             LinkNodeID parent_node, LinkNodeType type,
                             TimeInterval time, TimeInterval expire_time) override {
            G_DEBUG << __FUNCTION__ << ' ' << address.toIpPort()
                    << ' ' << service.data() << '|' << node.data()
                    << " parent " << parent_node.data() << " type " << NodeTypeName(type)
                    << " time " << to_string(time, true);
        };

        void create_logger(const Address& address, LinkServiceID service, LinkNodeID node,
                           TimeInterval init_time) override {
            G_DEBUG << __FUNCTION__ << ' ' << address.toIpPort()
                    << ' ' << service.data() << '|'
                    << node.data() << " init_time "
                    << to_string(init_time, true);
        };

        void logger_end(const Address& address, LinkServiceID service, LinkNodeID node,
                        TimeInterval end_time) override {
            G_DEBUG << __FUNCTION__ << ' ' << address.toIpPort()
                    << ' ' << service.data() << '|'
                    << node.data() << " end_time "
                    << to_string(end_time, true);
        };

        void receive_log(const Address& address, Link_Log log) override {
            G_DEBUG << __FUNCTION__ << ' ' << address.toIpPort()
                    << ' ' << log.service.data() << '|'
                    << log.node.data() << " rank " << LogRankName(log.rank)
                    << ' ' << to_string(log.time, true)
                    << '[' << log.text << ']';
        };

        void handling_error(const Address& address, LinkServiceID service, LinkNodeID node,
                            TimeInterval time, LinkErrorType type) override {
            G_DEBUG << __FUNCTION__ << ' ' << address.toIpPort()
                    << ' ' << service.data() << '|' << node.data()
                    << " time " << to_string(time, true)
                    << " type " << getLinkErrorTypeName(type);
        };

        void node_online(const Address& address, TimeInterval time) override {
            G_DEBUG << __FUNCTION__ << ' ' << address.toIpPort()
                    << " at " << to_string(time, true);
        };

        void node_offline(const Address& address, TimeInterval time) override {
            G_DEBUG << __FUNCTION__ << ' ' << address.toIpPort()
                    << " at " << to_string(time, true);
        };

        bool node_timeout(const Address& address) override {
            G_DEBUG << __FUNCTION__ << ' ' << address.toIpPort();
            return true;
        };

        void node_error(const Address& address, error_mark mark) override {
            G_DEBUG << __FUNCTION__ << ' ' << address.toIpPort();
        };

    };

    class Searcher : public LinkLogSearchHandler {
    public:
        void receive_log(Link_Log log) override {
            G_WARN << log.service.data() << '|' << log.node.data()
                    << '{' << to_string(log.node_init_time, true) << '}'
                    << '(' << LogRankName(log.rank) << ')'
                    << " at " << to_string(log.time, true)
                    << '[' << log.text << ']';
        };

        bool node_filter(Index_Key key, Index_Value val) override {
            G_WARN << __FUNCTION__ << ": " << key.service().data() << '|' << key.node().data()
                    << " at " << to_string(key.init_time(), true)
                    << NodeTypeName(val.type()) << "----- parent: "
                    << val.parent_node().data() << " at "
                    << to_string(val.parent_init_time(), true);
            return true;
        };
    };

    class ReplayHandler : public LinkLogReplayHandler {
    public:
        void create_head_logger(LinkServiceID service, LinkNodeID node,
                                TimeInterval time) override {
            G_DEBUG << __FUNCTION__ << ' ' << service.data() << '|'
                    << node.data() << " init_time "
                    << to_string(time, true);
        };

        void register_logger(LinkServiceID service, LinkNodeID node,
                             LinkNodeID parent_node, LinkNodeType type,
                             TimeInterval time, TimeInterval expire_time) override {
            G_DEBUG << __FUNCTION__ << ' ' << service.data() << '|' << node.data()
                    << " parent " << parent_node.data() << " type " << NodeTypeName(type)
                    << " time " << to_string(time, true);
        };

        void create_logger(LinkServiceID service, LinkNodeID node,
                           TimeInterval init_time) override {
            G_DEBUG << __FUNCTION__ << ' ' << service.data() << '|'
                    << node.data() << " init_time "
                    << to_string(init_time, true);
        };

        void logger_end(LinkServiceID service, LinkNodeID node,
                        TimeInterval end_time) override {
            G_DEBUG << __FUNCTION__ << ' ' << service.data() << '|'
                    << node.data() << " end_time "
                    << to_string(end_time, true);
        };

        void receive_log(Link_Log log) override {
            G_DEBUG << __FUNCTION__ << ' ' << log.service.data() << '|'
                    << log.node.data() << " rank " << LogRankName(log.rank)
                    << ' ' << to_string(log.time, true)
                    << '[' << log.text << ']';
        };

        void handling_error(LinkServiceID service, LinkNodeID node,
                            TimeInterval time, LinkErrorType type) override {
            G_DEBUG << __FUNCTION__ << ' ' << service.data() << '|' << node.data()
                    << " time " << to_string(time, true)
                    << " type " << getLinkErrorTypeName(type);
        };

    };

    static LinkServiceID get_service_id(int index, int times) {
        assert(index >= 0 && index <= 99);
        assert(times >= 0 && times <= 99);
        char service_name[SERVICE_ID_SIZE] = "S_00_00";
        service_name[2] = index / 10 + '0';
        service_name[3] = index % 10 + '0';
        service_name[5] = times / 10 + '0';
        service_name[6] = times % 10 + '0';
        return LinkServiceID(service_name, SERVICE_ID_SIZE);
    }

    static LinkNodeID get_node_id(int index, int times) {
        assert(index >= 0 && index <= 99);
        assert(times >= 0 && times <= 99);
        char node_name[NODE_ID_SIZE] = "N_00_00";
        node_name[2] = index / 10 + '0';
        node_name[3] = index % 10 + '0';
        node_name[5] = times / 10 + '0';
        node_name[6] = times % 10 + '0';
        return LinkNodeID(node_name, NODE_ID_SIZE);
    }

    static void link_log_server(const InetAddress& address, int index);

#define USE_CENTER
#define USE_SERVER
// #define SEARCH
// #define REPLAY

    void link_log_test() {
        constexpr int servers = 2;
        vector<InetAddress> addresses;
        vector<Thread> threads;

#ifdef USE_CENTER
        string center_dic(GLOBAL_LOG_PATH "/link_log_center");
        shared_ptr<CenterHandler> handler = make_shared<CenterHandler>();
        LinkLogCenter center(handler, center_dic, Global_ScheduledThread);

#ifdef SEARCH
        Searcher searcher;
        for (int i = 0; i < servers; ++i) {
            auto service = get_service_id(i, 0);
            center.search_link(service, searcher);
        }
#endif

#endif

#ifdef USE_SERVER
        for (int i = 0; i < servers; ++i) {
            addresses.emplace_back(true, "127.0.0.1", 8888 + i);
            threads.emplace_back([i, &addresses]() {
                link_log_server(addresses[i], i);
            });
            threads.back().start();
        }
#endif

#ifdef USE_CENTER
#ifdef  USE_SERVER
        sleep(1);
        for (int i = 0; i < servers; ++i) {
            center.add_server(addresses[i]);
        }
#endif
#endif

#ifdef USE_SERVER
        for (int i = 0; i < servers; ++i) {
            threads[i].join();
        }
#endif

#ifdef REPLAY
        ReplayHandler replay;
        LinkLogCenter::replay_history(replay,
                                        // 需要填充.link_log的文件路径);
#endif
    }

    static void link_log_server(const InetAddress& address, int index) {
        shared_ptr<ServerHandler> handler = make_shared<ServerHandler>();
        string server_dic(GLOBAL_LOG_PATH "/link_log_server");
        server_dic.append(to_string(index));
        LinkLogServer server(address, handler, server_dic);
#ifdef USE_CENTER
        sleep(2);
#endif
        constexpr int times = 1, loops = 1;
        for (int i = 0; i < times; ++i) {
            int j = 0;

            auto service = get_service_id(index, i);
            LinkLogger header(TRACE, service, get_node_id(index, j), false, 1_s, server);
            DEBUG(header) << "Link log server started";

            for (++j; j < 1 + 4 * loops; j += 4) {
                auto fork_node1 = get_node_id(index, j);
                header.fork(fork_node1, 1_s);
                LinkLogger fork1(TRACE, header, fork_node1, 1_s);
                INFO(fork1) << "fork1 log: " << to_string(j);

                auto fork_node2 = get_node_id(index, j + 1);
                header.fork(fork_node2, 1_s);
                sleep(2);
                LinkLogger fork2(TRACE, header, fork_node2, 1_s);
                INFO(fork1) << "fork2 log: " << to_string(j + 1);

                auto follow_node = get_node_id(index, j + 2);
                fork1.follow(follow_node, 1_s);
                LinkLogger follow(TRACE, fork1, follow_node, 1_s);
                INFO(follow) << "follow log: " << to_string(j + 2);

                auto decision_node = get_node_id(index, j + 3);
                follow.decision(decision_node, 1_s);
                if ((j - 1) / 4 % 2) {
                    LinkLogger decision(TRACE, follow, decision_node, 1_s);
                    INFO(decision) << "decision log: " << to_string(j + 3);
                }
            }
        }

    }

}
