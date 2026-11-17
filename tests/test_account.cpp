#include "test_framework.hpp"

using namespace atm;

ATM_TEST(account_default_state) {
    Account account;
    CHECK_EQ(account.account_number(), std::string());
    CHECK_EQ(account.holder_name(), std::string());
    CHECK_EQ(account.balance(), Money());
    CHECK_EQ(account.status(), AccountStatus::Active);
    CHECK_EQ(account.failed_attempts(), 0);
    CHECK(account.is_active());
}

ATM_TEST(account_constructor_sets_fields) {
    Account account("1000001", "Usuario Demo", "hash", Money::from_cents(2500),
                    AccountStatus::Active);
    CHECK_EQ(account.account_number(), std::string("1000001"));
    CHECK_EQ(account.holder_name(), std::string("Usuario Demo"));
    CHECK_EQ(account.pin_hash(), std::string("hash"));
    CHECK_EQ(account.balance(), Money::from_cents(2500));
    CHECK(account.is_active());
}

ATM_TEST(account_mutation_of_profile_fields) {
    Account account;
    account.set_holder_name("Titular de Prueba");
    account.set_phone_number("0000000009");
    account.set_status(AccountStatus::Locked);
    account.set_balance(Money::from_cents(777));
    account.set_pin_hash("abc123");

    CHECK_EQ(account.holder_name(), std::string("Titular de Prueba"));
    CHECK_EQ(account.phone_number(), std::string("0000000009"));
    CHECK_EQ(account.status(), AccountStatus::Locked);
    CHECK_EQ(account.balance(), Money::from_cents(777));
    CHECK_EQ(account.pin_hash(), std::string("abc123"));
    CHECK(!account.is_active());
}

ATM_TEST(account_credit_and_debit) {
    Account account("1000001", "Usuario Demo", "hash", Money::from_cents(1000),
                    AccountStatus::Active);
    account.credit(Money::from_cents(250));
    CHECK_EQ(account.balance(), Money::from_cents(1250));
    account.debit(Money::from_cents(500));
    CHECK_EQ(account.balance(), Money::from_cents(750));
}

ATM_TEST(account_failed_attempts) {
    Account account;
    CHECK_EQ(account.failed_attempts(), 0);
    account.register_failed_attempt();
    account.register_failed_attempt();
    CHECK_EQ(account.failed_attempts(), 2);
    account.reset_failed_attempts();
    CHECK_EQ(account.failed_attempts(), 0);
    account.set_failed_attempts(-4);
    CHECK_EQ(account.failed_attempts(), 0);
    account.set_failed_attempts(3);
    CHECK_EQ(account.failed_attempts(), 3);
}

ATM_TEST(account_can_withdraw_rules) {
    Account active("1000001", "Demo", "hash", Money::from_cents(1000), AccountStatus::Active);
    CHECK(active.can_withdraw(Money::from_cents(1000)));
    CHECK(active.can_withdraw(Money::from_cents(1)));
    CHECK(!active.can_withdraw(Money::from_cents(1001)));
    CHECK(!active.can_withdraw(Money::from_cents(0)));
    CHECK(!active.can_withdraw(Money::from_cents(-10)));

    Account locked("1000002", "Demo", "hash", Money::from_cents(1000), AccountStatus::Locked);
    CHECK(!locked.can_withdraw(Money::from_cents(10)));

    Account closed("1000003", "Demo", "hash", Money::from_cents(1000), AccountStatus::Closed);
    CHECK(!closed.can_withdraw(Money::from_cents(10)));
}

ATM_TEST(account_status_serialization) {
    CHECK_EQ(to_string(AccountStatus::Active), std::string("active"));
    CHECK_EQ(to_string(AccountStatus::Locked), std::string("locked"));
    CHECK_EQ(to_string(AccountStatus::Closed), std::string("closed"));

    AccountStatus parsed = AccountStatus::Closed;
    CHECK(parse_account_status("Active", parsed));
    CHECK_EQ(parsed, AccountStatus::Active);
    CHECK(parse_account_status("LOCKED", parsed));
    CHECK_EQ(parsed, AccountStatus::Locked);
    CHECK(parse_account_status("closed", parsed));
    CHECK_EQ(parsed, AccountStatus::Closed);
    CHECK(!parse_account_status("frozen", parsed));
    CHECK(!parse_account_status("", parsed));
}
