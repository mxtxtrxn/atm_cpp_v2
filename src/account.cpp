#include "atm/account.hpp"

#include <cctype>

namespace atm {

std::string to_string(AccountStatus status) {
    switch (status) {
        case AccountStatus::Active:
            return "active";
        case AccountStatus::Locked:
            return "locked";
        case AccountStatus::Closed:
            return "closed";
    }
    return "active";
}

bool parse_account_status(const std::string& text, AccountStatus& out) {
    std::string lowered;
    lowered.reserve(text.size());
    for (char c : text) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    if (lowered == "active") {
        out = AccountStatus::Active;
        return true;
    }
    if (lowered == "locked") {
        out = AccountStatus::Locked;
        return true;
    }
    if (lowered == "closed") {
        out = AccountStatus::Closed;
        return true;
    }
    return false;
}

Account::Account()
    : account_number_(),
      holder_name_(),
      pin_hash_(),
      phone_number_(),
      balance_(),
      status_(AccountStatus::Active),
      failed_attempts_(0) {}

Account::Account(std::string account_number,
                 std::string holder_name,
                 std::string pin_hash,
                 Money balance,
                 AccountStatus status)
    : account_number_(std::move(account_number)),
      holder_name_(std::move(holder_name)),
      pin_hash_(std::move(pin_hash)),
      phone_number_(),
      balance_(balance),
      status_(status),
      failed_attempts_(0) {}

const std::string& Account::account_number() const {
    return account_number_;
}

const std::string& Account::holder_name() const {
    return holder_name_;
}

const std::string& Account::pin_hash() const {
    return pin_hash_;
}

Money Account::balance() const {
    return balance_;
}

AccountStatus Account::status() const {
    return status_;
}

int Account::failed_attempts() const {
    return failed_attempts_;
}

std::string Account::phone_number() const {
    return phone_number_;
}

void Account::set_holder_name(const std::string& name) {
    holder_name_ = name;
}

void Account::set_phone_number(const std::string& phone) {
    phone_number_ = phone;
}

void Account::set_status(AccountStatus status) {
    status_ = status;
}

void Account::set_balance(Money balance) {
    balance_ = balance;
}

void Account::set_pin_hash(const std::string& hash) {
    pin_hash_ = hash;
}

void Account::set_failed_attempts(int attempts) {
    failed_attempts_ = attempts < 0 ? 0 : attempts;
}

bool Account::is_active() const {
    return status_ == AccountStatus::Active;
}

bool Account::can_withdraw(Money amount) const {
    return is_active() && amount.is_positive() && balance_ >= amount;
}

void Account::credit(Money amount) {
    balance_ += amount;
}

void Account::debit(Money amount) {
    balance_ -= amount;
}

void Account::register_failed_attempt() {
    ++failed_attempts_;
}

void Account::reset_failed_attempts() {
    failed_attempts_ = 0;
}

}  // namespace atm
