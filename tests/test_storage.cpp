#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "atm/storage.hpp"
#include "test_framework.hpp"
#include "test_utils.hpp"

using namespace atm;

ATM_TEST(storage_csv_escape_plain_value) {
    CHECK_EQ(csv_escape("simple"), std::string("simple"));
    CHECK_EQ(csv_escape(""), std::string(""));
    CHECK_EQ(csv_escape("1234"), std::string("1234"));
}

ATM_TEST(storage_csv_escape_special_characters) {
    CHECK_EQ(csv_escape("Demo, Uno"), std::string("\"Demo, Uno\""));
    CHECK_EQ(csv_escape("Di \"comillas\""), std::string("\"Di \"\"comillas\"\"\""));
    CHECK_EQ(csv_escape("linea\nnueva"), std::string("\"linea\nnueva\""));
}

ATM_TEST(storage_csv_split_round_trip) {
    const std::string line = "a,\"b,c\",\"d\"\"e\",f";
    const auto fields = csv_split(line);
    REQUIRE(fields.size() == 4);
    CHECK_EQ(fields[0], std::string("a"));
    CHECK_EQ(fields[1], std::string("b,c"));
    CHECK_EQ(fields[2], std::string("d\"e"));
    CHECK_EQ(fields[3], std::string("f"));
}

ATM_TEST(storage_csv_split_handles_crlf_and_empty_fields) {
    auto fields = csv_split("uno,dos\r");
    REQUIRE(fields.size() == 2);
    CHECK_EQ(fields[0], std::string("uno"));
    CHECK_EQ(fields[1], std::string("dos"));
    fields = csv_split(",,");
    REQUIRE(fields.size() == 3);
    CHECK_EQ(fields[0], std::string());
    CHECK_EQ(fields[1], std::string());
    CHECK_EQ(fields[2], std::string());
}

ATM_TEST(storage_save_and_load_accounts) {
    const auto dir = test::fresh_temp_dir("storage_accounts");
    const auto path = dir / "accounts.csv";

    std::vector<Account> accounts;
    Account first("1000001", "Demo, Uno", hash_pin("1234"), Money::from_cents(12345),
                  AccountStatus::Active);
    first.set_failed_attempts(2);
    first.set_phone_number("0000000001");
    accounts.push_back(first);

    Account second("1000002", "Demo \"Dos\"", hash_pin("2345"), Money::from_cents(0),
                   AccountStatus::Locked);
    second.set_phone_number("0000000002");
    accounts.push_back(second);

    std::string error;
    REQUIRE(save_accounts(path.string(), accounts).ok);

    std::vector<Account> restored;
    const StorageError result = load_accounts(path.string(), restored);
    REQUIRE(result.ok);
    REQUIRE(restored.size() == 2);

    CHECK_EQ(restored[0].account_number(), std::string("1000001"));
    CHECK_EQ(restored[0].holder_name(), std::string("Demo, Uno"));
    CHECK_EQ(restored[0].pin_hash(), hash_pin("1234"));
    CHECK_EQ(restored[0].balance(), Money::from_cents(12345));
    CHECK_EQ(restored[0].status(), AccountStatus::Active);
    CHECK_EQ(restored[0].failed_attempts(), 2);
    CHECK_EQ(restored[0].phone_number(), std::string("0000000001"));

    CHECK_EQ(restored[1].account_number(), std::string("1000002"));
    CHECK_EQ(restored[1].holder_name(), std::string("Demo \"Dos\""));
    CHECK_EQ(restored[1].status(), AccountStatus::Locked);

    test::remove_dir(dir);
}

ATM_TEST(storage_save_and_load_transactions) {
    const auto dir = test::fresh_temp_dir("storage_transactions");
    const auto path = dir / "transactions.csv";

    std::vector<Transaction> transactions;
    Transaction first;
    first.id = "TX000001";
    first.account_number = "1000001";
    first.type = TransactionType::Deposit;
    first.amount = Money::from_cents(5000);
    first.balance_after = Money::from_cents(15000);
    first.timestamp = "2026-01-01T10:00:00";
    first.note = "Deposito, inicial";
    transactions.push_back(first);

    Transaction second;
    second.id = "TX000002";
    second.account_number = "1000002";
    second.type = TransactionType::TransferOut;
    second.amount = Money::from_cents(2500);
    second.balance_after = Money::from_cents(7500);
    second.timestamp = "2026-01-01T10:05:00";
    second.note = "Transferencia enviada";
    transactions.push_back(second);

    REQUIRE(save_transactions(path.string(), transactions).ok);

    std::vector<Transaction> restored;
    const StorageError result = load_transactions(path.string(), restored);
    REQUIRE(result.ok);
    REQUIRE(restored.size() == 2);
    CHECK_EQ(restored[0].id, std::string("TX000001"));
    CHECK_EQ(restored[0].type, TransactionType::Deposit);
    CHECK_EQ(restored[0].amount, Money::from_cents(5000));
    CHECK_EQ(restored[0].balance_after, Money::from_cents(15000));
    CHECK_EQ(restored[0].timestamp, std::string("2026-01-01T10:00:00"));
    CHECK_EQ(restored[0].note, std::string("Deposito, inicial"));
    CHECK_EQ(restored[1].type, TransactionType::TransferOut);

    test::remove_dir(dir);
}

ATM_TEST(storage_load_missing_files_fail) {
    const auto dir = test::fresh_temp_dir("storage_missing");
    std::vector<Account> accounts;
    std::vector<Transaction> transactions;

    const StorageError account_result = load_accounts((dir / "nope.csv").string(), accounts);
    CHECK(!account_result.ok);
    CHECK(!account_result.message.empty());

    const StorageError transaction_result =
        load_transactions((dir / "nope.csv").string(), transactions);
    CHECK(!transaction_result.ok);
    CHECK(!transaction_result.message.empty());

    test::remove_dir(dir);
}

ATM_TEST(storage_load_empty_and_header_only_files) {
    const auto dir = test::fresh_temp_dir("storage_empty");
    const auto accounts_path = dir / "accounts.csv";
    const auto transactions_path = dir / "transactions.csv";

    {
        std::ofstream output(accounts_path);
        output << "account_number,holder_name,pin_hash,balance_cents,status,failed_attempts,phone_number\n";
    }
    {
        std::ofstream output(transactions_path);
        output << "id,account_number,type,amount_cents,balance_after_cents,timestamp,note\n";
    }

    std::vector<Account> accounts;
    std::vector<Transaction> transactions;
    CHECK(load_accounts(accounts_path.string(), accounts).ok);
    CHECK(accounts.empty());
    CHECK(load_transactions(transactions_path.string(), transactions).ok);
    CHECK(transactions.empty());

    test::remove_dir(dir);
}

ATM_TEST(storage_rejects_malformed_account_rows) {
    const auto dir = test::fresh_temp_dir("storage_bad_accounts");
    const auto path = dir / "accounts.csv";
    std::vector<Account> accounts;

    {
        std::ofstream output(path);
        output << "account_number,holder_name,pin_hash,balance_cents,status,failed_attempts,phone_number\n";
        output << "1000001,Demo,hash,1000,active,0\n";
    }
    CHECK(!load_accounts(path.string(), accounts).ok);

    {
        std::ofstream output(path, std::ios::trunc);
        output << "account_number,holder_name,pin_hash,balance_cents,status,failed_attempts,phone_number\n";
        output << "1000001,Demo,hash,no_es_numero,active,0,0000000001\n";
    }
    CHECK(!load_accounts(path.string(), accounts).ok);

    {
        std::ofstream output(path, std::ios::trunc);
        output << "account_number,holder_name,pin_hash,balance_cents,status,failed_attempts,phone_number\n";
        output << "1000001,Demo,hash,1000,frozen,0,0000000001\n";
    }
    CHECK(!load_accounts(path.string(), accounts).ok);

    test::remove_dir(dir);
}

ATM_TEST(storage_rejects_malformed_transaction_rows) {
    const auto dir = test::fresh_temp_dir("storage_bad_transactions");
    const auto path = dir / "transactions.csv";
    std::vector<Transaction> transactions;

    {
        std::ofstream output(path);
        output << "id,account_number,type,amount_cents,balance_after_cents,timestamp,note\n";
        output << "TX1,1000001,deposit,100\n";
    }
    CHECK(!load_transactions(path.string(), transactions).ok);

    {
        std::ofstream output(path, std::ios::trunc);
        output << "id,account_number,type,amount_cents,balance_after_cents,timestamp,note\n";
        output << "TX1,1000001,desconocido,100,0,2026-01-01T00:00:00,nota\n";
    }
    CHECK(!load_transactions(path.string(), transactions).ok);

    test::remove_dir(dir);
}

ATM_TEST(storage_save_creates_parent_directories) {
    const auto dir = test::fresh_temp_dir("storage_nested");
    const auto path = dir / "nivel1" / "nivel2" / "accounts.csv";
    CHECK(ensure_parent_directory(path.string()));
    CHECK(std::filesystem::exists(path.parent_path()));

    std::vector<Account> accounts;
    accounts.push_back(test::make_account("1000001", "1234", 100));
    CHECK(save_accounts(path.string(), accounts).ok);
    CHECK(std::filesystem::exists(path));

    test::remove_dir(dir);
}

ATM_TEST(storage_save_overwrites_previous_content) {
    const auto dir = test::fresh_temp_dir("storage_overwrite");
    const auto path = dir / "accounts.csv";

    std::vector<Account> first;
    first.push_back(test::make_account("1000001", "1234", 100));
    REQUIRE(save_accounts(path.string(), first).ok);

    std::vector<Account> second;
    second.push_back(test::make_account("1000002", "2345", 200));
    REQUIRE(save_accounts(path.string(), second).ok);

    std::vector<Account> restored;
    REQUIRE(load_accounts(path.string(), restored).ok);
    REQUIRE(restored.size() == 1);
    CHECK_EQ(restored[0].account_number(), std::string("1000002"));
    CHECK_EQ(restored[0].balance(), Money::from_cents(200));

    test::remove_dir(dir);
}
