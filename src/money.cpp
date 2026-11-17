#include "atm/money.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace atm {

Money::Money() : cents_(0) {}

Money::Money(std::int64_t cents) : cents_(cents) {}

Money Money::from_cents(std::int64_t cents) {
    return Money(cents);
}

Money Money::from_units(double units) {
    const double cents = units * 100.0;
    if (!std::isfinite(cents) || cents > 9.0e18 || cents < -9.0e18) {
        throw std::invalid_argument("Invalid money units");
    }
    return Money(static_cast<std::int64_t>(std::llround(cents)));
}

Money Money::parse(const std::string& text) {
    std::string cleaned;
    cleaned.reserve(text.size());
    for (char c : text) {
        if (c == ' ' || c == '\t' || c == ',') {
            continue;
        }
        if (c == '$') {
            continue;
        }
        cleaned.push_back(c);
    }
    if (cleaned.empty()) {
        return Money(0);
    }

    bool negative = false;
    std::size_t index = 0;
    if (cleaned[0] == '-') {
        negative = true;
        index = 1;
    } else if (cleaned[0] == '+') {
        index = 1;
    }

    std::string integer_part;
    std::string decimal_part;
    bool seen_dot = false;
    for (; index < cleaned.size(); ++index) {
        char c = cleaned[index];
        if (c == '.') {
            if (seen_dot) {
                throw std::invalid_argument("Invalid money format");
            }
            seen_dot = true;
            continue;
        }
        if (c < '0' || c > '9') {
            throw std::invalid_argument("Invalid money format");
        }
        if (seen_dot) {
            decimal_part.push_back(c);
        } else {
            integer_part.push_back(c);
        }
    }

    if (integer_part.empty() && decimal_part.empty()) {
        throw std::invalid_argument("Invalid money format");
    }

    while (decimal_part.size() < 2) {
        decimal_part.push_back('0');
    }
    if (decimal_part.size() > 2) {
        decimal_part = decimal_part.substr(0, 2);
    }

    std::int64_t units = integer_part.empty() ? 0 : std::stoll(integer_part);
    std::int64_t cents = decimal_part.empty() ? 0 : std::stoll(decimal_part);
    std::int64_t total = units * 100 + cents;
    if (negative) {
        total = -total;
    }
    return Money(total);
}

std::int64_t Money::cents() const {
    return cents_;
}

double Money::units() const {
    return static_cast<double>(cents_) / 100.0;
}

Money Money::operator+(const Money& other) const {
    return Money(cents_ + other.cents_);
}

Money Money::operator-(const Money& other) const {
    return Money(cents_ - other.cents_);
}

Money& Money::operator+=(const Money& other) {
    cents_ += other.cents_;
    return *this;
}

Money& Money::operator-=(const Money& other) {
    cents_ -= other.cents_;
    return *this;
}

bool Money::operator==(const Money& other) const {
    return cents_ == other.cents_;
}

bool Money::operator!=(const Money& other) const {
    return cents_ != other.cents_;
}

bool Money::operator<(const Money& other) const {
    return cents_ < other.cents_;
}

bool Money::operator<=(const Money& other) const {
    return cents_ <= other.cents_;
}

bool Money::operator>(const Money& other) const {
    return cents_ > other.cents_;
}

bool Money::operator>=(const Money& other) const {
    return cents_ >= other.cents_;
}

bool Money::is_positive() const {
    return cents_ > 0;
}

bool Money::is_zero() const {
    return cents_ == 0;
}

std::string Money::to_string() const {
    std::int64_t absolute = cents_ < 0 ? -cents_ : cents_;
    std::int64_t units = absolute / 100;
    std::int64_t decimals = absolute % 100;

    std::ostringstream stream;
    if (cents_ < 0) {
        stream << "-";
    }
    stream << units << "." << std::setw(2) << std::setfill('0') << decimals;
    return stream.str();
}

}  // namespace atm
