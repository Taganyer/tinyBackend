//
// Created by taganyer on 24-5-10.
//

#ifndef BASE_ASYNCFUN_HPP
#define BASE_ASYNCFUN_HPP

#include <future>
#include "Base/Exception.hpp"

namespace Base::Detail {

    template<typename Result, typename Target_fun, typename ...Args>
    class AsyncFun {
    private:
        std::promise<Result> _promise;
        Target_fun fun;
        std::tuple<Args...> args;

    public:
        AsyncFun(Target_fun &&fun, Args &&...args) :
                fun(fun), args(std::tuple<Args...>(std::forward<Args>(args)...)) {};

        auto get_future() { return _promise.get_future(); };

        void kill_task() {
            _promise.set_exception(std::make_exception_ptr(Exception("This Task have been interrupted")));
        };

        void operator()() {
            try {
                if constexpr (std::is_same<Result, void>::value) {
                    std::apply(fun, std::move(args));
                    _promise.set_value();
                } else {
                    _promise.set_value(std::apply(fun, std::move(args)));
                }
            } catch (...) {
                _promise.set_exception(std::current_exception());
            }
        }
    };

}

#endif //BASE_ASYNCFUN_HPP
