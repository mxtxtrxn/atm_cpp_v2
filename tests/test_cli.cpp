#include <sstream>
#include <string>
#include <vector>

#include "atm/auth.hpp"
#include "atm/cli.hpp"
#include "atm/ui.hpp"
#include "test_framework.hpp"

using namespace atm;

namespace {

bool parse(const std::vector<std::string>& args, CliOptions& options, std::string& error) {
    std::vector<std::string> storage = args;
    std::vector<char*> pointers;
    pointers.reserve(storage.size());
    for (auto& value : storage) {
        pointers.push_back(value.data());
    }
    return parse_arguments(static_cast<int>(pointers.size()), pointers.data(), options, error);
}

Bank seeded_bank() {
    Bank bank;
    seed_if_empty(bank);
    return bank;
}

}  // namespace

ATM_TEST(cli_parse_default_options) {
    CliOptions options;
    std::string error;
    CHECK(parse({"atm"}, options, error));
    CHECK_EQ(options.accounts_path, std::string("data/accounts.csv"));
    CHECK_EQ(options.transactions_path, std::string("data/transactions.csv"));
    CHECK(!options.seed_demo_data);
    CHECK(!options.non_interactive);
}

ATM_TEST(cli_parse_paths_and_seed_flag) {
    CliOptions options;
    std::string error;
    CHECK(parse({"atm", "--accounts", "a.csv", "--transactions", "t.csv", "--seed-demo"},
                options, error));
    CHECK_EQ(options.accounts_path, std::string("a.csv"));
    CHECK_EQ(options.transactions_path, std::string("t.csv"));
    CHECK(options.seed_demo_data);
    CHECK(!options.non_interactive);
}

ATM_TEST(cli_parse_batch_command) {
    CliOptions options;
    std::string error;
    CHECK(parse({"atm", "--batch", "withdraw", "1000001", "1234", "25.00"}, options, error));
    CHECK(options.non_interactive);
    CHECK_EQ(options.command, std::string("withdraw"));
    REQUIRE(options.arguments.size() == 3);
    CHECK_EQ(options.arguments[0], std::string("1000001"));
    CHECK_EQ(options.arguments[1], std::string("1234"));
    CHECK_EQ(options.arguments[2], std::string("25.00"));
}

ATM_TEST(cli_parse_rejects_invalid_arguments) {
    CliOptions options;
    std::string error;

    CHECK(!parse({"atm", "--unknown"}, options, error));
    CHECK(!error.empty());

    error.clear();
    CHECK(!parse({"atm", "--accounts"}, options, error));
    CHECK(!error.empty());

    error.clear();
    CHECK(!parse({"atm", "--transactions"}, options, error));
    CHECK(!error.empty());

    error.clear();
    CHECK(!parse({"atm", "--batch"}, options, error));
    CHECK(!error.empty());
}

ATM_TEST(cli_parse_help_returns_usage) {
    CliOptions options;
    std::string error;
    CHECK(!parse({"atm", "--help"}, options, error));
    CHECK(error.find("Uso:") != std::string::npos);
}

ATM_TEST(console_read_double_rejects_invalid_values) {
    std::istringstream input("nan\ninf\n1e400\n1e13\n5.5\n");
    std::ostringstream output;
    Console console(input, output);

    bool ok = true;
    CHECK_EQ(console.read_double("", ok), 0.0);
    CHECK(!ok);
    CHECK_EQ(console.read_double("", ok), 0.0);
    CHECK(!ok);
    CHECK_EQ(console.read_double("", ok), 0.0);
    CHECK(!ok);
    CHECK_EQ(console.read_double("", ok), 0.0);
    CHECK(!ok);
    CHECK_EQ(console.read_double("", ok), 5.5);
    CHECK(ok);
}

ATM_TEST(cli_demo_accounts_are_synthetic_and_valid) {
    const auto accounts = make_demo_accounts();
    REQUIRE(accounts.size() == 3);

    CHECK_EQ(accounts[0].account_number(), std::string("1000001"));
    CHECK_EQ(accounts[0].holder_name(), std::string("Usuario Demo Uno"));
    CHECK_EQ(accounts[0].phone_number(), std::string("0000000001"));
    CHECK_EQ(accounts[0].balance(), Money::from_cents(1500000));
    CHECK(verify_pin("1234", accounts[0].pin_hash()));
    CHECK(accounts[0].is_active());

    CHECK_EQ(accounts[1].account_number(), std::string("1000002"));
    CHECK_EQ(accounts[1].holder_name(), std::string("Usuario Demo Dos"));
    CHECK_EQ(accounts[1].phone_number(), std::string("0000000002"));
    CHECK_EQ(accounts[1].balance(), Money::from_cents(850000));
    CHECK(verify_pin("2345", accounts[1].pin_hash()));

    CHECK_EQ(accounts[2].account_number(), std::string("1000003"));
    CHECK_EQ(accounts[2].holder_name(), std::string("Usuario Demo Tres"));
    CHECK_EQ(accounts[2].phone_number(), std::string("0000000003"));
    CHECK_EQ(accounts[2].balance(), Money::from_cents(320000));
    CHECK(verify_pin("3456", accounts[2].pin_hash()));
}

ATM_TEST(cli_seed_if_empty_only_seeds_once) {
    Bank bank;
    CHECK_EQ(bank.account_count(), static_cast<std::size_t>(0));
    seed_if_empty(bank);
    CHECK_EQ(bank.account_count(), static_cast<std::size_t>(3));
    seed_if_empty(bank);
    CHECK_EQ(bank.account_count(), static_cast<std::size_t>(3));

    Bank custom;
    custom.add_account(Account("9000001", "Titular Demo", hash_pin("1111"),
                               Money::from_cents(1), AccountStatus::Active));
    seed_if_empty(custom);
    CHECK_EQ(custom.account_count(), static_cast<std::size_t>(1));
}

ATM_TEST(cli_batch_balance_success) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "balance";
    options.arguments = {"1000001", "1234"};

    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_batch(), 0);
    CHECK(output.str().find("15000.00") != std::string::npos);
}

ATM_TEST(cli_batch_balance_failure) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "balance";
    options.arguments = {"1000001", "0000"};

    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_batch(), 1);
    CHECK(output.str().find("[ERROR]") != std::string::npos);
}

ATM_TEST(cli_batch_usage_errors) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "balance";
    options.arguments = {"1000001"};

    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_batch(), 2);
}

ATM_TEST(cli_batch_withdraw) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "withdraw";
    options.arguments = {"1000001", "1234", "100.00"};

    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_batch(), 0);
    CHECK_EQ(bank.find_account("1000001")->balance(), Money::from_cents(1490000));
    CHECK(output.str().find("[OK]") != std::string::npos);
}

ATM_TEST(cli_batch_withdraw_rejects_bad_input) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "withdraw";
    options.arguments = {"1000001", "1234", "abc"};

    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_batch(), 2);

    options.arguments = {"1000001", "1234", "999999.00"};
    output.str("");
    Cli second(bank, console, options);
    CHECK_EQ(second.run_batch(), 1);

    options.arguments = {"1000001", "1234"};
    output.str("");
    Cli third(bank, console, options);
    CHECK_EQ(third.run_batch(), 2);
}

ATM_TEST(cli_batch_deposit) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "deposit";
    options.arguments = {"1000001", "250.50"};

    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_batch(), 0);
    CHECK_EQ(bank.find_account("1000001")->balance(), Money::from_cents(1525050));
}

ATM_TEST(cli_batch_deposit_rejects_bad_input) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "deposit";

    options.arguments = {"1000001"};
    Cli missing_amount(bank, console, options);
    CHECK_EQ(missing_amount.run_batch(), 2);

    options.arguments = {"1000001", "0"};
    Cli zero_amount(bank, console, options);
    CHECK_EQ(zero_amount.run_batch(), 2);

    options.arguments = {"9999999", "10.00"};
    Cli missing_account(bank, console, options);
    CHECK_EQ(missing_account.run_batch(), 1);
}

ATM_TEST(cli_batch_transfer) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "transfer";
    options.arguments = {"1000001", "1234", "1000002", "500.00"};

    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_batch(), 0);
    CHECK_EQ(bank.find_account("1000001")->balance(), Money::from_cents(1450000));
    CHECK_EQ(bank.find_account("1000002")->balance(), Money::from_cents(900000));
}

ATM_TEST(cli_batch_transfer_rejects_bad_input) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "transfer";

    options.arguments = {"1000001", "1234", "1000002"};
    Cli missing_amount(bank, console, options);
    CHECK_EQ(missing_amount.run_batch(), 2);

    options.arguments = {"1000001", "1234", "1000002", "nope"};
    Cli invalid_amount(bank, console, options);
    CHECK_EQ(invalid_amount.run_batch(), 2);

    options.arguments = {"1000001", "1234", "1000001", "10.00"};
    Cli same_account(bank, console, options);
    CHECK_EQ(same_account.run_batch(), 1);
}

ATM_TEST(cli_batch_history) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "history";
    options.arguments = {"1000001"};

    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_batch(), 0);
    CHECK(output.str().find("Sin movimientos.") != std::string::npos);

    std::string detail;
    bank.deposit("1000001", Money::from_cents(100), detail);
    output.str("");
    CHECK_EQ(cli.run_batch(), 0);
    CHECK(output.str().find("deposit") != std::string::npos);
}

ATM_TEST(cli_batch_unknown_command) {
    Bank bank = seeded_bank();
    std::istringstream input;
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    options.non_interactive = true;
    options.command = "frobnicate";

    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_batch(), 2);
    CHECK(output.str().find("Comando desconocido") != std::string::npos);
}

ATM_TEST(cli_interactive_login_balance_logout) {
    Bank bank = seeded_bank();
    std::istringstream input("1\n1000001\n1234\n1\n\n9\n2\n");
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_interactive(), 0);
    const std::string text = output.str();
    CHECK(text.find("Sesion iniciada") != std::string::npos);
    CHECK(text.find("Saldo disponible") != std::string::npos);
    CHECK(text.find("15000.00") != std::string::npos);
    CHECK(text.find("Sesion cerrada.") != std::string::npos);
    CHECK(text.find("Hasta luego.") != std::string::npos);
}

ATM_TEST(cli_interactive_handles_invalid_menu_input) {
    Bank bank = seeded_bank();
    std::istringstream input("x\n2\n");
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_interactive(), 0);
    CHECK(output.str().find("Opcion invalida") != std::string::npos);
}

ATM_TEST(cli_interactive_handles_failed_login) {
    Bank bank = seeded_bank();
    std::istringstream input("1\n1000001\n0000\n2\n");
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_interactive(), 0);
    CHECK(output.str().find("PIN incorrecto") != std::string::npos);
}

ATM_TEST(cli_interactive_withdrawal_flow) {
    Bank bank = seeded_bank();
    std::istringstream input("1\n1000001\n1234\n2\n120.00\n1234\n\n9\n2\n");
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_interactive(), 0);
    CHECK_EQ(bank.find_account("1000001")->balance(), Money::from_cents(1488000));
    CHECK(output.str().find("Retiro realizado") != std::string::npos);
}

ATM_TEST(cli_interactive_profile_and_history) {
    Bank bank = seeded_bank();
    std::string detail;
    bank.deposit("1000001", Money::from_cents(100), detail);
    std::istringstream input("1\n1000001\n1234\n6\n\n5\n\n9\n2\n");
    std::ostringstream output;
    Console console(input, output);

    CliOptions options;
    Cli cli(bank, console, options);
    CHECK_EQ(cli.run_interactive(), 0);
    const std::string text = output.str();
    CHECK(text.find("Datos del perfil") != std::string::npos);
    CHECK(text.find("Usuario Demo Uno") != std::string::npos);
    CHECK(text.find("Historial de movimientos") != std::string::npos);
}
