//
// Created by taganyer on 24-8-18.
//

#ifndef AOP_HPP
#define AOP_HPP
#ifdef AOP_HPP

#include <exception>
#include <type_traits>
#include <bits/move.h>

#define AOP_WILL_USE_SOURCE_LOCATION /// 该宏解除后不会使用 SourceLocation 相关内容。
#ifdef AOP_WILL_USE_SOURCE_LOCATION

#include "SourceLocation.hpp"

namespace Base {
    /// Aspect 对象可以从这里获得调用函数的信息（对于 before() 不起作用）。
    inline thread_local SourceLocation AOPthreadLoc;
}

/// 用于获得调用函数的函数信息，使用时放到函数内部的开头，只有当函数运行时才会修改 thread_local 对象：AOPthreadLoc
#define AOP_FUN_MARK \
do { \
    if (Base::AOPthreadLoc.is_unknown()) \
        Base::AOPthreadLoc = CURRENT_FUN_LOCATION; \
} while(0);

#else

#define AOP_FUN_MARK

#endif

namespace Base {

//------------------------------------------------------------------------------------------------

    /// 检查 T 是否可以被简单的调用。
    template <typename T, typename...Args>
    class CallableChecker {
    private:
        using Fun = std::remove_reference_t<T>;
        template <typename U>
        static auto test(U* p) -> decltype((std::declval<U>())(std::declval<Args>()...),
            std::true_type());

        template <typename U>
        static std::false_type test(...);

    public:
        static constexpr bool common_callable = decltype(test<Fun>(nullptr))::value;
    };

//------------------------------------------------------------------------------------------------

    /// 检查 O 以 .* 还是 ->* 方式调用 P，Args 为函数参数。
    template <typename P, typename O, typename...Args>
    class MemberFunPtrCallable {
        using MPtr = std::remove_reference_t<P>;
        using OPtr = std::remove_reference_t<O>;

    private:
        template <typename U, typename W>
        static auto test_ptr(U p, W w) -> decltype((w->*p)(std::declval<Args>()...),
            std::true_type());
        template <typename U, typename W>
        static std::false_type test_ptr(...);

        template <typename U, typename W>
        static auto test_object(U p) -> decltype((std::declval<W>().*p)(std::declval<Args>()...),
            std::true_type());
        template <typename U, typename W>
        static std::false_type test_object(...);

    public:
        static constexpr bool ptr_callable = std::is_member_function_pointer_v<MPtr> &&
            std::is_pointer_v<OPtr> && decltype(test_ptr<MPtr, OPtr>(nullptr, nullptr))::value;

        static constexpr bool object_callable = std::is_member_function_pointer_v<MPtr> &&
            !std::is_pointer_v<OPtr> && decltype(test_object<MPtr, OPtr>(nullptr))::value;

        static constexpr bool callable = ptr_callable || object_callable;

    };

//------------------------------------------------------------------------------------------------

    /// 检查 T 是否存在调用 before()、after()、error(std::exception_ptr)(这里不能为 exception_ptr &)、destroy()。
    template <typename T>
    class CallableExitChecker {
    private:
        template <typename U>
        static auto before_test(int) -> decltype(std::declval<U>().before(),
            std::true_type());
        template <typename U>
        static std::false_type before_test(...);

        template <typename U>
        static auto after_test(int) -> decltype(std::declval<U>().after(),
            std::true_type());
        template <typename U>
        static std::false_type after_test(...);

        template <typename U>
        static auto error_test(int) -> decltype(std::declval<U>().error(std::declval<std::exception_ptr>()),
            std::true_type());
        template <typename U>
        static std::false_type error_test(...);

        template <typename U>
        static auto destroy_test(int) -> decltype(std::declval<U>().destroy(),
            std::true_type());
        template <typename U>
        static std::false_type destroy_test(...);

    public:
        static constexpr bool has_before_callable = decltype(before_test<T>(0))::value;

        static constexpr bool has_after_callable = decltype(after_test<T>(0))::value;

        static constexpr bool has_error_callable = decltype(error_test<T>(0))::value;

        static constexpr bool has_destroy_callable = decltype(destroy_test<T>(0))::value;

    };

//------------------------------------------------------------------------------------------------

    /// AOP 的实现类，以继承的方式实现。
    template <std::size_t Index, typename Aspect_, typename...Res>
    class AOP_impl : public AOP_impl<Index + 1, Res...> {
    public:
        using ParentClass = AOP_impl<Index + 1, Res...>;

        using AOP_Type = AOP_impl;

        using AOP_ConstType = const AOP_Type;

        using Aspect = Aspect_;

        using ConstAspect = const Aspect_;

        constexpr AOP_impl(): ParentClass(), _aspect() {};

        constexpr explicit AOP_impl(const Aspect &aspect, const Res &...res):
            ParentClass(res...), _aspect(aspect) {};

        template <typename T, typename...Args,
                  typename = std::enable_if_t<sizeof...(Res) == sizeof...(Args)>>
        constexpr explicit AOP_impl(T &&aspect, Args &&...args) :
            ParentClass(std::forward<Args>(args)...), _aspect(std::forward<T>(aspect)) {};

        template <typename...Args>
        constexpr AOP_impl(const AOP_impl<Index, Args...> &other) :
            ParentClass(static_cast<const typename AOP_impl<Index, Args...>::ParentClass&>(other)),
            _aspect(other.get_aspect()) {};

        template <typename...Args>
        constexpr AOP_impl(AOP_impl<Index, Args...> &&other) :
            ParentClass(static_cast<typename AOP_impl<Index, Args...>::ParentClass&&>(other)),
            _aspect(std::move(other.get_aspect())) {};

        constexpr AOP_impl(const AOP_impl &other) :
            ParentClass(static_cast<const ParentClass&>(other)), _aspect(other.get_aspect()) {};

        constexpr AOP_impl& operator=(const AOP_impl &) = default;

        constexpr AOP_impl(AOP_impl &&) = default;

        constexpr AOP_impl& operator=(AOP_impl &&) = default;

        constexpr Aspect& get_aspect() {
            return _aspect;
        };

        constexpr ConstAspect& get_aspect() const {
            return _aspect;
        };

    protected:
        constexpr void invoke_before() {
            if constexpr (CallableExitChecker<Aspect>::has_before_callable)
                _aspect.before();
            ParentClass::invoke_before();
        };

        constexpr void invoke_before() const {
            if constexpr (CallableExitChecker<const Aspect>::has_before_callable)
                _aspect.before();
            ParentClass::invoke_before();
        };

        constexpr void invoke_after() {
            ParentClass::invoke_after();
            if constexpr (CallableExitChecker<Aspect>::has_after_callable)
                _aspect.after();
        };

        constexpr void invoke_after() const {
            ParentClass::invoke_after();
            if constexpr (CallableExitChecker<const Aspect>::has_after_callable)
                _aspect.after();
        };

        template <typename...Args>
        auto handle_error(Args &&...args) {
            if constexpr (CallableExitChecker<Aspect>::has_error_callable) {
                try {
                    return ParentClass::handle_error(std::forward<Args>(args)...);
                } catch (...) {
                    _aspect.error(std::current_exception());
                    throw;
                }
            } else {
                return ParentClass::handle_error(std::forward<Args>(args)...);
            }
        }

        template <typename...Args>
        auto handle_error(Args &&...args) const {
            if constexpr (CallableExitChecker<const Aspect>::has_error_callable) {
                try {
                    return ParentClass::handle_error(std::forward<Args>(args)...);
                } catch (...) {
                    _aspect.error(std::current_exception());
                    throw;
                }
            } else {
                return ParentClass::handle_error(std::forward<Args>(args)...);
            }
        }

    private:
        Aspect _aspect;

    };

    /// AOP_impl 继承终止。
    template <std::size_t Index, typename Aspect_>
    class AOP_impl<Index, Aspect_> {
    public:
        using AOP_Type = AOP_impl;

        using AOP_ConstType = const AOP_Type;

        using Aspect = Aspect_;

        using ConstAspect = const Aspect_;

        constexpr AOP_impl(): _aspect() {};

        constexpr explicit AOP_impl(const Aspect &aspect) : _aspect(aspect) {};

        template <typename Arg>
        constexpr explicit AOP_impl(Arg &&aspect) : _aspect(std::forward<Arg>(aspect)) {};

        template <typename Arg>
        constexpr AOP_impl(const AOP_impl<Index, Arg> &other) : _aspect(other.get_aspect()) {};

        template <typename Arg>
        constexpr AOP_impl(AOP_impl<Index, Arg> &&other) : _aspect(std::move(other.get_aspect())) {};

        constexpr AOP_impl(const AOP_impl &other) : _aspect(other.get_aspect()) {};

        constexpr AOP_impl& operator=(const AOP_impl &other) {
            _aspect = other.get_aspect();
            return *this;
        };

        constexpr AOP_impl(AOP_impl &&other)
            noexcept(std::is_nothrow_move_constructible_v<Aspect>)
            : _aspect(std::move(other.get_aspect())) {};

        constexpr AOP_impl& operator=(AOP_impl &&other)
            noexcept(std::is_nothrow_move_assignable_v<Aspect>) = default;

        constexpr Aspect& get_aspect() {
            return _aspect;
        };

        constexpr ConstAspect& get_aspect() const {
            return _aspect;
        };

    protected:
        constexpr void invoke_before() {
            if constexpr (CallableExitChecker<Aspect>::has_before_callable)
                _aspect.before();
        };

        constexpr void invoke_before() const {
            if constexpr (CallableExitChecker<const Aspect>::has_before_callable)
                _aspect.before();
        };

        constexpr void invoke_after() {
            if constexpr (CallableExitChecker<Aspect>::has_after_callable)
                _aspect.after();
        };

        constexpr void invoke_after() const {
            if constexpr (CallableExitChecker<const Aspect>::has_after_callable)
                _aspect.after();
        };

        template <typename FunPtr, typename...Args>
        auto handle_error(FunPtr &&ptr, Args &&...args) {
            if constexpr (CallableExitChecker<Aspect>::has_error_callable) {
                if constexpr (CallableChecker<FunPtr, Args...>::common_callable) {
                    try {
                        return ptr(std::forward<Args>(args)...);
                    } catch (...) {
                        _aspect.error(std::current_exception());
                        throw;
                    }
                } else if (MemberFunPtrCallable<FunPtr, Args...>::callable) {
                    try {
                        return member_FunPtr_invoke(std::forward<FunPtr>(ptr), std::forward<Args>(args)...);
                    } catch (...) {
                        _aspect.error(std::current_exception());
                        throw;
                    }
                } else {
                    static_assert(CallableChecker<FunPtr, Args...>::common_callable ||
                                  MemberFunPtrCallable<FunPtr, Args...>::callable,
                                  "unknown type or wrong input");
                }
            } else {
                if constexpr (CallableChecker<FunPtr, Args...>::common_callable) {
                    return ptr(std::forward<Args>(args)...);
                } else if (MemberFunPtrCallable<FunPtr, Args...>::callable) {
                    return member_FunPtr_invoke(std::forward<FunPtr>(ptr), std::forward<Args>(args)...);
                } else {
                    static_assert(CallableChecker<FunPtr, Args...>::common_callable ||
                                  MemberFunPtrCallable<FunPtr, Args...>::callable,
                                  "unknown type or wrong input");
                }
            }
        };

        template <typename FunPtr, typename...Args>
        auto handle_error(FunPtr &&ptr, Args &&...args) const {
            if constexpr (CallableExitChecker<const Aspect>::has_error_callable) {
                if constexpr (CallableChecker<FunPtr, Args...>::common_callable) {
                    try {
                        return ptr(std::forward<Args>(args)...);
                    } catch (...) {
                        _aspect.error(std::current_exception());
                        throw;
                    }
                } else if (MemberFunPtrCallable<FunPtr, Args...>::callable) {
                    try {
                        return member_FunPtr_invoke(std::forward<FunPtr>(ptr), std::forward<Args>(args)...);
                    } catch (...) {
                        _aspect.error(std::current_exception());
                        throw;
                    }
                } else {
                    static_assert(CallableChecker<FunPtr, Args...>::common_callable ||
                                  MemberFunPtrCallable<FunPtr, Args...>::callable,
                                  "unknown type or wrong input");
                }
            } else {
                if constexpr (CallableChecker<FunPtr, Args...>::common_callable) {
                    return ptr(std::forward<Args>(args)...);
                } else if (MemberFunPtrCallable<FunPtr, Args...>::callable) {
                    return member_FunPtr_invoke(std::forward<FunPtr>(ptr), std::forward<Args>(args)...);
                } else {
                    static_assert(CallableChecker<FunPtr, Args...>::common_callable ||
                                  MemberFunPtrCallable<FunPtr, Args...>::callable,
                                  "unknown type or wrong input");
                }
            }
        };

        template <typename FunPtr, typename Invoker, typename...Args>
        auto member_FunPtr_invoke(FunPtr fun_ptr, Invoker invoker, Args &&...args) {
            if constexpr (MemberFunPtrCallable<FunPtr, Invoker, Args...>::ptr_callable) {
                return (invoker->*fun_ptr)(std::forward<Args>(args)...);
            } else {
                return (invoker.*fun_ptr)(std::forward<Args>(args)...);
            }
        };

        template <typename FunPtr, typename Invoker, typename...Args>
        auto member_FunPtr_invoke(FunPtr fun_ptr, Invoker invoker, Args &&...args) const {
            if constexpr (MemberFunPtrCallable<FunPtr, Invoker, Args...>::ptr_callable) {
                return (invoker->*fun_ptr)(std::forward<Args>(args)...);
            } else {
                return (invoker.*fun_ptr)(std::forward<Args>(args)...);
            }
        };

    private:
        Aspect _aspect;

    };

//------------------------------------------------------------------------------------------------

    /// 用于转换 AOP 到指定位置的 AOP_impl。
    template <std::size_t Index, typename Head, typename...Args>
    constexpr typename AOP_impl<Index, Head, Args...>::AOP_Type&
    AOP_cast(AOP_impl<Index, Head, Args...> &aop_impl) {
        return aop_impl;
    };

    template <std::size_t Index, typename Head, typename...Args>
    constexpr typename AOP_impl<Index, Head, Args...>::AOP_ConstType&
    AOP_cast(const AOP_impl<Index, Head, Args...> &aop_impl) {
        return aop_impl;
    };

    template <std::size_t Index, typename Head, typename...Args>
    constexpr typename AOP_impl<Index, Head, Args...>::AOP_Type*
    AOP_cast(AOP_impl<Index, Head, Args...>* aop_impl) {
        return aop_impl;
    };

    template <std::size_t Index, typename Head, typename...Args>
    constexpr typename AOP_impl<Index, Head, Args...>::AOP_ConstType*
    AOP_cast(const AOP_impl<Index, Head, Args...>* aop_impl) {
        return aop_impl;
    };

    template <std::size_t Index, typename Head, typename...Args>
    struct AOP_traits {
        using AOP_Type = typename std::remove_reference_t<
            decltype(AOP_cast<Index>(std::declval<AOP_impl<0, Head, Args...>>()))>::AOP_Type;
        using AOP_ConstType = typename AOP_Type::AOP_ConstType;
        using Aspect = typename AOP_Type::Aspect;
        using ConstAspect = typename AOP_Type::ConstAspect;
    };

//------------------------------------------------------------------------------------------------

    /// 用于推断 AOP 的构造函数是否可以声明为 explict。
    template <bool, typename...Aspects>
    struct AOPConstraints {
        template <typename...Args>
        static constexpr bool is_implicitly_constructible() {
            return std::__and_v<std::is_constructible<Aspects, Args>...,
                                std::is_convertible<Args, Aspects>...>;
        };

        template <typename...Args>
        static constexpr bool is_explicitly_constructible() {
            return std::__and_v<std::is_constructible<Aspects, Args>...,
                                std::__not_<std::__and_<std::is_convertible<Args, Aspects>...>>>;
        };

        static constexpr bool is_implicitly_default_constructible() {
            return std::__and_v<std::__is_implicitly_default_constructible<Aspects>...>;
        };

        static constexpr bool is_explicitly_default_constructible() {
            return std::__and_v<std::is_default_constructible<Aspects>...,
                                std::__not_<std::__and_<
                                    std::__is_implicitly_default_constructible<Aspects>...>>>;
        };
    };

    template <typename...Args>
    struct AOPConstraints<false, Args...> {
        template <typename...>
        static constexpr bool is_implicitly_constructible() {
            return false;
        };

        template <typename...>
        static constexpr bool is_explicitly_constructible() {
            return false;
        };
    };

//------------------------------------------------------------------------------------------------

    /// AOP 模板
    template <typename...Aspects>
    class AOP : public AOP_impl<0, Aspects...> {
        static_assert(sizeof...(Aspects) > 0, "AOP must have at least one argument");

        template <bool Cond>
        using Constraints = AOPConstraints<Cond, Aspects...>;

        template <bool Cond, typename...Args>
        using Implicit = std::enable_if_t<Constraints<Cond>::template
                                          is_implicitly_constructible<Args...>(), bool>;

        template <bool Cond, typename...Args>
        using Explicit = std::enable_if_t<Constraints<Cond>::template
                                          is_explicitly_constructible<Args...>(), int>;

        template <bool O_O>
        using ImplicitDefault = std::enable_if_t<Constraints<O_O>::
                                                 is_implicitly_default_constructible(), bool>;

        template <bool O_O>
        using ExplicitDefault = std::enable_if_t<Constraints<O_O>::
                                                 is_explicitly_default_constructible(), int>;

        template <typename OtherAOP, typename = AOP,
                  typename = std::__remove_cvref_t<OtherAOP>>
        struct UseOther : std::false_type {};

        template <typename OtherAOP, typename Args1, typename Args2>
        struct UseOther<OtherAOP, AOP<Args1>, AOP<Args2>>
            : std::__or_<std::is_convertible<OtherAOP, Args1>,
                         std::is_constructible<Args1, OtherAOP>> {};

        template <typename OtherAOP, typename SameArgs>
        struct UseOther<OtherAOP, AOP<SameArgs>, AOP<SameArgs>>
            : std::true_type {};

        template <typename OtherAOP>
        static constexpr bool other_cannot_convert_directly() {
            return !UseOther<OtherAOP>::value;
        };

        template <typename Arg>
        static constexpr bool valid_args() {
            return sizeof...(Aspects) == 1 && !std::is_same_v<AOP, std::__remove_cvref_t<Arg>>;
        };

        template <typename...Args>
        static constexpr bool valid_args() {
            return sizeof...(Aspects) == sizeof...(Args);
        };

    protected:
        /// 检查默认构造函数是否可以声明为 noexcept。
        static constexpr bool check_default_construct_noexcept() {
            return std::__and_v<std::is_nothrow_default_constructible<Aspects>...>;
        };

        /// 检查构造函数是否可以声明为 noexcept。
        template <typename...Args>
        static constexpr bool check_noexcept() {
            return std::__and_v<std::is_nothrow_constructible<Aspects, Args>...>;
        };

    public:
        using ParentClass = AOP_impl<0, Aspects...>;

        /// 默认构造函数（如果存在的话）。
        template <typename O_o = void, ImplicitDefault<std::is_void_v<O_o>>  = true>
        constexpr AOP()
            noexcept(check_default_construct_noexcept())
            : ParentClass() {};

        template <typename o_O = void, ExplicitDefault<std::is_void_v<o_O>>  = 0>
        constexpr explicit AOP()
            noexcept(check_default_construct_noexcept())
            : ParentClass() {};

        /// 复制构造函数，用来进行模板推断。
        template <typename O_o = void, Implicit<std::is_void_v<O_o>, const Aspects&...>  = true>
        constexpr AOP(const Aspects &...args)
            noexcept(check_noexcept<const Aspects&...>())
            : ParentClass(args...) {};

        template <typename o_O = void, Explicit<std::is_void_v<o_O>, const Aspects&...>  = 0>
        constexpr explicit AOP(const Aspects &...args)
            noexcept(check_noexcept<const Aspects&...>())
            : ParentClass(args...) {};

        /// 构造函数，注意 Args 的数目必须和 Aspects 一致。
        template <typename...Args, Implicit<valid_args<Args...>(), Args...>  = true>
        constexpr AOP(Args &&...args)
            noexcept(check_noexcept<Args...>())
            : ParentClass(std::forward<Args>(args)...) {};

        template <typename...Args, Explicit<valid_args<Args...>(), Args...>  = 0>
        constexpr explicit AOP(Args &&...args)
            noexcept(check_noexcept<Args...>())
            : ParentClass(std::forward<Args>(args)...) {};

        /// 当其他种类的 AOP 可以转化到本类时起作用。
        template <typename...Args, Implicit<sizeof...(Aspects) == sizeof...(Args)
                                            && other_cannot_convert_directly<const AOP<Args...>&>(),
                                            const Args&...>  = true>
        constexpr AOP(const AOP<Args...> &aop)
            noexcept(check_noexcept<const Args&...>())
            : ParentClass(static_cast<const typename AOP<Args...>::ParentClass&>(aop)) {};

        template <typename...Args, Explicit<sizeof...(Aspects) == sizeof...(Args)
                                            && other_cannot_convert_directly<const AOP<Args...>&>(),
                                            const Args&...>  = 0>
        constexpr explicit AOP(const AOP<Args...> &aop)
            noexcept(check_noexcept<const Args&...>())
            : ParentClass(static_cast<const typename AOP<Args...>::ParentClass&>(aop)) {};

        template <typename...Args, Implicit<sizeof...(Aspects) == sizeof...(Args)
                                            && other_cannot_convert_directly<AOP<Args...>&&>(),
                                            Args...>  = true>
        constexpr explicit AOP(AOP<Args...> &&aop)
            noexcept(check_noexcept<Args...>())
            : ParentClass(static_cast<typename AOP<Args...>::ParentClass&&>(aop)) {};

        template <typename...Args, Explicit<sizeof...(Aspects) == sizeof...(Args)
                                            && other_cannot_convert_directly<AOP<Args...>&&>(),
                                            Args...>  = 0>
        constexpr explicit AOP(AOP<Args...> &&aop)
            noexcept(check_noexcept<Args...>())
            : ParentClass(static_cast<typename AOP<Args...>::ParentClass&&>(aop)) {};

        constexpr AOP(const AOP &other) = default;

        constexpr AOP& operator=(const AOP &) = default;

        constexpr AOP(AOP &&aop) = default;

        constexpr AOP& operator=(AOP &&aop) = default;

        /*
         * 可以调用 Class 对象的成员函数、Class 对象的静态函数和其他普通可调用函数。
         * 调用 Class 对象的成员函数时需传入成员函数指针和对应的函数参数。
         * 调用 Class 对象的静态函数时需传入静态函数指针和对应的函数参数。
         * 调用普通可调用对象或函数指针时需传入可调用对象或函数指针及其对应的函数参数。
         */
        template <typename...FunArgs>
        auto invoke(FunArgs &&...args) {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            SourceLocation save = AOPthreadLoc;
            AOPthreadLoc = SourceLocation();
#endif
            ParentClass::invoke_before();
            using ReturnType = decltype(ParentClass::handle_error(std::forward<FunArgs>(args)...));
            if constexpr (std::is_same_v<ReturnType, void>) {
                ParentClass::handle_error(std::forward<FunArgs>(args)...);
                ParentClass::invoke_after();
#ifdef AOP_WILL_USE_SOURCE_LOCATION
                AOPthreadLoc = save;
#endif
            } else {
                ReturnType result = ParentClass::handle_error(std::forward<FunArgs>(args)...);
                ParentClass::invoke_after();
#ifdef AOP_WILL_USE_SOURCE_LOCATION
                AOPthreadLoc = save;
#endif
                return result;
            }
        };

        template <typename...FunArgs>
        auto invoke(FunArgs &&...args) const {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            SourceLocation save = AOPthreadLoc;
#endif
            ParentClass::invoke_before();
            using ReturnType = decltype(ParentClass::handle_error(std::forward<FunArgs>(args)...));
            if constexpr (std::is_same_v<ReturnType, void>) {
                ParentClass::handle_error(std::forward<FunArgs>(args)...);
                ParentClass::invoke_after();
#ifdef AOP_WILL_USE_SOURCE_LOCATION
                AOPthreadLoc = save;
#endif
            } else {
                ReturnType result = ParentClass::handle_error(std::forward<FunArgs>(args)...);
                ParentClass::invoke_after();
#ifdef AOP_WILL_USE_SOURCE_LOCATION
                AOPthreadLoc = save;
#endif
                return result;
            }
        };

        /// 得到指定位置的 aspect 对象引用。
        template <std::size_t Index>
        constexpr auto get_aspect() ->
            typename AOP_traits<Index, Aspects...>::Aspect& {
            static_assert(Index < sizeof...(Aspects), "index out of range");
            return AOP_cast<Index>(this)->get_aspect();
        };

        template <std::size_t Index>
        constexpr auto get_aspect() const ->
            typename AOP_traits<Index, Aspects...>::ConstAspect& {
            static_assert(Index < sizeof...(Aspects), "index out of range");
            return AOP_cast<Index>(this)->get_aspect();
        };

    };

    template <>
    class AOP<> {
    public:
        AOP() = default;
    };

//------------------------------------------------------------------------------------------------

    /// 类对象指针包装器。
    template <typename Class>
    class ObjectWrapper {
    public:
        using ClassPtr = Class *;
        using ConstClassPtr = const Class *;

        template <typename T, typename = std::enable_if_t<std::is_convertible_v<T, Class>>>
        explicit ObjectWrapper(T &cls) noexcept : _ptr(&cls) {};

        ObjectWrapper(const ObjectWrapper &) = default;

        ClassPtr get_class_ptr() { return _ptr; };

        ConstClassPtr get_class_ptr() const { return _ptr; };

        void reset_class_ptr(Class &cls) { _ptr = &cls; };

    private:
        ClassPtr _ptr;

    };

//------------------------------------------------------------------------------------------------

    /// 推断 AOP_Wrapper 的构造函数是否可以声明为 explict。
    template <bool, typename Class, typename...Aspects>
    struct AOP_WrapperConstraints : AOPConstraints<true, Aspects...> {
        using ParentClass = AOPConstraints<true, Aspects...>;

        template <typename C, typename...Args>
        static constexpr bool is_implicitly_constructible() {
            return std::is_convertible_v<C, Class&>
                && ParentClass::template is_implicitly_constructible<Args...>();
        };

        template <typename C, typename...Args>
        static constexpr bool is_explicitly_constructible() {
            return std::is_convertible_v<C, Class&>
                && ParentClass::template is_explicitly_constructible<Args...>();
        };
    };

    template <typename Class, typename...Aspects>
    struct AOP_WrapperConstraints<false, Class, Aspects...> :
        AOPConstraints<false, Aspects...> {};

//------------------------------------------------------------------------------------------------

    /// AOP_Wrapper 不掌握对象生命周期，第一个参数需传入对象的引用（non-const)进行构造。
    template <typename Class, typename...Aspects>
    class AOP_Wrapper : public ObjectWrapper<Class>, public AOP<Aspects...> {
        using Wrapper = ObjectWrapper<Class>;
        using ParentClass = AOP<Aspects...>;

        template <bool Cond>
        using Constraints = AOP_WrapperConstraints<Cond, Class, Aspects...>;

        template <bool Cond, typename C, typename...Args>
        using Implicit = std::enable_if_t<Constraints<Cond>::template
                                          is_implicitly_constructible<C, Args...>(), bool>;

        template <bool Cond, typename C, typename...Args>
        using Explicit = std::enable_if_t<Constraints<Cond>::template
                                          is_explicitly_constructible<C, Args...>(), int>;

        template <typename T, typename = std::__remove_cvref_t<T>>
        struct Is_AOP_Wrapper : std::false_type {};

        template <typename T, typename...Args>
        struct Is_AOP_Wrapper<T, AOP_Wrapper<Args...>> : std::true_type {};

        template <typename C, typename...Args>
        static constexpr bool other_cannot_convert_directly() {
            return sizeof...(Aspects) == sizeof...(Args)
                && !std::__and_v<std::is_same<Class, C>, std::is_same<Aspects, Args>...>;
        };

        template <typename...Args>
        static constexpr bool check_noexcept() {
            return ParentClass::template check_noexcept<Args...>();
        };

    public:
        /// 用于辅助推断模板。
        template <typename O_o = void, Implicit<std::is_void_v<O_o>,
                                                Class&, const Aspects&...>  = true>
        constexpr AOP_Wrapper(Class &object, const Aspects &...aspects)
            noexcept(check_noexcept<const Aspects&...>())
            : Wrapper(object), ParentClass(aspects...) {};

        template <typename o_O = void, Explicit<std::is_void_v<o_O>,
                                                Class&, const Aspects&...>  = 0>
        constexpr explicit AOP_Wrapper(Class &object, const Aspects &...aspects)
            noexcept(check_noexcept<const Aspects&...>())
            : Wrapper(object), ParentClass(aspects...) {};

        /// Args 用于构造 AOP。
        template <typename Object, typename...Args,
                  typename = std::enable_if_t<!Is_AOP_Wrapper<Object>::value>>
        constexpr AOP_Wrapper(Object &object, Args &&...args) :
            Wrapper(object), ParentClass(std::forward<Args>(args)...) {};

        /// 当其他种类的 AOP_Wrapper 可以转换到本类时起作用。
        template <typename C, typename...Args,
                  Implicit<other_cannot_convert_directly<C, Args...>(), C&, const Args&...>  = true>
        constexpr AOP_Wrapper(const AOP_Wrapper<C, Args...> &aop)
            noexcept(check_noexcept<const Args&...>())
            : Wrapper(*const_cast<typename AOP_Wrapper<C, Args...>::ClassPtr>(aop.get_class_ptr())),
            ParentClass(static_cast<const AOP<Args...>&>(aop)) {};

        template <typename C, typename...Args,
                  Explicit<other_cannot_convert_directly<C, Args...>(), C&, const Args&...>  = 0>
        constexpr explicit AOP_Wrapper(const AOP_Wrapper<C, Args...> &aop)
            noexcept(check_noexcept<const Args&...>())
            : Wrapper(*const_cast<typename AOP_Wrapper<C, Args...>::ClassPtr>(aop.get_class_ptr())),
            ParentClass(static_cast<const AOP<Args...>&>(aop)) {};

        template <typename C, typename...Args,
                  Implicit<other_cannot_convert_directly<C, Args...>(), C&, Args...>  = true>
        constexpr AOP_Wrapper(AOP_Wrapper<C, Args...> &&aop)
            noexcept(check_noexcept<Args...>())
            : Wrapper(*aop.get_class_ptr()),
            ParentClass(static_cast<AOP<Args...>&&>(aop)) {};

        template <typename C, typename...Args,
                  Explicit<other_cannot_convert_directly<C, Args...>(), C&, Args...>  = 0>
        constexpr AOP_Wrapper(AOP_Wrapper<C, Args...> &&aop)
            noexcept(check_noexcept<Args...>())
            : Wrapper(*aop.get_class_ptr()),
            ParentClass(static_cast<AOP<Args...>&&>(aop)) {};

        constexpr AOP_Wrapper(const AOP_Wrapper &) = default;

        constexpr AOP_Wrapper& operator=(const AOP_Wrapper &) = default;

        constexpr AOP_Wrapper(AOP_Wrapper &&) = default;

        constexpr AOP_Wrapper& operator=(AOP_Wrapper &&) = default;

        /// 当调用类成员函数时会自动传入本对象的指针，同时也可以运行非成员函数对象。
        template <typename Fun, typename...FunArgs>
        auto invoke(Fun &&fun, FunArgs &&...args) {
            if constexpr (CallableChecker<Fun, FunArgs...>::common_callable) {
                return ParentClass::invoke(std::forward<Fun>(fun), std::forward<FunArgs>(args)...);
            } else {
                return ParentClass::invoke(std::forward<Fun>(fun), Wrapper::get_class_ptr(),
                                           std::forward<FunArgs>(args)...);
            }
        };

        template <typename Fun, typename...FunArgs>
        auto invoke(Fun &&fun, FunArgs &&...args) const {
            if constexpr (CallableChecker<Fun, FunArgs...>::common_callable) {
                return ParentClass::invoke(std::forward<Fun>(fun), std::forward<FunArgs>(args)...);
            } else {
                return ParentClass::invoke(std::forward<Fun>(fun), Wrapper::get_class_ptr(),
                                           std::forward<FunArgs>(args)...);
            }
        };

    };

//------------------------------------------------------------------------------------------------

    /// 用于推断 AOP_Object 的构造函数是否可以声明为 explict。
    template <bool, typename Class, typename...Aspects>
    struct AOP_ObjectConstraints {
        template <typename AOP_, typename...Args>
        static constexpr bool is_implicitly_constructible() {
            return std::__and_v<std::is_constructible<AOP<Aspects...>, AOP_>,
                                std::is_constructible<Class, Args...>,
                                std::is_convertible<AOP_, AOP<Aspects...>>>;
        };

        template <typename AOP_, typename...Args>
        static constexpr bool is_explicitly_constructible() {
            return std::__and_v<std::is_constructible<AOP<Aspects...>, AOP_>,
                                std::is_constructible<Class, Args...>,
                                std::__not_<std::is_convertible<AOP_, AOP<Aspects...>>>>;
        };

        static constexpr bool is_implicitly_default_constructible() {
            return std::__and_v<std::__is_implicitly_default_constructible<Class>,
                                std::__is_implicitly_default_constructible<AOP<Aspects...>>>;
        };

        static constexpr bool is_explicitly_default_constructible() {
            return std::__and_v<std::is_default_constructible<Class>,
                                std::is_default_constructible<AOP<Aspects...>>,
                                std::__not_<std::__and_<
                                    std::__is_implicitly_default_constructible<Class>,
                                    std::__is_implicitly_default_constructible<AOP<Aspects...>>>>>;
        };
    };

    template <typename Class, typename...Aspects>
    struct AOP_ObjectConstraints<false, Class, Aspects...> {
        template <typename...>
        static constexpr bool is_implicitly_constructible() {
            return false;
        };

        template <typename...>
        static constexpr bool is_explicitly_constructible() {
            return false;
        };
    };

    /// AOP_Object 模板，继承了 Class & AOP，直接控制对象生命周期，同时可以在对象销毁时运行 Aspects 中的 destroy() 函数，
    /// 模板第一个参数为 AOP 的左值或右值（这时不能按照 Aspects 来进行构造 AOP）。
    template <typename Class, typename...Aspects>
    class AOP_Object : public AOP<Aspects...>, public Class {
        using ParentClass = AOP<Aspects...>;

        template <bool Cond>
        using Constraints = AOP_ObjectConstraints<Cond, Class, Aspects...>;

        template <bool Cond, typename AOP_, typename...Args>
        using Implicit = std::enable_if_t<Constraints<Cond>::template
                                          is_implicitly_constructible<AOP_, Args...>(), bool>;

        template <bool Cond, typename AOP_, typename...Args>
        using Explicit = std::enable_if_t<Constraints<Cond>::template
                                          is_explicitly_constructible<AOP_, Args...>(), int>;

        template <bool O_O>
        using ImplicitDefault = std::enable_if_t<Constraints<O_O>::
                                                 is_implicitly_default_constructible(), bool>;

        template <bool O_O>
        using ExplicitDefault = std::enable_if_t<Constraints<O_O>::
                                                 is_explicitly_default_constructible(), int>;

        template <typename T, typename = std::__remove_cvref_t<T>>
        struct Is_AOP_Object : std::false_type {};

        template <typename T, typename...Args>
        struct Is_AOP_Object<T, AOP_Object<Args...>> : std::true_type {};

        template <typename C, typename...Args>
        static constexpr bool other_cannot_convert_directly() {
            return sizeof...(Aspects) == sizeof...(Args)
                && !std::__and_v<std::is_same<Class, C>, std::is_same<Aspects, Args>...>;
        };

        static constexpr bool check_default_construct_noexcept() {
            return std::is_nothrow_default_constructible_v<Class>
                && ParentClass::check_default_construct_noexcept();
        };

        template <typename AOP_, typename...Args>
        static constexpr bool check_noexcept() {
            return std::is_nothrow_constructible_v<ParentClass, AOP_>
                && std::is_nothrow_constructible_v<Class, Args...>;
        };

    public:
        /// 如果 AOP 和 Class 都存在默认构造函数，此时可以进行默认构造。
        template <typename O_o = void, ImplicitDefault<std::is_void_v<O_o>>  = true>
        constexpr AOP_Object()
            noexcept(check_default_construct_noexcept())
            : ParentClass(), Class() {};

        template <typename o_O = void, ExplicitDefault<std::is_void_v<o_O>>  = 0>
        constexpr explicit AOP_Object()
            noexcept(check_default_construct_noexcept())
            : ParentClass(), Class() {};

        /// 复制构造函数，用于辅助模板推断。
        template <typename O_o = void,
                  Implicit<std::is_void_v<O_o>, const AOP<Aspects...>, const Class&>  = true>
        constexpr AOP_Object(const AOP<Aspects...> &aop, const Class &object)
            noexcept(check_noexcept<const AOP<Aspects...>&, const Class&>())
            : ParentClass(aop), Class(object) {};

        template <typename O_o = void,
                  Explicit<std::is_void_v<O_o>, const AOP<Aspects...>, const Class&>  = 0>
        constexpr explicit AOP_Object(const AOP<Aspects...> &aop, const Class &object)
            noexcept(check_noexcept<const AOP<Aspects...>&, const Class&>())
            : ParentClass(aop), Class(object) {};

        /// 第一个用于构造 AOP，剩下的 Args 用于构造 Class。
        template <typename AOP_, typename...Args,
                  Implicit<!Is_AOP_Object<AOP_>::value, AOP_, Args...>  = true>
        constexpr AOP_Object(AOP_ &&aop, Args &&...args)
            noexcept(check_noexcept<AOP_, Args...>())
            : ParentClass(std::forward<AOP_>(aop)),
            Class(std::forward<Args>(args)...) {};

        template <typename AOP_, typename...Args,
                  Explicit<!Is_AOP_Object<AOP_>::value, AOP_, Args...>  = 0>
        constexpr explicit AOP_Object(AOP_ &&aop, Args &&...args)
            noexcept(check_noexcept<AOP_, Args...>())
            : ParentClass(std::forward<AOP_>(aop)),
            Class(std::forward<Args>(args)...) {};

        /// 当其他种类的 AOP_Object 可以转化为本类时起作用。
        template <typename Object, typename...Args,
                  Implicit<other_cannot_convert_directly<Object, Args...>(),
                           const AOP<Args...>&, const Object&> = true>
        constexpr AOP_Object(const AOP_Object<Object, Args...> &object)
            noexcept(check_noexcept<const AOP<Args...>&, const Object&>())
            : ParentClass(static_cast<const AOP<Args...>&>(object)),
            Class(static_cast<const Object&>(object)) {};

        template <typename Object, typename...Args,
                  Explicit<other_cannot_convert_directly<Object, Args...>(),
                           const AOP<Args...>&, const Object&> = 0>
        constexpr explicit AOP_Object(const AOP_Object<Object, Args...> &object)
            noexcept(check_noexcept<const AOP<Args...>&, const Object&>())
            : ParentClass(static_cast<const AOP<Args...>&>(object)),
            Class(static_cast<const Object&>(object)) {};

        template <typename Object, typename...Args,
                  Implicit<other_cannot_convert_directly<Object, Args...>(),
                           AOP<Args...>&&, Object&&> = true>
        constexpr AOP_Object(AOP_Object<Object, Args...> &&object)
            noexcept(check_noexcept<AOP<Args...>&&, Object&&>())
            : ParentClass(static_cast<AOP<Args...>&&>(object)),
            Class(static_cast<Object&&>(object)) {};

        template <typename Object, typename...Args,
                  Explicit<other_cannot_convert_directly<Object, Args...>(),
                           AOP<Args...>&&, Object&&> = 0>
        constexpr explicit AOP_Object(AOP_Object<Object, Args...> &&object)
            noexcept(check_noexcept<AOP<Args...>&&, Object&&>())
            : ParentClass(static_cast<AOP<Args...>&&>(object)),
            Class(static_cast<Object&&>(object)) {};

        constexpr AOP_Object(const AOP_Object &) = default;

        constexpr AOP_Object& operator=(const AOP_Object &) = default;

        constexpr AOP_Object(AOP_Object &&) = default;

        constexpr AOP_Object& operator=(AOP_Object &&) = default;

        ~AOP_Object() { invoke_destroy<0>(); };

        /// 当调用类成员函数时会自动传入本对象的指针，同时也可以运行非成员函数对象。
        template <typename Fun, typename...FunArgs>
        auto invoke(Fun &&fun, FunArgs &&...args) {
            if constexpr (CallableChecker<Fun, FunArgs...>::common_callable) {
                return ParentClass::invoke(std::forward<Fun>(fun), std::forward<FunArgs>(args)...);
            } else {
                return ParentClass::invoke(std::forward<Fun>(fun), this,
                                           std::forward<FunArgs>(args)...);
            }
        };

        template <typename Fun, typename...FunArgs>
        auto invoke(Fun &&fun, FunArgs &&...args) const {
            if constexpr (CallableChecker<Fun, FunArgs...>::common_callable) {
                return ParentClass::invoke(std::forward<Fun>(fun), std::forward<FunArgs>(args)...);
            } else {
                return ParentClass::invoke(std::forward<Fun>(fun), this,
                                           std::forward<FunArgs>(args)...);
            }
        };

    private:
        /// 对象销毁时调用。
        template <std::size_t Index>
        constexpr void invoke_destroy() {
            using Aspect = typename AOP_traits<Index, Aspects...>::Aspect;
            using ConstAspect = typename AOP_traits<Index, Aspects...>::ConstAspect;
            if constexpr (Index + 1 < sizeof...(Aspects)) {
                this->invoke_destroy<Index + 1>();
            }
            if constexpr (CallableExitChecker<Aspect>::has_destroy_callable
                || CallableExitChecker<ConstAspect>::has_destroy_callable) {
                ParentClass::template get_aspect<Index>().destroy();
            }
        };
    };

//------------------------------------------------------------------------------------------------

/// 仅能运行类的成员函数，和类的静态函数。
#define AOP_MemberFun_Agent_(fun_name_) \
    [] (auto &object__, auto &&...args__) \
    { return (object__).fun_name_(std::forward<decltype(args__)>(args__)...); }

/// 运行时AOP_Wrapper 的成员函数宏，object_ 为 AOP_Wrapper 的引用，fun_name_为调用的成员函数名，其余可视情况传入函数参数。
#define AOP_Wrapper_Agent(object_, fun_name_, ...) \
    object_.invoke(AOP_MemberFun_Agent_(fun_name_), *((object_).get_class_ptr()), ##__VA_ARGS__ )

/// 运行时AOP_Object 的成员函数宏，object_ 为 AOP_Object 的引用，fun_name_为调用的成员函数名，其余可视情况传入函数参数。
#define AOP_Object_Agent(object_, fun_name_, ...) \
    object_.invoke(AOP_MemberFun_Agent_(fun_name_), object_, ##__VA_ARGS__ )

//------------------------------------------------------------------------------------------------

}

#endif

#endif //AOP_HPP
