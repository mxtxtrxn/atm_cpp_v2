#ifndef ATM_TRANSACTION_HPP
#define ATM_TRANSACTION_HPP

#include <string>
#include <vector>

#include "atm/money.hpp"

namespace atm {

enum class TransactionType {
    Deposit,
    Withdrawal,
    TransferIn,
    TransferOut,
    BalanceInquiry
};

std::string to_string(TransactionType type);
bool parse_transaction_type(const std::string& text, TransactionType& out);

struct Transaction {
    std::string id;
    std::string account_number;
    TransactionType type;
    Money amount;
    Money balance_after;
    std::string timestamp;
    std::string note;
};

class TransactionLog {
public:
    void add(const Transaction& tx);
    const std::vector<Transaction>& entries() const;
    std::vector<Transaction> for_account(const std::string& account_number) const;
    std::size_t size() const;
    void clear();

private:
    std::vector<Transaction> entries_;
};

}  // namespace atm

#endif
