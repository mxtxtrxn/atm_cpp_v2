#include <filesystem>
#include <fstream>
#include <string>

#include "test_framework.hpp"
#include "test_utils.hpp"

using namespace atm;

namespace {

Bank make_bank() {
    Bank bank;
    bank.add_account(test::make_account("1000001", "1234", 1000000));
    bank.add_account(test::make_account("1000002", "2345", 5000));
    bank.add_account(test::make_account("1000003", "3456", 10000));
    return bank;
}

}  // namespace

ATM_TEST(bank_add_and_remove_accounts) {
    Bank bank;
    CHECK(bank.add_account(test::make_account("1000001", "1234", 1000)));
    CHECK(!bank.add_account(test::make_account("1000001", "9999", 0)));
    CHECK_EQ(bank.account_count(), static_cast<std::size_t>(1));

    CHECK(bank.find_account("1000001") != nullptr);
    CHECK(bank.find_account("9999999") == nullptr);
    CHECK(bank.remove_account("1000001"));
    CHECK(!bank.remove_account("1000001"));
    CHECK_EQ(bank.account_count(), static_cast<std::size_t>(0));
}

ATM_TEST(bank_authentication_success_and_wrong_pin) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.authenticate("1000001", "1234", detail), OperationResult::Success);
    CHECK(detail.empty());
    CHECK_EQ(bank.authenticate("1000001", "0000", detail), OperationResult::InvalidPin);
    CHECK(!detail.empty());
    CHECK_EQ(bank.find_account("1000001")->failed_attempts(), 1);

    CHECK_EQ(bank.authenticate("1000001", "1234", detail), OperationResult::Success);
    CHECK_EQ(bank.find_account("1000001")->failed_attempts(), 0);

    CHECK_EQ(bank.authenticate("9999999", "1234", detail), OperationResult::AccountNotFound);
}

ATM_TEST(bank_locks_after_failed_attempts) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.authenticate("1000001", "1111", detail), OperationResult::InvalidPin);
    CHECK_EQ(bank.authenticate("1000001", "2222", detail), OperationResult::InvalidPin);
    CHECK_EQ(bank.authenticate("1000001", "3333", detail), OperationResult::AccountLocked);
    CHECK_EQ(bank.find_account("1000001")->status(), AccountStatus::Locked);
    CHECK_EQ(bank.authenticate("1000001", "1234", detail), OperationResult::AccountLocked);
}

ATM_TEST(bank_rejects_invalid_pin_format_without_counting_attempt) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.authenticate("1000001", "12", detail), OperationResult::InvalidPin);
    CHECK_EQ(bank.find_account("1000001")->failed_attempts(), 0);
    CHECK_EQ(bank.authenticate("1000001", "abcd", detail), OperationResult::InvalidPin);
    CHECK_EQ(bank.find_account("1000001")->failed_attempts(), 0);
}

ATM_TEST(bank_closed_account_cannot_authenticate) {
    Bank bank;
    bank.add_account(test::make_account("1000001", "1234", 1000, AccountStatus::Closed));
    std::string detail;

    CHECK_EQ(bank.authenticate("1000001", "1234", detail), OperationResult::AccountClosed);
    CHECK_EQ(bank.authenticate("1000001", "9999", detail), OperationResult::AccountClosed);
}

ATM_TEST(bank_deposit_updates_balance_and_history) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.deposit("1000001", Money::from_cents(500), detail), OperationResult::Success);
    CHECK_EQ(bank.find_account("1000001")->balance(), Money::from_cents(1000500));

    auto history = bank.history_for("1000001");
    REQUIRE(history.size() == 1);
    CHECK_EQ(history[0].type, TransactionType::Deposit);
    CHECK_EQ(history[0].amount, Money::from_cents(500));
    CHECK_EQ(history[0].balance_after, Money::from_cents(1000500));
    CHECK_EQ(history[0].account_number, std::string("1000001"));

    CHECK_EQ(bank.deposit("1000001", Money::from_cents(0), detail), OperationResult::InvalidAmount);
    CHECK_EQ(bank.deposit("1000001", Money::from_cents(-5), detail), OperationResult::InvalidAmount);
    CHECK_EQ(bank.deposit("9999999", Money::from_cents(100), detail), OperationResult::AccountNotFound);
}

ATM_TEST(bank_deposit_rejects_inactive_account) {
    Bank bank;
    bank.add_account(test::make_account("1000001", "1234", 1000, AccountStatus::Locked));
    std::string detail;

    CHECK_EQ(bank.deposit("1000001", Money::from_cents(100), detail), OperationResult::AccountLocked);
    CHECK_EQ(bank.find_account("1000001")->balance(), Money::from_cents(1000));
}

ATM_TEST(bank_withdraw_success) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.withdraw("1000001", "1234", Money::from_cents(2500), detail),
             OperationResult::Success);
    CHECK_EQ(bank.find_account("1000001")->balance(), Money::from_cents(997500));
    CHECK_EQ(bank.withdrawn_today("1000001"), Money::from_cents(2500));

    auto history = bank.history_for("1000001");
    REQUIRE(history.size() == 1);
    CHECK_EQ(history[0].type, TransactionType::Withdrawal);
    CHECK_EQ(history[0].amount, Money::from_cents(2500));
    CHECK_EQ(history[0].balance_after, Money::from_cents(997500));
}

ATM_TEST(bank_withdraw_rejects_invalid_requests) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.withdraw("1000001", "0000", Money::from_cents(100), detail),
             OperationResult::InvalidPin);
    CHECK_EQ(bank.withdraw("9999999", "1234", Money::from_cents(100), detail),
             OperationResult::AccountNotFound);
    CHECK_EQ(bank.withdraw("1000001", "1234", Money::from_cents(0), detail),
             OperationResult::InvalidAmount);
    CHECK_EQ(bank.withdraw("1000001", "1234", Money::from_cents(-1), detail),
             OperationResult::InvalidAmount);
    CHECK_EQ(bank.withdraw("1000001", "1234", Money::from_cents(2000000), detail),
             OperationResult::LimitExceeded);
    CHECK_EQ(bank.withdraw("1000002", "2345", Money::from_cents(6000), detail),
             OperationResult::InsufficientFunds);
    CHECK_EQ(bank.find_account("1000001")->balance(), Money::from_cents(1000000));
    CHECK_EQ(bank.find_account("1000002")->balance(), Money::from_cents(5000));
}

ATM_TEST(bank_withdraw_respects_daily_limit) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.withdraw("1000001", "1234", Money::from_cents(400000), detail),
             OperationResult::Success);
    CHECK_EQ(bank.withdraw("1000001", "1234", Money::from_cents(150000), detail),
             OperationResult::LimitExceeded);
    CHECK_EQ(bank.find_account("1000001")->balance(), Money::from_cents(600000));
    CHECK_EQ(bank.withdrawn_today("1000001"), Money::from_cents(400000));
}

ATM_TEST(bank_withdraw_daily_limit_reset) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.withdraw("1000001", "1234", Money::from_cents(400000), detail),
             OperationResult::Success);
    CHECK_EQ(bank.withdrawn_today("1000001"), Money::from_cents(400000));
    bank.reset_daily_limits();
    CHECK_EQ(bank.withdrawn_today("1000001"), Money::from_cents(0));
    CHECK_EQ(bank.withdraw("1000001", "1234", Money::from_cents(400000), detail),
             OperationResult::Success);
}

ATM_TEST(bank_transfer_moves_money_and_records_both_sides) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.transfer("1000001", "1234", "1000002", Money::from_cents(2500), detail),
             OperationResult::Success);
    CHECK_EQ(bank.find_account("1000001")->balance(), Money::from_cents(997500));
    CHECK_EQ(bank.find_account("1000002")->balance(), Money::from_cents(7500));

    auto source_history = bank.history_for("1000001");
    REQUIRE(source_history.size() == 1);
    CHECK_EQ(source_history[0].type, TransactionType::Withdrawal);

    auto target_history = bank.history_for("1000002");
    REQUIRE(target_history.size() == 1);
    CHECK_EQ(target_history[0].type, TransactionType::TransferIn);
    CHECK_EQ(target_history[0].amount, Money::from_cents(2500));
    CHECK_EQ(target_history[0].balance_after, Money::from_cents(7500));
}

ATM_TEST(bank_transfer_rejects_invalid_requests) {
    Bank bank = make_bank();
    bank.add_account(test::make_account("1000004", "4567", 0, AccountStatus::Closed));
    std::string detail;

    CHECK_EQ(bank.transfer("1000001", "1234", "1000001", Money::from_cents(100), detail),
             OperationResult::InvalidInput);
    CHECK_EQ(bank.transfer("1000001", "1234", "9999999", Money::from_cents(100), detail),
             OperationResult::AccountNotFound);
    CHECK_EQ(bank.transfer("9999999", "1234", "1000002", Money::from_cents(100), detail),
             OperationResult::AccountNotFound);
    CHECK_EQ(bank.transfer("1000001", "1234", "1000004", Money::from_cents(100), detail),
             OperationResult::AccountClosed);
    CHECK_EQ(bank.transfer("1000001", "1234", "1000002", Money::from_cents(0), detail),
             OperationResult::InvalidAmount);
    CHECK_EQ(bank.transfer("1000002", "2345", "1000001", Money::from_cents(6000), detail),
             OperationResult::InsufficientFunds);
}

ATM_TEST(bank_transfer_counts_toward_daily_limit) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.transfer("1000001", "1234", "1000002", Money::from_cents(400000), detail),
             OperationResult::Success);
    CHECK_EQ(bank.transfer("1000001", "1234", "1000003", Money::from_cents(150000), detail),
             OperationResult::LimitExceeded);
    CHECK_EQ(bank.withdrawn_today("1000001"), Money::from_cents(400000));
}

ATM_TEST(bank_balance_inquiry) {
    Bank bank = make_bank();
    std::string detail;
    Money result;

    CHECK_EQ(bank.balance("1000001", "1234", result, detail), OperationResult::Success);
    CHECK_EQ(result, Money::from_cents(1000000));
    auto history = bank.history_for("1000001");
    REQUIRE(history.size() == 1);
    CHECK_EQ(history[0].type, TransactionType::BalanceInquiry);
    CHECK_EQ(history[0].amount, Money());

    CHECK_EQ(bank.balance("1000001", "0000", result, detail), OperationResult::InvalidPin);
    CHECK_EQ(bank.balance("9999999", "1234", result, detail), OperationResult::AccountNotFound);
}

ATM_TEST(bank_change_pin) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.change_pin("1000001", "1234", "5678", detail), OperationResult::Success);
    CHECK_EQ(bank.authenticate("1000001", "5678", detail), OperationResult::Success);
    CHECK_EQ(bank.authenticate("1000001", "1234", detail), OperationResult::InvalidPin);
    bank.find_account("1000001")->reset_failed_attempts();

    CHECK_EQ(bank.change_pin("1000001", "5678", "5678", detail), OperationResult::InvalidInput);
    CHECK_EQ(bank.change_pin("1000001", "5678", "12", detail), OperationResult::InvalidPin);
    CHECK_EQ(bank.change_pin("1000001", "0000", "9999", detail), OperationResult::InvalidPin);
    CHECK_EQ(bank.change_pin("9999999", "1234", "9999", detail), OperationResult::AccountNotFound);
}

ATM_TEST(bank_change_phone) {
    Bank bank = make_bank();
    std::string detail;

    CHECK_EQ(bank.change_phone("1000001", "1234", "0000000123", detail), OperationResult::Success);
    CHECK_EQ(bank.find_account("1000001")->phone_number(), std::string("0000000123"));
    CHECK_EQ(bank.change_phone("1000001", "1234", "", detail), OperationResult::InvalidInput);
    CHECK_EQ(bank.change_phone("1000001", "0000", "0000000999", detail), OperationResult::InvalidPin);
    CHECK_EQ(bank.change_phone("9999999", "1234", "0000000999", detail), OperationResult::AccountNotFound);
}

ATM_TEST(bank_config_defaults_and_overrides) {
    Bank bank;
    CHECK_EQ(bank.config().daily_withdrawal_limit, Money::from_cents(500000));
    CHECK_EQ(bank.config().max_failed_attempts, 3);
    CHECK_EQ(bank.config().minimum_balance, Money());

    BankConfig config;
    config.daily_withdrawal_limit = Money::from_cents(100000);
    config.max_failed_attempts = 5;
    bank.set_config(config);
    CHECK_EQ(bank.config().daily_withdrawal_limit, Money::from_cents(100000));
    CHECK_EQ(bank.config().max_failed_attempts, 5);

    BankConfig zeroed;
    bank.set_config(zeroed);
    CHECK_EQ(bank.config().daily_withdrawal_limit, Money::from_cents(500000));
    CHECK_EQ(bank.config().max_failed_attempts, 3);
}

ATM_TEST(bank_save_and_load_round_trip) {
    const auto dir = test::fresh_temp_dir("bank_round_trip");
    const auto accounts_path = dir / "accounts.csv";
    const auto transactions_path = dir / "transactions.csv";

    Bank original = make_bank();
    std::string detail;
    REQUIRE(original.deposit("1000001", Money::from_cents(1234), detail) == OperationResult::Success);
    REQUIRE(original.transfer("1000001", "1234", "1000002", Money::from_cents(500), detail) ==
            OperationResult::Success);
    REQUIRE(original.save(accounts_path.string(), transactions_path.string(), detail));

    Bank restored;
    REQUIRE(restored.load(accounts_path.string(), transactions_path.string(), detail));
    CHECK_EQ(restored.account_count(), original.account_count());
    CHECK_EQ(restored.find_account("1000001")->balance(),
             original.find_account("1000001")->balance());
    CHECK_EQ(restored.find_account("1000002")->balance(),
             original.find_account("1000002")->balance());
    CHECK_EQ(restored.find_account("1000001")->phone_number(), std::string("0000000000"));
    CHECK_EQ(restored.history_for("1000001").size(),
             original.history_for("1000001").size());
    CHECK_EQ(restored.history_for("1000002").size(),
             original.history_for("1000002").size());
    CHECK_EQ(restored.authenticate("1000001", "1234", detail), OperationResult::Success);

    test::remove_dir(dir);
}

ATM_TEST(bank_load_allows_missing_transaction_file) {
    const auto dir = test::fresh_temp_dir("bank_missing_transactions");
    const auto accounts_path = dir / "accounts.csv";
    const auto transactions_path = dir / "transactions.csv";

    Bank original = make_bank();
    std::string detail;
    REQUIRE(original.save(accounts_path.string(), transactions_path.string(), detail));
    std::filesystem::remove(transactions_path);

    Bank restored;
    CHECK(restored.load(accounts_path.string(), transactions_path.string(), detail));
    CHECK_EQ(restored.account_count(), static_cast<std::size_t>(3));
    CHECK(restored.history_for("1000001").empty());

    test::remove_dir(dir);
}

ATM_TEST(bank_load_fails_on_missing_accounts_file) {
    const auto dir = test::fresh_temp_dir("bank_missing_accounts");
    Bank bank;
    std::string detail;
    CHECK(!bank.load((dir / "accounts.csv").string(), (dir / "transactions.csv").string(), detail));
    CHECK(!detail.empty());
    test::remove_dir(dir);
}

ATM_TEST(bank_load_fails_on_corrupt_transaction_file) {
    const auto dir = test::fresh_temp_dir("bank_corrupt_transactions");
    const auto accounts_path = dir / "accounts.csv";
    const auto transactions_path = dir / "transactions.csv";

    Bank original = make_bank();
    std::string detail;
    REQUIRE(original.save(accounts_path.string(), transactions_path.string(), detail));
    {
        std::ofstream output(transactions_path, std::ios::trunc);
        output << "id,account_number,type,amount_cents,balance_after_cents,timestamp,note\n";
        output << "TX1,1000001,not_a_type,100,0,2026-01-01T00:00:00,test\n";
    }

    Bank restored;
    CHECK(!restored.load(accounts_path.string(), transactions_path.string(), detail));
    CHECK(!detail.empty());

    test::remove_dir(dir);
}

ATM_TEST(bank_operation_result_names) {
    CHECK_EQ(to_string(OperationResult::Success), std::string("success"));
    CHECK_EQ(to_string(OperationResult::AccountNotFound), std::string("account_not_found"));
    CHECK_EQ(to_string(OperationResult::InvalidPin), std::string("invalid_pin"));
    CHECK_EQ(to_string(OperationResult::InvalidAmount), std::string("invalid_amount"));
    CHECK_EQ(to_string(OperationResult::InsufficientFunds), std::string("insufficient_funds"));
    CHECK_EQ(to_string(OperationResult::AccountLocked), std::string("account_locked"));
    CHECK_EQ(to_string(OperationResult::AccountClosed), std::string("account_closed"));
    CHECK_EQ(to_string(OperationResult::LimitExceeded), std::string("limit_exceeded"));
    CHECK_EQ(to_string(OperationResult::InvalidInput), std::string("invalid_input"));
    CHECK_EQ(to_string(OperationResult::StorageFailure), std::string("storage_failure"));
}
