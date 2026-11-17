#include <limits>
#include <string>

#include "test_framework.hpp"

using atm::Money;

ATM_TEST(money_parse_integer_units) {
    CHECK_EQ(Money::parse("10"), Money::from_cents(1000));
    CHECK_EQ(Money::parse("0"), Money::from_cents(0));
    CHECK_EQ(Money::parse("007"), Money::from_cents(700));
}

ATM_TEST(money_parse_decimals) {
    CHECK_EQ(Money::parse("10.5"), Money::from_cents(1050));
    CHECK_EQ(Money::parse("10.50"), Money::from_cents(1050));
    CHECK_EQ(Money::parse("0.09"), Money::from_cents(9));
    CHECK_EQ(Money::parse(".75"), Money::from_cents(75));
    CHECK_EQ(Money::parse("10.999"), Money::from_cents(1099));
}

ATM_TEST(money_parse_symbols_and_separators) {
    CHECK_EQ(Money::parse("$1,234.56"), Money::from_cents(123456));
    CHECK_EQ(Money::parse(" 42.00 "), Money::from_cents(4200));
    CHECK_EQ(Money::parse("+3.25"), Money::from_cents(325));
    CHECK_EQ(Money::parse("-3.25"), Money::from_cents(-325));
}

ATM_TEST(money_parse_rejects_invalid_text) {
    CHECK_THROWS(Money::parse("abc"));
    CHECK_THROWS(Money::parse("1.2.3"));
    CHECK_THROWS(Money::parse("12a"));
    CHECK_THROWS(Money::parse("1e5"));
}

ATM_TEST(money_from_units_rounds_to_cents) {
    CHECK_EQ(Money::from_units(12.34), Money::from_cents(1234));
    CHECK_EQ(Money::from_units(12.345), Money::from_cents(1235));
    CHECK_EQ(Money::from_units(0.1 + 0.2), Money::from_cents(30));
    CHECK_EQ(Money::from_units(-1.006), Money::from_cents(-101));
}

ATM_TEST(money_from_units_rejects_non_finite_and_oversized_values) {
    CHECK_THROWS(Money::from_units(std::numeric_limits<double>::infinity()));
    CHECK_THROWS(Money::from_units(-std::numeric_limits<double>::infinity()));
    CHECK_THROWS(Money::from_units(std::numeric_limits<double>::quiet_NaN()));
    CHECK_THROWS(Money::from_units(1.0e17));
    CHECK_THROWS(Money::from_units(-1.0e17));
}

ATM_TEST(money_to_string_formats_two_decimals) {
    CHECK_EQ(Money::from_cents(0).to_string(), std::string("0.00"));
    CHECK_EQ(Money::from_cents(5).to_string(), std::string("0.05"));
    CHECK_EQ(Money::from_cents(-5).to_string(), std::string("-0.05"));
    CHECK_EQ(Money::from_cents(100).to_string(), std::string("1.00"));
    CHECK_EQ(Money::from_cents(123456).to_string(), std::string("1234.56"));
    CHECK_EQ(Money::from_cents(-123456).to_string(), std::string("-1234.56"));
}

ATM_TEST(money_arithmetic_is_exact) {
    Money total = Money::from_cents(1000);
    total += Money::from_cents(255);
    CHECK_EQ(total, Money::from_cents(1255));
    total -= Money::from_cents(55);
    CHECK_EQ(total, Money::from_cents(1200));

    Money difference = Money::from_cents(500) - Money::from_cents(800);
    CHECK_EQ(difference, Money::from_cents(-300));
    CHECK_EQ(Money::from_cents(100) + Money::from_cents(200), Money::from_cents(300));
}

ATM_TEST(money_comparisons_and_helpers) {
    CHECK(Money::from_cents(100) < Money::from_cents(200));
    CHECK(Money::from_cents(200) <= Money::from_cents(200));
    CHECK(Money::from_cents(300) > Money::from_cents(200));
    CHECK(Money::from_cents(300) >= Money::from_cents(300));
    CHECK(Money::from_cents(300) != Money::from_cents(301));
    CHECK(Money::from_cents(1).is_positive());
    CHECK(!Money::from_cents(0).is_positive());
    CHECK(Money::from_cents(0).is_zero());
    CHECK(!Money::from_cents(-1).is_zero());
}

ATM_TEST(money_units_round_trip) {
    CHECK_EQ(Money::from_units(150.75).units(), 150.75);
    CHECK_EQ(Money::from_cents(999).cents(), static_cast<std::int64_t>(999));
    CHECK_EQ(Money().cents(), static_cast<std::int64_t>(0));
}
