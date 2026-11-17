#ifndef ATM_BANK_HPP
#define ATM_BANK_HPP

#include <string>
#include <vector>

#include "atm/account.hpp"
#include "atm/money.hpp"
#include "atm/transaction.hpp"

namespace atm {

enum class OperationResult {
    Success,
    AccountNotFound,
    InvalidPin,
    InvalidAmount,
    InsufficientFunds,
    AccountLocked,
    AccountClosed,
    LimitExceeded,
    InvalidInput,
    StorageFailure
};

std::string to_string(OperationResult result);

struct BankConfig {
    Money daily_withdrawal_limit;
    int max_failed_attempts = 3;
    Money minimum_balance;
};

class Bank {
public:
    explicit Bank(BankConfig config = BankConfig{});

    // Account management
    bool add_account(const Account& account);
    bool remove_account(const std::string& account_number);
    Account* find_account(const std::string& account_number);
    const Account* find_account(const std::string& account_number) const;
    const std::vector<Account>& accounts() const;
    std::size_t account_count() const;

    // Authentication
    OperationResult authenticate(const std::string& account_number,
                                 const std::string& pin,
                                 std::string& error_detail);

    // Transactions
    OperationResult deposit(const std::string& account_number, Money amount,
                            std::string& error_detail);
    OperationResult withdraw(const std::string& account_number, const std::string& pin,
                             Money amount, std::string& error_detail);
    OperationResult transfer(const std::string& from_account, const std::string& pin,
                             const std::string& to_account, Money amount,
                             std::string& error_detail);
    OperationResult balance(const std::string& account_number, const std::string& pin,
                            Money& out_balance, std::string& error_detail);

    // Profile updates
    OperationResult change_pin(const std::string& account_number, const std::string& old_pin,
                               const std::string& new_pin, std::string& error_detail);
    OperationResult change_phone(const std::string& account_number, const std::string& pin,
                                 const std::string& new_phone, std::string& error_detail);

    // History
    std::vector<Transaction> history_for(const std::string& account_number) const;

    // Persistence
    bool load(const std::string& accounts_path, const std::string& transactions_path,
              std::string& error_detail);
    bool save(const std::string& accounts_path, const std::string& transactions_path,
              std::string& error_detail) const;

    // Daily withdrawal tracking (in-memory)
    Money withdrawn_today(const std::string& account_number) const;
    void reset_daily_limits();

    const BankConfig& config() const;
    void set_config(const BankConfig& config);

private:
    Account* find_mutable(const std::string& account_number);
    OperationResult check_amount(Money amount) const;
    OperationResult apply_withdrawal(Account& account, Money amount, const std::string& note,
                                     std::string& error_detail);
    std::string next_transaction_id();
    std::string timestamp_now() const;
    void record_transaction(const std::string& account_number, TransactionType type,
                            Money amount, Money balance_after, const std::string& note);
    bool account_has_daily_capacity(const std::string& account_number, Money amount) const;
    void register_daily_withdrawal(const std::string& account_number, Money amount);

    BankConfig config_;
    std::vector<Account> accounts_;
    TransactionLog log_;
    std::vector<std::pair<std::string, Money>> daily_withdrawals_;
    std::uint64_t transaction_counter_;
};

}  // namespace atm

#endif
