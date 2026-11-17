#include "atm/auth.hpp"
#include "test_framework.hpp"

using namespace atm;

ATM_TEST(auth_hash_is_deterministic) {
    const std::string first = hash_pin("1234");
    const std::string second = hash_pin("1234");
    CHECK_EQ(first, second);
    CHECK(!first.empty());
    CHECK(first.size() > 8);
}

ATM_TEST(auth_hash_differs_between_pins) {
    CHECK(hash_pin("1234") != hash_pin("1235"));
    CHECK(hash_pin("0000") != hash_pin("9999"));
    CHECK(hash_pin("1234") != hash_pin("12345"));
}

ATM_TEST(auth_verify_pin) {
    const std::string hash = hash_pin("4321");
    CHECK(verify_pin("4321", hash));
    CHECK(!verify_pin("1234", hash));
    CHECK(!verify_pin("", hash));
    CHECK(!verify_pin("4321", ""));
}

ATM_TEST(auth_pin_format) {
    CHECK(is_valid_pin_format("0000"));
    CHECK(is_valid_pin_format("1234"));
    CHECK(is_valid_pin_format("9999"));
    CHECK(!is_valid_pin_format("123"));
    CHECK(!is_valid_pin_format("12345"));
    CHECK(!is_valid_pin_format("12a4"));
    CHECK(!is_valid_pin_format(" 123"));
    CHECK(!is_valid_pin_format(""));
    CHECK(!is_valid_pin_format("12 4"));
}
