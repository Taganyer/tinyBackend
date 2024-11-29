//
// Created by taganyer on 24-8-19.
//

#include "../AOP_test.hpp"
#include "AOP/AOP.hpp"
#include "AOP/SourceLocation.hpp"

#include <cassert>
#include <iostream>

using namespace std;
using namespace Base;

static void AOP_Wrapper_test() {
    class A {
    public:
        A(int a) : a(a) {};

        static int test() {
            AOP_FUN_MARK
            cout << CURRENT_FUN_LOCATION.function() << ' ' <<
                CURRENT_FUN_LOCATION.line() << endl;
            return 0;
        }

        void foo(int &b) {
            AOP_FUN_MARK
            cout << CURRENT_FUN_LOCATION.function() << ' ' <<
                CURRENT_FUN_LOCATION.line() << endl;
        };

        void foo(int &b) const {
            AOP_FUN_MARK
            cout << CURRENT_FUN_LOCATION.function() << ' ' <<
                CURRENT_FUN_LOCATION.line() << endl;
        };

        void foo(int &b, int) {\
            AOP_FUN_MARK
            cout << CURRENT_FUN_LOCATION.function() << ' ' <<
                CURRENT_FUN_LOCATION.line() << endl;
        }

    private:
        int a = 0;

    };

    struct A1 {
        void before() {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            cout << "A1 before the: " << AOPthreadLoc.function() << endl;
#endif
        };

        void after() {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            cout << "A1 after the: " << AOPthreadLoc.function() << endl;
#endif
        };

        void before() const {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            cout << "A1 const before the: " << AOPthreadLoc.function() << endl;
#endif
        };

        void after() const {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            cout << "A1 const after the: " << AOPthreadLoc.function() << endl;
#endif
        };

        /// std::exception_ptr & 不起作用。
        bool error(const std::exception_ptr &) {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            cerr << "A1 error in the: " << AOPthreadLoc.function() << endl;
#endif
            return true;
        };
    };

    struct A2 {
        void before() {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            cout << "A2 before the: " << AOPthreadLoc.function() << endl;
#endif
        };

        void after() {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            cout << "A2 after the: " << AOPthreadLoc.function() << endl;
#endif
        };

        void before() const {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            cout << "A2 const before the: " << AOPthreadLoc.function() << endl;
#endif
        };

        void after() const {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            cout << "A2 const after the: " << AOPthreadLoc.function() << endl;
#endif
        };

        bool error(const std::exception_ptr &) const {
#ifdef AOP_WILL_USE_SOURCE_LOCATION
            cerr << "A2 const error in the: " << AOPthreadLoc.function() << endl;
#endif
            return false;
        };
    };

    struct A3 {
        void operator()() {
            AOP_FUN_MARK
            cout << CURRENT_FUN_LOCATION.function() << ' ' <<
                CURRENT_FUN_LOCATION.line() << endl;
        };
    };

    auto fun = [] {
        AOP_FUN_MARK
        cout << CURRENT_FUN_LOCATION.function() << ' ' <<
            CURRENT_FUN_LOCATION.line() << endl;
    };

    auto raise_error = [] {
        AOP_FUN_MARK
        throw runtime_error("A3 raise error");
    };

    /// WARN: C++17 无法自动推断同名函数的成员函数指针，需手动指定。
    void (A::*foo_non_const)(int &) = &A::foo;
    void (A::*foo_2)(int &, int) = &A::foo;
    void (A::*foo_const)(int &) const = &A::foo;

    A a(1);

    AOP aop { A1(), A2() };
    AOP_Wrapper aop_w1(a, aop);
    static_assert(is_same_v<decltype(aop_w1), AOP_Wrapper<A, AOP<A1, A2>>>);
    AOP_Wrapper aop_w2(a, std::move(aop));
    static_assert(is_same_v<decltype(aop_w2), AOP_Wrapper<A, AOP<A1, A2>>>);
    AOP_Wrapper aop_w3(a, A1());
    static_assert(is_same_v<decltype(aop_w3), AOP_Wrapper<A, A1>>);

    AOP_Wrapper<A, A1, const A2> AOP_A { a, A1(), A2() };
    cout << "use AOP:" << endl;
    int temp = AOP_A.invoke(&A::test);
    cout << "fun0 end\n" << endl;
    AOP_A.invoke(foo_non_const, temp);
    AOP_Wrapper_Agent(AOP_A, foo, temp);
    cout << "fun1 end\n" << endl;
    AOP_A.invoke(foo_2, temp, 0);
    AOP_Wrapper_Agent(AOP_A, foo, temp, 0);
    cout << "fun2 end\n" << endl;
    AOP_A.invoke(fun);
    cout << "fun3 end\n" << endl;
    AOP_A.invoke(A3());
    cout << "fun4 end\n" << endl;

    const AOP_Wrapper const_AOP_A { a, A1(), A2() };
    cout << "\nconst use AOP:" << endl;

    temp = const_AOP_A.invoke(&A::test);
    AOP_Wrapper_Agent(const_AOP_A, test);
    cout << "fun5 end\n" << endl;

    const_AOP_A.invoke(foo_const, temp);
    AOP_Wrapper_Agent(const_AOP_A, foo, temp);
    cout << "fun6 end\n" << endl;

    const_AOP_A.invoke(fun);
    cout << "fun7 end\n" << endl;

    const_AOP_A.invoke(A3());
    cout << "fun8 end\n" << endl;

    // get_aspect:
    static_assert(is_same_v<AOP_traits<0, A1, const A2>::Aspect&,
                            decltype(AOP_A.get_aspect<0>())>);
    static_assert(is_same_v<AOP_traits<1, A1, const A2>::Aspect&,
                            decltype(AOP_A.get_aspect<1>())>);
    static_assert(is_same_v<AOP_traits<1, A1, A2>::ConstAspect&,
                            decltype(const_AOP_A.get_aspect<1>())>);
    cout << endl;

    // ptr test
    assert(&a == AOP_A.get_class_ptr());
    assert(&a == const_AOP_A.get_class_ptr());
    class B : public A {
    public:
        B(): A(1) {};
    };
    B b;
    AOP_A.reset_class_ptr(b);
    assert(&b == AOP_A.get_class_ptr());

    // error test
    try {
        AOP_A.invoke(raise_error);
    } catch (...) {
        cerr << "error test" << endl;
    }

}

static void AOP_Object_test() {
    cout << "AOP_Object test:" << endl;
    class No_copy {
    public:
        No_copy() = default;

        No_copy(const No_copy &) = delete;
        No_copy&operator=(const No_copy &) = delete;

        No_copy(No_copy &&) = default;
        No_copy&operator=(No_copy &&) = default;

        ~No_copy() = default;

    };

    class A {
    public:
        A() { cout << "A" << endl; };

        A(const A &) { cout << "A::A(const A &)" << endl; };

        A&operator=(const A &) {
            cout << "A::operator=(const A &)" << endl;
            return *this;
        };

        A(A &&) noexcept { cout << "A::A(A &&)" << endl; };

        A& operator=(A &&) noexcept {
            cout << "A::operator=(A &&)" << endl;
            return *this;
        };

        ~A() { cout << "~A::A()" << endl; };

    private:
        No_copy c;
    };

    struct A1 {
        A1() { cout << "A1" << endl; };

        A1(A1 &) { cout << "A1::A1(A1 &)" << endl; };

        A1(const A1 &) { cout << "A1::A1(const A1 &)" << endl; };

        A1&operator=(const A1 &) {
            cout << "A1::operator=(const A1 &)" << endl;
            return *this;
        };

        A1(A1 &&) noexcept { cout << "A1::A1(A1 &&)" << endl; };

        A1& operator=(A1 &&) noexcept {
            cout << "A1::operator=(A1 &&)" << endl;
            return *this;
        };

        ~A1() { cout << "~A1::A1()" << endl; };

        void destroy() noexcept {
            cout << "A1 destroy before the object." << endl;
        }

        void destroy() const noexcept {
            cout << "A1 destroy const before the object." << endl;
        }
    };

    struct A2 {
        A2() { cout << "A2" << endl; };

        A2(A2 &) { cout << "A2::A2(A2 &)" << endl; };

        A2(const A2 &) { cout << "A2::A2(const A2 &)" << endl; };

        A2&operator=(const A2 &) {
            cout << "A2::operator=(const A2 &)" << endl;
            return *this;
        };

        A2(A2 &&) noexcept { cout << "A2::A2(A2 &&)" << endl; };

        A2& operator=(A2 &&) noexcept {
            cout << "A2::operator=(A2 &&)" << endl;
            return *this;
        };

        ~A2() { cout << "~A2::A2()" << endl; };

        void destroy() noexcept {
            cout << "A2 destroy before the object." << endl;
        }

        void destroy() const noexcept {
            cout << "A2 destroy const before the object." << endl;
        }
    };

    struct A3 : public A2 {
        // A3() { cout << "A3" << endl; };
        A3(const A3 &) { cout << "A3::A3(const A3 &)" << endl; };
        A3(int, int) { cout << "A3::A3(int, int)" << endl; };
    };

    AOP aop1 { A1(), A2() };
    A2 a2;
    AOP aop2 { A1(), a2 };
    AOP aop3 { A1(), aop1 };
    static_assert(is_same_v<decltype(aop3), AOP<A1, AOP<A1, A2>>>);
    AOP aop4 { aop2 }, aop5 { std::move(aop2) };
    static_assert(is_same_v<decltype(aop2), decltype(aop2)>);
    cout << "aop end" << endl;
    AOP_Wrapper t { a2, std::move(aop1) };
    AOP_Wrapper t1 { a2, aop2 };
    cout << "wrapper end" << endl;
    AOP_Object<A1, A1, A2> object { std::move(aop2) };
    const AOP_Object o { aop2, A3(1, 1) };
    cout << "aop_object end" << endl;
    AOP_Object_Agent(o, destroy);
}

void static marco_test() {
    cout << "marco test:" << endl;
}

void Test::AOP_test() {
    // AOP_Wrapper_test();
    AOP_Object_test();
};
