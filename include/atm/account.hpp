#ifndef ATM_ACCOUNT_HPP
#define ATM_ACCOUNT_HPP

#include <string>

#include "atm/money.hpp"

namespace atm {

enum class AccountStatus {
    Active,
    Locked,
    Closed
};

std::string to_string(AccountStatus status);
bool parse_account_status(const std::string& text, AccountStatus& out);

class Account {
public:
    Account();
    Account(std::string account_number,
            std::string holder_name,
            std::string pin_hash,
            Money balance,
            AccountStatus status);

    const std::string& account_number() const;
    const std::string& holder_name() const;
    const std::string& pin_hash() const;
    Money balance() const;
    AccountStatus status() const;
    int failed_attempts() const;
    std::string phone_number() const;

    void set_holder_name(const std::string& name);
    void set_phone_number(const std::string& phone);
    void set_status(AccountStatus status);
    void set_balance(Money balance);
    void set_pin_hash(const std::string& hash);
    void set_failed_attempts(int attempts);

    bool is_active() const;
    bool can_withdraw(Money amount) const;

    void credit(Money amount);
    void debit(Money amount);
    void register_failed_attempt();
    void reset_failed_attempts();

private:
    std::string account_number_;
    std::string holder_name_;
    std::string pin_hash_;
    std::string phone_number_;
    Money balance_;
    AccountStatus status_;
    int failed_attempts_;
};

}  // namespace atm

#endif
