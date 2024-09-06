//
// Created by taganyer on 24-2-6.
//

#ifndef BASE_EXCEPTION_HPP
#define BASE_EXCEPTION_HPP

#include <exception>
#include <string>

namespace Base {

    class Exception : public std::exception {
    public:
        Exception(std::string what);

        ~Exception() noexcept override = default;

        [[nodiscard]] const char *what() const noexcept override {
            return message_.c_str();
        }

        [[nodiscard]] const char *stackTrace() const noexcept {
            return stack_.c_str();
        }

    private:
        std::string message_;
        std::string stack_;
    };

}

#endif //BASE_EXCEPTION_HPP
