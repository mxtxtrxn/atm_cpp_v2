#include "atm/storage.hpp"

#include <exception>
#include <filesystem>
#include <fstream>

namespace atm {

StorageError StorageError::success() {
    return StorageError{true, ""};
}

StorageError StorageError::failure(const std::string& message) {
    return StorageError{false, message};
}

std::string csv_escape(const std::string& value) {
    bool needs_quotes = value.find(',') != std::string::npos ||
                        value.find('"') != std::string::npos ||
                        value.find('\n') != std::string::npos;
    if (!needs_quotes) {
        return value;
    }
    std::string escaped;
    escaped.push_back('"');
    for (char c : value) {
        if (c == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(c);
    }
    escaped.push_back('"');
    return escaped;
}

std::vector<std::string> csv_split(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool in_quotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current.push_back('"');
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                current.push_back(c);
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else if (c != '\r') {
                current.push_back(c);
            }
        }
    }
    fields.push_back(current);
    return fields;
}

bool ensure_parent_directory(const std::string& path) {
    std::filesystem::path p(path);
    std::filesystem::path parent = p.parent_path();
    if (parent.empty()) {
        return true;
    }
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    return !ec;
}

StorageError load_accounts(const std::string& path, std::vector<Account>& out) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return StorageError::failure("No se pudo abrir el archivo de cuentas: " + path);
    }

    std::vector<Account> accounts;
    std::string line;
    bool first = true;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        if (first) {
            first = false;
            // Skip header if present.
            if (line.find("account_number") != std::string::npos) {
                continue;
            }
        }
        auto fields = csv_split(line);
        if (fields.size() < 7) {
            return StorageError::failure("Registro de cuenta invalido en: " + path);
        }
        try {
            AccountStatus status = AccountStatus::Active;
            if (!parse_account_status(fields[4], status)) {
                return StorageError::failure("Estado de cuenta invalido: " + fields[4]);
            }
            Account account(fields[0], fields[1], fields[2],
                            Money::from_cents(std::stoll(fields[3])), status);
            account.set_failed_attempts(std::stoi(fields[5]));
            account.set_phone_number(fields[6]);
            accounts.push_back(std::move(account));
        } catch (const std::exception&) {
            return StorageError::failure("Valor numerico invalido en el archivo de cuentas: " + path);
        }
    }

    out = std::move(accounts);
    return StorageError::success();
}

StorageError save_accounts(const std::string& path, const std::vector<Account>& accounts) {
    if (!ensure_parent_directory(path)) {
        return StorageError::failure("No se pudo crear el directorio para: " + path);
    }
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        return StorageError::failure("No se pudo escribir el archivo de cuentas: " + path);
    }
    output << "account_number,holder_name,pin_hash,balance_cents,status,failed_attempts,phone_number\n";
    for (const auto& account : accounts) {
        output << csv_escape(account.account_number()) << ','
               << csv_escape(account.holder_name()) << ','
               << csv_escape(account.pin_hash()) << ','
               << account.balance().cents() << ','
               << to_string(account.status()) << ','
               << account.failed_attempts() << ','
               << csv_escape(account.phone_number()) << '\n';
    }
    return StorageError::success();
}

StorageError load_transactions(const std::string& path, std::vector<Transaction>& out) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return StorageError::failure("No se pudo abrir el archivo de transacciones: " + path);
    }

    std::vector<Transaction> transactions;
    std::string line;
    bool first = true;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        if (first) {
            first = false;
            if (line.find("id,account_number") != std::string::npos) {
                continue;
            }
        }
        auto fields = csv_split(line);
        if (fields.size() < 7) {
            return StorageError::failure("Registro de transaccion invalido en: " + path);
        }
        try {
            Transaction tx;
            tx.id = fields[0];
            tx.account_number = fields[1];
            TransactionType type = TransactionType::Deposit;
            if (!parse_transaction_type(fields[2], type)) {
                return StorageError::failure("Tipo de transaccion invalido: " + fields[2]);
            }
            tx.type = type;
            tx.amount = Money::from_cents(std::stoll(fields[3]));
            tx.balance_after = Money::from_cents(std::stoll(fields[4]));
            tx.timestamp = fields[5];
            tx.note = fields[6];
            transactions.push_back(std::move(tx));
        } catch (const std::exception&) {
            return StorageError::failure("Valor numerico invalido en el archivo de transacciones: " + path);
        }
    }

    out = std::move(transactions);
    return StorageError::success();
}

StorageError save_transactions(const std::string& path, const std::vector<Transaction>& transactions) {
    if (!ensure_parent_directory(path)) {
        return StorageError::failure("No se pudo crear el directorio para: " + path);
    }
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        return StorageError::failure("No se pudo escribir el archivo de transacciones: " + path);
    }
    output << "id,account_number,type,amount_cents,balance_after_cents,timestamp,note\n";
    for (const auto& tx : transactions) {
        output << csv_escape(tx.id) << ','
               << csv_escape(tx.account_number) << ','
               << to_string(tx.type) << ','
               << tx.amount.cents() << ','
               << tx.balance_after.cents() << ','
               << csv_escape(tx.timestamp) << ','
               << csv_escape(tx.note) << '\n';
    }
    return StorageError::success();
}

}  // namespace atm
