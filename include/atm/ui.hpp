#ifndef ATM_UI_HPP
#define ATM_UI_HPP

#include <istream>
#include <ostream>
#include <string>

#include "atm/account.hpp"
#include "atm/money.hpp"
#include "atm/transaction.hpp"

namespace atm {

// Console abstraction that keeps input/output testable and platform neutral.
class Console {
public:
    Console(std::istream& in, std::ostream& out);
    explicit Console(std::ostream& out);

    std::string read_line(const std::string& prompt);
    std::string read_password(const std::string& prompt);
    int read_int(const std::string& prompt, bool& ok);
    double read_double(const std::string& prompt, bool& ok);

    void print_line(const std::string& text = "");
    void print_header(const std::string& title);
    void print_error(const std::string& text);
    void print_success(const std::string& text);
    void print_money(const std::string& label, Money amount);
    void print_transaction(const Transaction& tx);
    void print_account_summary(const Account& account);
    void pause();

private:
    std::istream* in_;
    std::ostream& out_;
};

}  // namespace atm

#endif
