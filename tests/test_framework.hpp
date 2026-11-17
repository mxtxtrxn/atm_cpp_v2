#ifndef ATM_TEST_FRAMEWORK_HPP
#define ATM_TEST_FRAMEWORK_HPP

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "atm/account.hpp"
#include "atm/bank.hpp"
#include "atm/money.hpp"
#include "atm/transaction.hpp"

namespace test {

struct Case {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failure_count() {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(std::string name, std::function<void()> fn) {
        registry().push_back(Case{std::move(name), std::move(fn)});
    }
};

class FatalFailure : public std::exception {
public:
    explicit FatalFailure(std::string message) : message_(std::move(message)) {}
    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

inline std::string stringify(const std::string& value) {
    return value;
}

inline std::string stringify(const char* value) {
    return value == nullptr ? "null" : value;
}

inline std::string stringify(bool value) {
    return value ? "true" : "false";
}

inline std::string stringify(const atm::Money& value) {
    return value.to_string();
}

inline std::string stringify(atm::AccountStatus value) {
    return atm::to_string(value);
}

inline std::string stringify(atm::TransactionType value) {
    return atm::to_string(value);
}

inline std::string stringify(atm::OperationResult value) {
    return atm::to_string(value);
}

template <typename T>
std::string stringify(const T& value) {
    if constexpr (std::is_arithmetic_v<T>) {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }
    return "<value>";
}

inline void report(const char* expression, const char* file, int line,
                   const std::string& detail, bool fatal) {
    ++failure_count();
    std::cerr << "    " << (fatal ? "FATAL" : "FAIL") << " " << file << ":" << line
              << ": " << expression;
    if (!detail.empty()) {
        std::cerr << " -> " << detail;
    }
    std::cerr << '\n';
    if (fatal) {
        throw FatalFailure(expression);
    }
}

inline int run_all() {
    int failed_cases = 0;
    for (const auto& test_case : registry()) {
        int before = failure_count();
        try {
            test_case.fn();
        } catch (const FatalFailure&) {
            // Already reported with location information.
        } catch (const std::exception& error) {
            ++failure_count();
            std::cerr << "    FAIL exception: " << error.what() << '\n';
        } catch (...) {
            ++failure_count();
            std::cerr << "    FAIL unknown exception\n";
        }
        bool passed = failure_count() == before;
        std::cout << (passed ? "[PASS] " : "[FAIL] ") << test_case.name << '\n';
        if (!passed) {
            ++failed_cases;
        }
    }

    const std::size_t total = registry().size();
    const std::size_t passed = total - static_cast<std::size_t>(failed_cases);
    std::cout << '\n' << passed << '/' << total << " casos aprobados\n";
    return failed_cases == 0 ? 0 : 1;
}

}  // namespace test

#define ATM_TEST(name)                                       \
    static void name();                                      \
    static ::test::Registrar registrar_##name(#name, name);  \
    static void name()

#define CHECK(condition)                                                           \
    do {                                                                           \
        if (!(condition)) {                                                        \
            ::test::report(#condition, __FILE__, __LINE__, "", false);             \
        }                                                                          \
    } while (0)

#define REQUIRE(condition)                                                         \
    do {                                                                           \
        if (!(condition)) {                                                        \
            ::test::report(#condition, __FILE__, __LINE__, "", true);              \
        }                                                                          \
    } while (0)

#define CHECK_EQ(actual, expected)                                                 \
    do {                                                                           \
        auto&& actual_value_ = (actual);                                           \
        auto&& expected_value_ = (expected);                                       \
        if (!(actual_value_ == expected_value_)) {                                 \
            ::test::report(#actual " == " #expected, __FILE__, __LINE__,           \
                           ::test::stringify(actual_value_) + " vs " +             \
                               ::test::stringify(expected_value_),                 \
                           false);                                                 \
        }                                                                          \
    } while (0)

#define CHECK_THROWS(expression)                                                   \
    do {                                                                           \
        bool threw_ = false;                                                       \
        try {                                                                      \
            (void)(expression);                                                    \
        } catch (...) {                                                            \
            threw_ = true;                                                         \
        }                                                                          \
        if (!threw_) {                                                             \
            ::test::report(#expression " throws", __FILE__, __LINE__, "", false);  \
        }                                                                          \
    } while (0)

#endif
