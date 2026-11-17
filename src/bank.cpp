#include "atm/bank.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "atm/auth.hpp"
#include "atm/storage.hpp"

namespace atm {

std::string to_string(OperationResult result) {
    switch (result) {
        case OperationResult::Success:
            return "success";
        case OperationResult::AccountNotFound:
            return "account_not_found";
        case OperationResult::InvalidPin:
            return "invalid_pin";
        case OperationResult::InvalidAmount:
            return "invalid_amount";
        case OperationResult::InsufficientFunds:
            return "insufficient_funds";
        case OperationResult::AccountLocked:
            return "account_locked";
        case OperationResult::AccountClosed:
            return "account_closed";
        case OperationResult::LimitExceeded:
            return "limit_exceeded";
        case OperationResult::InvalidInput:
            return "invalid_input";
        case OperationResult::StorageFailure:
            return "storage_failure";
    }
    return "unknown";
}

namespace {

BankConfig default_config() {
    BankConfig config;
    config.daily_withdrawal_limit = Money::from_units(5000.0);
    config.max_failed_attempts = 3;
    config.minimum_balance = Money::from_cents(0);
    return config;
}

}  // namespace

Bank::Bank(BankConfig config)
    : config_(config),
      accounts_(),
      log_(),
      daily_withdrawals_(),
      transaction_counter_(0) {
    if (config_.daily_withdrawal_limit.is_zero()) {
        config_.daily_withdrawal_limit = default_config().daily_withdrawal_limit;
    }
    if (config_.max_failed_attempts <= 0) {
        config_.max_failed_attempts = default_config().max_failed_attempts;
    }
}

bool Bank::add_account(const Account& account) {
    if (find_account(account.account_number()) != nullptr) {
        return false;
    }
    accounts_.push_back(account);
    return true;
}

bool Bank::remove_account(const std::string& account_number) {
    auto it = std::remove_if(accounts_.begin(), accounts_.end(),
                             [&](const Account& account) {
                                 return account.account_number() == account_number;
                             });
    if (it == accounts_.end()) {
        return false;
    }
    accounts_.erase(it, accounts_.end());
    return true;
}

Account* Bank::find_account(const std::string& account_number) {
    return find_mutable(account_number);
}

const Account* Bank::find_account(const std::string& account_number) const {
    for (const auto& account : accounts_) {
        if (account.account_number() == account_number) {
            return &account;
        }
    }
    return nullptr;
}

const std::vector<Account>& Bank::accounts() const {
    return accounts_;
}

std::size_t Bank::account_count() const {
    return accounts_.size();
}

OperationResult Bank::authenticate(const std::string& account_number,
                                   const std::string& pin,
                                   std::string& error_detail) {
    Account* account = find_mutable(account_number);
    if (account == nullptr) {
        error_detail = "Cuenta no encontrada";
        return OperationResult::AccountNotFound;
    }
    if (account->status() == AccountStatus::Closed) {
        error_detail = "La cuenta esta cerrada";
        return OperationResult::AccountClosed;
    }
    if (account->status() == AccountStatus::Locked) {
        error_detail = "La cuenta esta bloqueada";
        return OperationResult::AccountLocked;
    }
    if (!is_valid_pin_format(pin)) {
        error_detail = "El PIN debe tener cuatro digitos";
        return OperationResult::InvalidPin;
    }
    if (!verify_pin(pin, account->pin_hash())) {
        account->register_failed_attempt();
        if (account->failed_attempts() >= config_.max_failed_attempts) {
            account->set_status(AccountStatus::Locked);
            error_detail = "Cuenta bloqueada por intentos fallidos";
            return OperationResult::AccountLocked;
        }
        error_detail = "PIN incorrecto";
        return OperationResult::InvalidPin;
    }
    account->reset_failed_attempts();
    return OperationResult::Success;
}

OperationResult Bank::deposit(const std::string& account_number, Money amount,
                              std::string& error_detail) {
    OperationResult amount_check = check_amount(amount);
    if (amount_check != OperationResult::Success) {
        error_detail = "Monto invalido";
        return amount_check;
    }
    Account* account = find_mutable(account_number);
    if (account == nullptr) {
        error_detail = "Cuenta no encontrada";
        return OperationResult::AccountNotFound;
    }
    if (account->status() != AccountStatus::Active) {
        error_detail = "La cuenta no esta activa";
        return OperationResult::AccountLocked;
    }
    account->credit(amount);
    record_transaction(account_number, TransactionType::Deposit, amount, account->balance(), "Deposito");
    return OperationResult::Success;
}

OperationResult Bank::withdraw(const std::string& account_number, const std::string& pin,
                               Money amount, std::string& error_detail) {
    OperationResult auth = authenticate(account_number, pin, error_detail);
    if (auth != OperationResult::Success) {
        return auth;
    }
    OperationResult amount_check = check_amount(amount);
    if (amount_check != OperationResult::Success) {
        error_detail = "Monto invalido";
        return amount_check;
    }
    Account* account = find_mutable(account_number);
    if (account == nullptr) {
        error_detail = "Cuenta no encontrada";
        return OperationResult::AccountNotFound;
    }
    if (!account_has_daily_capacity(account_number, amount)) {
        error_detail = "Limite diario de retiro excedido";
        return OperationResult::LimitExceeded;
    }
    if (!account->can_withdraw(amount)) {
        error_detail = "Saldo insuficiente";
        return OperationResult::InsufficientFunds;
    }
    OperationResult applied = apply_withdrawal(*account, amount, "Retiro en efectivo", error_detail);
    if (applied == OperationResult::Success) {
        register_daily_withdrawal(account_number, amount);
    }
    return applied;
}

OperationResult Bank::transfer(const std::string& from_account, const std::string& pin,
                               const std::string& to_account, Money amount,
                               std::string& error_detail) {
    OperationResult auth = authenticate(from_account, pin, error_detail);
    if (auth != OperationResult::Success) {
        return auth;
    }
    OperationResult amount_check = check_amount(amount);
    if (amount_check != OperationResult::Success) {
        error_detail = "Monto invalido";
        return amount_check;
    }
    if (from_account == to_account) {
        error_detail = "La cuenta destino debe ser distinta";
        return OperationResult::InvalidInput;
    }
    Account* source = find_mutable(from_account);
    Account* target = find_mutable(to_account);
    if (source == nullptr || target == nullptr) {
        error_detail = "Cuenta no encontrada";
        return OperationResult::AccountNotFound;
    }
    if (target->status() != AccountStatus::Active) {
        error_detail = "La cuenta destino no esta activa";
        return OperationResult::AccountClosed;
    }
    if (!account_has_daily_capacity(from_account, amount)) {
        error_detail = "Limite diario de retiro excedido";
        return OperationResult::LimitExceeded;
    }
    if (!source->can_withdraw(amount)) {
        error_detail = "Saldo insuficiente";
        return OperationResult::InsufficientFunds;
    }
    OperationResult applied = apply_withdrawal(*source, amount, "Transferencia enviada", error_detail);
    if (applied != OperationResult::Success) {
        return applied;
    }
    target->credit(amount);
    record_transaction(to_account, TransactionType::TransferIn, amount, target->balance(),
                       "Transferencia recibida de " + from_account);
    register_daily_withdrawal(from_account, amount);
    return OperationResult::Success;
}

OperationResult Bank::balance(const std::string& account_number, const std::string& pin,
                              Money& out_balance, std::string& error_detail) {
    OperationResult auth = authenticate(account_number, pin, error_detail);
    if (auth != OperationResult::Success) {
        return auth;
    }
    const Account* account = find_account(account_number);
    if (account == nullptr) {
        error_detail = "Cuenta no encontrada";
        return OperationResult::AccountNotFound;
    }
    out_balance = account->balance();
    record_transaction(account_number, TransactionType::BalanceInquiry, Money(),
                       account->balance(), "Consulta de saldo");
    return OperationResult::Success;
}

OperationResult Bank::change_pin(const std::string& account_number, const std::string& old_pin,
                                 const std::string& new_pin, std::string& error_detail) {
    OperationResult auth = authenticate(account_number, old_pin, error_detail);
    if (auth != OperationResult::Success) {
        return auth;
    }
    if (!is_valid_pin_format(new_pin)) {
        error_detail = "El nuevo PIN debe tener cuatro digitos";
        return OperationResult::InvalidPin;
    }
    if (old_pin == new_pin) {
        error_detail = "El nuevo PIN debe ser distinto";
        return OperationResult::InvalidInput;
    }
    Account* account = find_mutable(account_number);
    if (account == nullptr) {
        error_detail = "Cuenta no encontrada";
        return OperationResult::AccountNotFound;
    }
    account->set_pin_hash(hash_pin(new_pin));
    return OperationResult::Success;
}

OperationResult Bank::change_phone(const std::string& account_number, const std::string& pin,
                                   const std::string& new_phone, std::string& error_detail) {
    OperationResult auth = authenticate(account_number, pin, error_detail);
    if (auth != OperationResult::Success) {
        return auth;
    }
    if (new_phone.empty()) {
        error_detail = "El telefono no puede estar vacio";
        return OperationResult::InvalidInput;
    }
    Account* account = find_mutable(account_number);
    if (account == nullptr) {
        error_detail = "Cuenta no encontrada";
        return OperationResult::AccountNotFound;
    }
    account->set_phone_number(new_phone);
    return OperationResult::Success;
}

std::vector<Transaction> Bank::history_for(const std::string& account_number) const {
    return log_.for_account(account_number);
}

bool Bank::load(const std::string& accounts_path, const std::string& transactions_path,
                std::string& error_detail) {
    std::vector<Account> accounts;
    StorageError accounts_result = load_accounts(accounts_path, accounts);
    if (!accounts_result.ok) {
        error_detail = accounts_result.message;
        return false;
    }
    std::vector<Transaction> transactions;
    std::error_code file_error;
    const bool has_transactions = std::filesystem::exists(transactions_path, file_error);
    if (has_transactions) {
        StorageError transactions_result = load_transactions(transactions_path, transactions);
        if (!transactions_result.ok) {
            error_detail = transactions_result.message;
            return false;
        }
    }
    accounts_ = std::move(accounts);
    log_.clear();
    for (const auto& tx : transactions) {
        log_.add(tx);
    }
    transaction_counter_ = log_.size();
    return true;
}

bool Bank::save(const std::string& accounts_path, const std::string& transactions_path,
                std::string& error_detail) const {
    StorageError accounts_result = save_accounts(accounts_path, accounts_);
    if (!accounts_result.ok) {
        error_detail = accounts_result.message;
        return false;
    }
    StorageError transactions_result = save_transactions(transactions_path, log_.entries());
    if (!transactions_result.ok) {
        error_detail = transactions_result.message;
        return false;
    }
    return true;
}

Money Bank::withdrawn_today(const std::string& account_number) const {
    for (const auto& entry : daily_withdrawals_) {
        if (entry.first == account_number) {
            return entry.second;
        }
    }
    return Money();
}

void Bank::reset_daily_limits() {
    daily_withdrawals_.clear();
}

const BankConfig& Bank::config() const {
    return config_;
}

void Bank::set_config(const BankConfig& config) {
    config_ = config;
    if (config_.daily_withdrawal_limit.is_zero()) {
        config_.daily_withdrawal_limit = default_config().daily_withdrawal_limit;
    }
    if (config_.max_failed_attempts <= 0) {
        config_.max_failed_attempts = default_config().max_failed_attempts;
    }
}

Account* Bank::find_mutable(const std::string& account_number) {
    for (auto& account : accounts_) {
        if (account.account_number() == account_number) {
            return &account;
        }
    }
    return nullptr;
}

OperationResult Bank::check_amount(Money amount) const {
    if (!amount.is_positive()) {
        return OperationResult::InvalidAmount;
    }
    return OperationResult::Success;
}

OperationResult Bank::apply_withdrawal(Account& account, Money amount, const std::string& note,
                                       std::string& error_detail) {
    if (!account.can_withdraw(amount)) {
        error_detail = "Saldo insuficiente";
        return OperationResult::InsufficientFunds;
    }
    account.debit(amount);
    record_transaction(account.account_number(), TransactionType::Withdrawal, amount,
                       account.balance(), note);
    return OperationResult::Success;
}

std::string Bank::next_transaction_id() {
    ++transaction_counter_;
    std::ostringstream stream;
    stream << "TX" << std::setw(6) << std::setfill('0') << transaction_counter_;
    return stream.str();
}

std::string Bank::timestamp_now() const {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_value{};
#if defined(_WIN32)
    localtime_s(&tm_value, &time);
#else
    localtime_r(&time, &tm_value);
#endif
    std::ostringstream stream;
    stream << std::put_time(&tm_value, "%Y-%m-%dT%H:%M:%S");
    return stream.str();
}

void Bank::record_transaction(const std::string& account_number, TransactionType type,
                              Money amount, Money balance_after, const std::string& note) {
    Transaction tx;
    tx.id = next_transaction_id();
    tx.account_number = account_number;
    tx.type = type;
    tx.amount = amount;
    tx.balance_after = balance_after;
    tx.timestamp = timestamp_now();
    tx.note = note;
    log_.add(tx);
}

bool Bank::account_has_daily_capacity(const std::string& account_number, Money amount) const {
    if (config_.daily_withdrawal_limit.is_zero()) {
        return true;
    }
    Money already = withdrawn_today(account_number);
    return already + amount <= config_.daily_withdrawal_limit;
}

void Bank::register_daily_withdrawal(const std::string& account_number, Money amount) {
    for (auto& entry : daily_withdrawals_) {
        if (entry.first == account_number) {
            entry.second += amount;
            return;
        }
    }
    daily_withdrawals_.emplace_back(account_number, amount);
}

}  // namespace atm
