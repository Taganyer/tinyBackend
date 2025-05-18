//
// Created by taganyer on 24-2-7.
//

#include "../base_test.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include <tinyBackend/Base/LinkedThreadPool.hpp>
#include <tinyBackend/Base/GlobalObject.hpp>
#include <tinyBackend/Base/Thread.hpp>
#include <tinyBackend/Base/ThreadPool.hpp>
#include <tinyBackend/Base/BPTree_impls/BPTree.hpp>
#include <tinyBackend/Base/BPTree_impls/BPTree_impl.hpp>
#include <tinyBackend/Base/Buffer/BufferPool.hpp>
#include <tinyBackend/Base/Time/Timer.hpp>
using namespace std;
using namespace Base;

namespace Test {

    static void exit_test_fun() {
        cout << "exit normal" << endl;
    }

    void CurrentThreadTest() {
        CurrentThread::set_error_message_file(fopen("error.log", "w"));
        Thread t([] {
            CurrentThread::set_local_terminal_function([] (CurrentThread::ExceptionPtr) {
                std::cout << "local" << std::endl;
            });
            throw exception(); // 测试异常退出。
        });
        t.start();

        CurrentThread::set_global_terminal_function([] (CurrentThread::ExceptionPtr) {
            std::cout << "global" << std::endl;
        });
        cout << CurrentThread::set_exit_function(exit_test_fun) << endl;

        t.join();
    }

    struct T {
        static inline atomic<int> t = 0;

        ~T() {
            t.fetch_add(1);
        }
    };

    void ThreadPool_test() {
        T::t.store(0);
        ThreadPool pool(3, 100000);
        atomic<int> reject = 0;
        auto fun = [] { T t; };

        cout << pool.get_core_threads() << " " << pool.get_max_queues() << endl;

        vector<Thread> at;
        auto start = Unix_to_now();

        for (int j = 0; j < 3; ++j) {
            at.emplace_back(string("Thread" + std::to_string(j)), [&fun, &pool, &reject] {
                vector<future<void>> arr;
                for (int i = 0; i < 100000; ++i) {
                    try {
                        arr.push_back(pool.submit_with_future(fun));
                    } catch (Exception&) {
                        reject.fetch_add(1);
                    }
                    if (i == 600) pool.stop();
                    if (i == 800) pool.start();
                }
                cout << "submit end" << endl;
                for (auto& t : arr) {
                    t.get();
                }
                cout << "thread end" << endl;
            });
        }

        Thread thread([&pool] {
            Timer timer(1_s, [&pool] {
                cout << "feel tasks: " << pool.get_free_tasks() << " state " << pool.stopping() << endl;
            });
            timer.invoke(10);
            pool.start();
        });

        thread.start();

        for (auto& t : at) {
            t.start();
        }

        for (auto& t : at) {
            t.join();
        }

        auto end = Unix_to_now();
        auto duration = end - start;
        cout << "执行时间: " << duration.to_ms() << " 毫秒" << std::endl;
        cout << "结果: " << T::t.load() << " 拒绝： " << reject.load() << endl;
    }

    void LinkedThreadPool_test() {
        T::t.store(0);
        LinkedThreadPool pool(3, 100000);
        atomic<int> reject = 0;
        auto fun = [] { T t; };

        cout << pool.get_core_threads() << " " << pool.get_max_queues() << endl;

        vector<Thread> at;
        auto start = Unix_to_now();

        for (int j = 0; j < 3; ++j) {
            at.emplace_back(string("Thread" + std::to_string(j)), [&fun, &pool, &reject] {
                vector<future<void>> arr;
                for (int i = 0; i < 100000; ++i) {
                    try {
                        arr.push_back(pool.submit_to_back_with_future(fun));
                    } catch (Exception&) {
                        reject.fetch_add(1);
                    }
                    if (i == 600) pool.stop();
                    if (i == 800) pool.start();
                }
                cout << "submit end" << endl;
                for (auto& t : arr) {
                    t.get();
                }
                cout << "thread end" << endl;
            });
        }

        Thread thread([&pool] {
            Timer timer(1_s, [&pool] {
                cout << "feel tasks: " << pool.get_free_tasks() << " state " << pool.stopping() << endl;
            });
            timer.invoke(10);
            pool.start();
        });

        thread.start();

        for (auto& t : at) {
            t.start();
        }

        for (auto& t : at) {
            t.join();
        }

        auto end = Unix_to_now();
        auto duration = end - start;
        cout << "执行时间: " << duration.to_ms() << " 毫秒" << std::endl;
        cout << "结果: " << T::t.load() << " 拒绝： " << reject.load() << endl;
    }

    void timer_test() {
        TimeInterval timeout(1500_ms);
        Timer timer(timeout, [&timer] {
            cout << "invoke" << ' ' << timer.get_timeout() << endl;
        });
        timer.invoke(3);
    }

    void BufferPool_test() {
        uint64 total_loop = 10, loop = 1000,
               success = 0, real_success = 0,
               total_quires = 0,
               total_block = BufferPool::round_size(BufferPool::BLOCK_SIZE << 12),
               max_block = total_block / loop * 4;

        TimeInterval total_time;
        for (int i = 0; i < total_loop; ++i) {
            BufferPool buffer_pool(total_block);
            vector<BufferPool::Buffer> t;
            t.reserve(loop);

            default_random_engine seed(time(nullptr));
            uniform_int_distribution engine(1, (int) max_block);

            // map<char *, uint64> store;

            TimeInterval start = Unix_to_now();
            for (int j = 0; j < loop; ++j) {
                auto s = engine(seed);
                total_quires += s;
                BufferPool::Buffer b = buffer_pool.get(s);
                if (b) {
                    // auto iter = store.emplace(b.data(), b.size());
                    // if (!iter.second) {
                    //     cout << iter.first->second << ' ' << b.size() << endl;
                    //     exit(1);
                    // }
                    success += s;
                    real_success += b.size();
                    t.push_back(std::move(b));
                }
            }
            TimeInterval end = Unix_to_now();
            total_time = total_time + end - start;
        }

        cout << "total_quires: " << total_quires << endl;
        cout << "success: " << success << endl;
        cout << "real_success: " << real_success << endl;
        cout << "pool size: " << (total_loop * total_block) << endl;

        cout << "except success rate: " <<
            (double) total_block / ((max_block + 1) / 2 * loop) * 100 << '%' << endl;

        cout << "success rate: " <<
            (double) success / total_quires * 100 << '%' << endl;

        cout << "real_success rate: " <<
            (double) real_success / total_quires * 100 << '%' << endl;

        cout << "pool has been allocated: " <<
            (double) real_success / (total_loop * total_block) * 100 << '%' << endl;

        cout << "cost time per BufferPool::get: " <<
            total_time.to_us() / (total_loop * loop) << "us" << endl;

        total_time.nanoseconds = 0;
        for (int i = 0; i < total_loop; ++i) {
            vector<char *> t;
            t.reserve(loop);

            default_random_engine seed(time(nullptr));
            uniform_int_distribution engine(1, (int) max_block);

            TimeInterval start = Unix_to_now();
            for (int j = 0; j < loop; ++j) {
                auto s = engine(seed);
                t.push_back(new char[s]);
            }
            TimeInterval end = Unix_to_now();
            total_time = total_time + end - start;

            for (auto p : t) delete[] p;
        }

        cout << "cost time per new: " <<
            total_time.to_us() / (total_loop * loop) << "us" << endl;
    }

    constexpr uint64 total_size = 1 << 20;

    static void BPTree_insert_test(BPTree<BPTree_impl<int, int>>& tree) {
        std::vector<int> arr(total_size);
        for (int i = 0; i < arr.size(); ++i)
            arr[i] = i;
        std::random_device rd; // 用于生成随机种子
        std::mt19937 g(rd());  // 初始化随机数生成器
        std::shuffle(arr.begin(), arr.end(), g);

        {
            TimeInterval insert_begin = Unix_to_now();
            for (auto i : arr) {
                if (!tree.insert(i, i))
                    cout << i << " insert failed" << endl;
            }
            TimeInterval insert_end = Unix_to_now();
            cout << "insert: " << (insert_end - insert_begin).to_ms() << " ms" << endl;
        }

        std::shuffle(arr.begin(), arr.end(), g);
        {
            TimeInterval search_begin = Unix_to_now();
            for (auto i : arr) {
                auto [k, v] = tree.find(i);
                if (v != i) {
                    cout << i << ": " << k << ' ' << v << endl;
                }
            }
            TimeInterval search_end = Unix_to_now();
            cout << "common search: " << (search_end - search_begin).to_ms() << " ms" << endl;
        }

        {
            TimeInterval search1_begin = Unix_to_now();
            auto [results] = tree.search_from_begin();
            TimeInterval search1_end = Unix_to_now();
            cout << "search from begin: " << (search1_end - search1_begin).to_ms() << " ms" << endl;
            bool _is_sorted = std::is_sorted(results.begin(), results.end(),
                                             [] (const Result_Impl<int, int>& l, const Result_Impl<int, int>& r) {
                                                 return l.key < r.key;
                                             });
            assert(_is_sorted);
            cout << "is_sorted: " << _is_sorted << endl;
            if (!results.empty())
                cout << "size: " << results.size()
                    << " front: " << results.front().key << " back: " << results.back().key << endl;
        }

        {
            TimeInterval search1_begin = Unix_to_now();
            auto [results] = tree.search_from_end();
            TimeInterval search1_end = Unix_to_now();
            cout << "search from end: " << (search1_end - search1_begin).to_ms() << " ms" << endl;
            bool _is_sorted = std::is_sorted(results.begin(), results.end(),
                                             [] (const Result_Impl<int, int>& l, const Result_Impl<int, int>& r) {
                                                 return l.key > r.key;
                                             });
            assert(_is_sorted);
            cout << "is_sorted: " << _is_sorted << endl;
            if (!results.empty())
                cout << "size: " << results.size()
                    << " front: " << results.front().key << " back: " << results.back().key << endl;
        }
    }

    static void BPTree_erase_test(BPTree<BPTree_impl<int, int>>& tree) {
        std::vector<int> arr(total_size);
        for (int i = 0; i < arr.size(); ++i)
            arr[i] = arr.size() - 1 - i;

        std::random_device rd; // 用于生成随机种子
        std::mt19937 g(rd());  // 初始化随机数生成器
        std::reverse(arr.begin(), arr.end());

        std::shuffle(arr.begin(), arr.end(), g);

        {
            TimeInterval erase_begin = Unix_to_now();
            for (auto i : arr) {
                if (!tree.erase(i)) {
                    cout << i << " erase failed" << endl;
                }
            }
            TimeInterval erase_end = Unix_to_now();
            cout << "common erase: " << (erase_end - erase_begin).to_ms() << " ms" << endl;
        }

        {
            TimeInterval erase1_begin = Unix_to_now();
            tree.erase(0, total_size / 2);
            TimeInterval erase1_end = Unix_to_now();
            cout << "range erase: " << (erase1_end - erase1_begin).to_ms() << " ms" << endl;
        }

        {
            TimeInterval search1_begin = Unix_to_now();
            auto [results] = tree.search_from_end();
            TimeInterval search1_end = Unix_to_now();
            cout << "search from end: " << (search1_end - search1_begin).to_ms() << " ms" << endl;
            bool _is_sorted = std::is_sorted(results.begin(), results.end(),
                                             [] (const Result_Impl<int, int>& l, const Result_Impl<int, int>& r) {
                                                 return l.key > r.key;
                                             });
            assert(_is_sorted);
            cout << "is_sorted: " << _is_sorted << endl;
            if (!results.empty())
                cout << "size: " << results.size()
                    << " front: " << results.front().key << " back: " << results.back().key << endl;
        }
    }

    void BPTree_test() {
        BPTree<BPTree_impl<int, int>> tree(Global_ScheduledThread,
                                           GLOBAL_LOG_PATH "/table_int_int",
                                           1 << 20);
        BPTree_insert_test(tree);
        BPTree_erase_test(tree);
    }
}
