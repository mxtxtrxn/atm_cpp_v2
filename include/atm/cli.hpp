#ifndef ATM_CLI_HPP
#define ATM_CLI_HPP

#include <string>
#include <vector>

#include "atm/bank.hpp"
#include "atm/ui.hpp"

namespace atm {

// Options accepted by the interactive and batch entry points.
struct CliOptions {
    std::string accounts_path = "data/accounts.csv";
    std::string transactions_path = "data/transactions.csv";
    bool seed_demo_data = false;
    bool non_interactive = false;
    std::string command;
    std::vector<std::string> arguments;
};

class Cli {
public:
    Cli(Bank& bank, Console& console, CliOptions options);

    // Runs the interactive menu loop.
    int run_interactive();

    // Runs a single batch command (non-interactive mode).
    int run_batch();

private:
    void handle_balance();
    void handle_withdraw();
    void handle_deposit();
    void handle_transfer();
    void handle_history();
    void handle_profile();
    void handle_change_pin();
    void handle_change_phone();
    void handle_logout();

    bool require_session();
    bool prompt_credentials();

    Bank& bank_;
    Console& console_;
    CliOptions options_;
    std::string session_account_;
};

// Parses command-line arguments into CliOptions.
bool parse_arguments(int argc, char** argv, CliOptions& options, std::string& error);

// Creates a set of synthetic demo accounts with clearly fake data.
std::vector<Account> make_demo_accounts();

// Seeds a bank with demo accounts if it is empty.
void seed_if_empty(Bank& bank);

}  // namespace atm

#endif
