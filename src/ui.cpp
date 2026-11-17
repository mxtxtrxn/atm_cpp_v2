#include "atm/ui.hpp"

#include <cmath>
#include <iostream>

#include "atm/auth.hpp"
#include "atm/transaction.hpp"

namespace atm {

Console::Console(std::istream& in, std::ostream& out) : in_(&in), out_(out) {}

Console::Console(std::ostream& out) : in_(&std::cin), out_(out) {}

std::string Console::read_line(const std::string& prompt) {
    out_ << prompt;
    out_.flush();
    std::string value;
    std::getline(*in_, value);
    return value;
}

std::string Console::read_password(const std::string& prompt) {
    // Password entry is intentionally plain in this portable implementation.
    return read_line(prompt);
}

int Console::read_int(const std::string& prompt, bool& ok) {
    std::string value = read_line(prompt);
    try {
        std::size_t pos = 0;
        int parsed = std::stoi(value, &pos);
        if (pos != value.size()) {
            ok = false;
            return 0;
        }
        ok = true;
        return parsed;
    } catch (...) {
        ok = false;
        return 0;
    }
}

double Console::read_double(const std::string& prompt, bool& ok) {
    std::string value = read_line(prompt);
    try {
        std::size_t pos = 0;
        double parsed = std::stod(value, &pos);
        if (pos != value.size() || !std::isfinite(parsed) || parsed > 1.0e12 ||
            parsed < -1.0e12) {
            ok = false;
            return 0.0;
        }
        ok = true;
        return parsed;
    } catch (...) {
        ok = false;
        return 0.0;
    }
}

void Console::print_line(const std::string& text) {
    out_ << text << '\n';
}

void Console::print_header(const std::string& title) {
    out_ << "\n=== " << title << " ===\n";
}

void Console::print_error(const std::string& text) {
    out_ << "[ERROR] " << text << '\n';
}

void Console::print_success(const std::string& text) {
    out_ << "[OK] " << text << '\n';
}

void Console::print_money(const std::string& label, Money amount) {
    out_ << label << ": " << amount.to_string() << '\n';
}

void Console::print_transaction(const Transaction& tx) {
    out_ << tx.timestamp << " | " << to_string(tx.type) << " | "
         << tx.amount.to_string() << " | saldo " << tx.balance_after.to_string();
    if (!tx.note.empty()) {
        out_ << " | " << tx.note;
    }
    out_ << '\n';
}

void Console::print_account_summary(const Account& account) {
    out_ << "Cuenta: " << account.account_number() << '\n';
    out_ << "Titular: " << account.holder_name() << '\n';
    out_ << "Saldo: " << account.balance().to_string() << '\n';
    out_ << "Telefono: " << account.phone_number() << '\n';
    out_ << "Estado: " << to_string(account.status()) << '\n';
}

void Console::pause() {
    out_ << "Presione ENTER para continuar...";
    out_.flush();
    std::string ignored;
    std::getline(*in_, ignored);
}

}  // namespace atm
