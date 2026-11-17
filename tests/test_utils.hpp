#ifndef ATM_TEST_UTILS_HPP
#define ATM_TEST_UTILS_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

#include "atm/account.hpp"
#include "atm/auth.hpp"

namespace test {

inline std::filesystem::path fresh_temp_dir(const std::string& name) {
    const std::filesystem::path base =
        std::filesystem::temp_directory_path() / "atm_cpp_v2_tests" / name;
    std::error_code error;
    std::filesystem::remove_all(base, error);
    std::filesystem::create_directories(base, error);
    return base;
}

inline void remove_dir(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::remove_all(path, error);
}

inline atm::Account make_account(const std::string& number, const std::string& pin,
                                 std::int64_t cents,
                                 atm::AccountStatus status = atm::AccountStatus::Active) {
    atm::Account account(number, "Usuario Demo " + number, atm::hash_pin(pin),
                         atm::Money::from_cents(cents), status);
    account.set_phone_number("0000000000");
    return account;
}

}  // namespace test

#endif
