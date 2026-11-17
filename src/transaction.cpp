#include "atm/transaction.hpp"

#include <cctype>

namespace atm {

std::string to_string(TransactionType type) {
    switch (type) {
        case TransactionType::Deposit:
            return "deposit";
        case TransactionType::Withdrawal:
            return "withdrawal";
        case TransactionType::TransferIn:
            return "transfer_in";
        case TransactionType::TransferOut:
            return "transfer_out";
        case TransactionType::BalanceInquiry:
            return "balance_inquiry";
    }
    return "unknown";
}

bool parse_transaction_type(const std::string& text, TransactionType& out) {
    std::string lowered;
    lowered.reserve(text.size());
    for (char c : text) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    if (lowered == "deposit") {
        out = TransactionType::Deposit;
        return true;
    }
    if (lowered == "withdrawal") {
        out = TransactionType::Withdrawal;
        return true;
    }
    if (lowered == "transfer_in") {
        out = TransactionType::TransferIn;
        return true;
    }
    if (lowered == "transfer_out") {
        out = TransactionType::TransferOut;
        return true;
    }
    if (lowered == "balance_inquiry") {
        out = TransactionType::BalanceInquiry;
        return true;
    }
    return false;
}

void TransactionLog::add(const Transaction& tx) {
    entries_.push_back(tx);
}

const std::vector<Transaction>& TransactionLog::entries() const {
    return entries_;
}

std::vector<Transaction> TransactionLog::for_account(const std::string& account_number) const {
    std::vector<Transaction> result;
    for (const auto& tx : entries_) {
        if (tx.account_number == account_number) {
            result.push_back(tx);
        }
    }
    return result;
}

std::size_t TransactionLog::size() const {
    return entries_.size();
}

void TransactionLog::clear() {
    entries_.clear();
}

}  // namespace atm
