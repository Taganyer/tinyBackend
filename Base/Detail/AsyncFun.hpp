//
// Created by taganyer on 24-5-10.
//

#ifndef BASE_ASYNCFUN_HPP
#define BASE_ASYNCFUN_HPP

#include <future>
#include "../Exception.hpp"

namespace Base::Detail {

    template <typename Target_fun, typename... Args>
    class FunPack {
    public:
        explicit FunPack(Target_fun&& fun, Args&&... args) :
            fun(std::forward<Target_fun>(fun)), args(std::tuple<Args...>(std::forward<Args>(args)...)) {};

        virtual ~FunPack() = default;

        virtual void operator()() {
            std::apply(fun, std::move(args));
        };

    protected:
        Target_fun fun;
        std::tuple<Args...> args;

    };

    template <typename Result, typename Target_fun, typename... Args>
    class AsyncFun : public FunPack<Target_fun, Args...> {
    public:
        using Parent = FunPack<Target_fun, Args...>;

        explicit AsyncFun(Target_fun&& fun, Args&&... args) :
            Parent(std::forward<Target_fun>(fun), std::forward<Args>(args)...) {};

        ~AsyncFun() override = default;

        auto get_future() { return _promise.get_future(); };

        void kill_task() {
            _promise.set_exception(std::make_exception_ptr(Exception("This Task have been interrupted")));
        };

        void operator()() override {
            try {
                if constexpr (std::is_same_v<Result, void>) {
                    std::apply(Parent::fun, std::move(Parent::args));
                    _promise.set_value();
                } else {
                    _promise.set_value(std::apply(Parent::fun, std::move(Parent::args)));
                }
            } catch (...) {
                _promise.set_exception(std::current_exception());
            }
        };

    private:
        std::promise<Result> _promise;

    };

}

#endif //BASE_ASYNCFUN_HPP
