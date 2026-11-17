#ifndef ATM_STORAGE_HPP
#define ATM_STORAGE_HPP

#include <string>
#include <vector>

#include "atm/account.hpp"
#include "atm/transaction.hpp"

namespace atm {

struct StorageError {
    bool ok;
    std::string message;

    static StorageError success();
    static StorageError failure(const std::string& message);
};

// Loads and persists accounts in a CSV file. The file format is:
// account_number,holder_name,pin_hash,balance_cents,status,failed_attempts,phone_number
StorageError load_accounts(const std::string& path, std::vector<Account>& out);

StorageError save_accounts(const std::string& path, const std::vector<Account>& accounts);

// Loads and persists transaction history in a CSV file. The file format is:
// id,account_number,type,amount_cents,balance_after_cents,timestamp,note
StorageError load_transactions(const std::string& path, std::vector<Transaction>& out);

StorageError save_transactions(const std::string& path, const std::vector<Transaction>& transactions);

// Creates the directory portion of a path if it does not exist.
bool ensure_parent_directory(const std::string& path);

// Escapes/unescapes commas and quotes for the simple CSV format.
std::string csv_escape(const std::string& value);
std::vector<std::string> csv_split(const std::string& line);

}  // namespace atm

#endif
