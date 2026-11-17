#ifndef ATM_MONEY_HPP
#define ATM_MONEY_HPP

#include <cstdint>
#include <string>

namespace atm {

// Money is stored as integer cents to avoid floating point rounding errors.
class Money {
public:
    Money();
    explicit Money(std::int64_t cents);

    static Money from_cents(std::int64_t cents);
    static Money from_units(double units);
    static Money parse(const std::string& text);

    std::int64_t cents() const;
    double units() const;

    Money operator+(const Money& other) const;
    Money operator-(const Money& other) const;
    Money& operator+=(const Money& other);
    Money& operator-=(const Money& other);

    bool operator==(const Money& other) const;
    bool operator!=(const Money& other) const;
    bool operator<(const Money& other) const;
    bool operator<=(const Money& other) const;
    bool operator>(const Money& other) const;
    bool operator>=(const Money& other) const;

    bool is_positive() const;
    bool is_zero() const;

    std::string to_string() const;

private:
    std::int64_t cents_;
};

}  // namespace atm

#endif
