#ifndef ATM_AUTH_HPP
#define ATM_AUTH_HPP

#include <string>

namespace atm {

// Produces a stable, non-reversible hash for a PIN using a simple,
// self-contained algorithm. This is not a replacement for a real
// cryptographic hash in production; it avoids external dependencies.
std::string hash_pin(const std::string& pin);

bool verify_pin(const std::string& pin, const std::string& expected_hash);

// Validates that a PIN is exactly four digits.
bool is_valid_pin_format(const std::string& pin);

}  // namespace atm

#endif
