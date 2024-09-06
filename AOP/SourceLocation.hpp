//
// Created by taganyer on 24-8-22.
//

#ifndef SOURCELOCATION_HPP
#define SOURCELOCATION_HPP

#ifdef SOURCELOCATION_HPP

namespace Base {

    class SourceLocation {
    public:
        constexpr SourceLocation(const char* file, const char* fun, unsigned line) :
            fileName(file), functionName(fun),
            lineNum(line) {};

        constexpr SourceLocation() = default;

        SourceLocation& operator=(const SourceLocation &other) = default;

        [[nodiscard]] constexpr const char* file() const { return fileName; };

        bool operator==(const SourceLocation &other) const {
            return fileName == other.fileName
                && functionName == other.functionName
                && lineNum == other.lineNum;
        };

        [[nodiscard]] bool is_unknown() const {
            return *this == SourceLocation();
        };

        [[nodiscard]] constexpr const char* function() const { return functionName; };

        [[nodiscard]] constexpr unsigned line() const { return lineNum; };

    private:
        const char* fileName = "unknown";
        const char* functionName = "unknown";
        unsigned lineNum = -1;
    };

}

#if defined(__clang__) || defined(__GNUC__)
#define CURRENT_FUN_LOCATION Base::SourceLocation(__FILE__, __PRETTY_FUNCTION__, __LINE__)
#elif #defind(_MSC_VER)
#define CURRENT_FUN_LOCATION Base::SourceLocation(__FILE__, __FUNSIG__, __LINE__)
#endif

#endif
#endif //SOURCELOCATION_HPP
