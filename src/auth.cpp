#include "atm/auth.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>

namespace atm {

namespace {

std::uint64_t fnv1a(const std::string& input) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : input) {
        hash ^= static_cast<std::uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string to_hex(std::uint64_t value) {
    std::ostringstream stream;
    stream << std::hex << std::setw(16) << std::setfill('0') << value;
    return stream.str();
}

}  // namespace

std::string hash_pin(const std::string& pin) {
    // Two independent rounds reduce trivial collisions for the short PIN domain.
    std::uint64_t first = fnv1a("atm-pin:" + pin);
    std::uint64_t second = fnv1a("pin-atm:" + pin + ":salt");
    return to_hex(first) + to_hex(second);
}

bool verify_pin(const std::string& pin, const std::string& expected_hash) {
    return hash_pin(pin) == expected_hash;
}

bool is_valid_pin_format(const std::string& pin) {
    if (pin.size() != 4) {
        return false;
    }
    for (char c : pin) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return true;
}

}  // namespace atm
