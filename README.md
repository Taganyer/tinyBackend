# tinyBackend

C++17 实现，功能丰富的后端组件库。

* Base 部分：对底层原生API进行包装，实现了更为丰富易用的基础工具和通用模板。
    1. 实现了如ThreadPool、ScheduledThread等多种多线程组件。
    2. 提供了多个内存管理工具，如BufferPool、RingBuffer、LRUCache等。
    3. 实现了带有缓存、自动刷盘并支持各种键值类型的B+Tree模板。


* AOP 部分：

  一个易用的静态AOP组件，可以无侵入的增强函数或类的功能。


* Net 部分：

  除其他常用的网络组件外，以事件驱动为架构，实现了NetLink、Controller、Reactor等组件用于TCP网络通信。


* Distributed 部分：

  对Raft分布式共识算法进行封装，开放了同步接口，只需简单注册函数即可自动同步日志。


* LogSystem 部分：

    1. 系统日志：带有双缓冲区并能够自动进行异步刷新的本地日志系统。
    2. 分布式全链路日志追踪系统：

       以一个分布式全局唯一的ID为一条链路的标志，加上若干埋入的逻辑节点的ID定义一条链路。

       链路节点提供串行、并行、条件分支三种执行关系来进行逻辑表示，同时支持节点日志写入。

       链路上游节点单项依赖下游节点，支持灵活动态调整，降低了程序的耦合度。

       系统实现了链路级的逻辑追踪、逻辑检查、节点超时预警。

       系统中心实时搜集服务集群上报信息，并将其汇总为信息流，主动调用注册的处理函数，同时储存信息。

       系统提供提供链路级的历史日志查询功能和文件级的历史过程重放功能。

## 安装
可指定 CMAKE_INSTALL_PREFIX 改变安装路径（默认 /usr/local/）

可指定 LOG_PATH 改变日志路径（默认 ${PROJECT_ROOT_DIR}/global_logs）

可定义 -DADD_AOP=OFF -DADD_LOGSYSTEM=OFF -DADD_DISTRIBUTED=OFF 关闭这三个部分的编译（默认编译打开）
````shell
mkdir build
cd build
cmake ..
# 可选项
# cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install -DADD_AOP=OFF -DADD_LOGSYSTEM=OFF -DADD_DISTRIBUTED=OFF
sudo make install
# sudo make uninstall
````
## 使用（以CMake为例）
可以编辑 cmake_config/CMake_config.h.in 文件改变 GlobalObjects 的配置。
````CMake
find_package(tinyBackend REQUIRED)

# 其他代码...

# 选取需要的
target_link_libraries(${PROJECT_NAME}
        PRIVATE tinyBackend::Base
        PRIVATE tinyBackend::Net
        PRIVATE tinyBackend::Distributed
        PRIVATE tinyBackend::LogSystem)
````