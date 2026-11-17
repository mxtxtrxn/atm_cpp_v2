#include "atm/cli.hpp"

#include <cctype>
#include <utility>

#include "atm/auth.hpp"

namespace atm {

namespace {

std::string trim(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

bool parse_money_argument(const std::string& value, Money& out, std::string& error) {
    try {
        out = Money::parse(value);
    } catch (const std::exception&) {
        error = "Monto invalido: " + value;
        return false;
    }
    if (!out.is_positive()) {
        error = "El monto debe ser mayor a cero";
        return false;
    }
    return true;
}

}  // namespace

Cli::Cli(Bank& bank, Console& console, CliOptions options)
    : bank_(bank), console_(console), options_(std::move(options)), session_account_() {}

int Cli::run_interactive() {
    console_.print_header("Cajero automatico - Demo");
    while (true) {
        console_.print_line("1. Iniciar sesion");
        console_.print_line("2. Salir");
        bool ok = false;
        int option = console_.read_int("Seleccione una opcion: ", ok);
        if (!ok) {
            console_.print_error("Opcion invalida");
            continue;
        }
        if (option == 2) {
            console_.print_line("Hasta luego.");
            return 0;
        }
        if (option != 1) {
            console_.print_error("Opcion invalida");
            continue;
        }
        if (!prompt_credentials()) {
            continue;
        }
        console_.print_success("Sesion iniciada");
        while (true) {
            console_.print_header("Menu principal");
            console_.print_line("1. Consultar saldo");
            console_.print_line("2. Retirar dinero");
            console_.print_line("3. Depositar");
            console_.print_line("4. Transferir");
            console_.print_line("5. Historial");
            console_.print_line("6. Datos del perfil");
            console_.print_line("7. Cambiar PIN");
            console_.print_line("8. Cambiar telefono");
            console_.print_line("9. Cerrar sesion");
            bool menu_ok = false;
            int choice = console_.read_int("Seleccione una opcion: ", menu_ok);
            if (!menu_ok) {
                console_.print_error("Opcion invalida");
                continue;
            }
            switch (choice) {
                case 1:
                    handle_balance();
                    break;
                case 2:
                    handle_withdraw();
                    break;
                case 3:
                    handle_deposit();
                    break;
                case 4:
                    handle_transfer();
                    break;
                case 5:
                    handle_history();
                    break;
                case 6:
                    handle_profile();
                    break;
                case 7:
                    handle_change_pin();
                    break;
                case 8:
                    handle_change_phone();
                    break;
                case 9:
                    handle_logout();
                    break;
                default:
                    console_.print_error("Opcion invalida");
                    break;
            }
            if (session_account_.empty()) {
                break;
            }
        }
    }
}

int Cli::run_batch() {
    if (options_.command == "balance") {
        if (options_.arguments.size() < 2) {
            console_.print_error("Uso: --batch balance <cuenta> <pin>");
            return 2;
        }
        const std::string& account = options_.arguments[0];
        const std::string& pin = options_.arguments[1];
        Money balance;
        std::string detail;
        OperationResult result = bank_.balance(account, pin, balance, detail);
        if (result != OperationResult::Success) {
            console_.print_error(detail);
            return 1;
        }
        console_.print_money("Saldo", balance);
        return 0;
    }

    if (options_.command == "withdraw") {
        if (options_.arguments.size() < 3) {
            console_.print_error("Uso: --batch withdraw <cuenta> <pin> <monto>");
            return 2;
        }
        Money amount;
        std::string error;
        if (!parse_money_argument(options_.arguments[2], amount, error)) {
            console_.print_error(error);
            return 2;
        }
        std::string detail;
        OperationResult result = bank_.withdraw(options_.arguments[0], options_.arguments[1], amount, detail);
        if (result != OperationResult::Success) {
            console_.print_error(detail);
            return 1;
        }
        console_.print_success("Retiro realizado");
        return 0;
    }

    if (options_.command == "deposit") {
        if (options_.arguments.size() < 2) {
            console_.print_error("Uso: --batch deposit <cuenta> <monto>");
            return 2;
        }
        Money amount;
        std::string error;
        if (!parse_money_argument(options_.arguments[1], amount, error)) {
            console_.print_error(error);
            return 2;
        }
        std::string detail;
        OperationResult result = bank_.deposit(options_.arguments[0], amount, detail);
        if (result != OperationResult::Success) {
            console_.print_error(detail);
            return 1;
        }
        console_.print_success("Deposito realizado");
        return 0;
    }

    if (options_.command == "transfer") {
        if (options_.arguments.size() < 4) {
            console_.print_error("Uso: --batch transfer <origen> <pin> <destino> <monto>");
            return 2;
        }
        Money amount;
        std::string error;
        if (!parse_money_argument(options_.arguments[3], amount, error)) {
            console_.print_error(error);
            return 2;
        }
        std::string detail;
        OperationResult result = bank_.transfer(options_.arguments[0], options_.arguments[1],
                                                options_.arguments[2], amount, detail);
        if (result != OperationResult::Success) {
            console_.print_error(detail);
            return 1;
        }
        console_.print_success("Transferencia realizada");
        return 0;
    }

    if (options_.command == "history") {
        if (options_.arguments.size() < 1) {
            console_.print_error("Uso: --batch history <cuenta>");
            return 2;
        }
        auto entries = bank_.history_for(options_.arguments[0]);
        if (entries.empty()) {
            console_.print_line("Sin movimientos.");
            return 0;
        }
        for (const auto& tx : entries) {
            console_.print_transaction(tx);
        }
        return 0;
    }

    console_.print_error("Comando desconocido: " + options_.command);
    return 2;
}

void Cli::handle_balance() {
    if (!require_session()) {
        return;
    }
    const Account* account = bank_.find_account(session_account_);
    if (account == nullptr) {
        console_.print_error("Cuenta no encontrada");
        session_account_.clear();
        return;
    }
    console_.print_money("Saldo disponible", account->balance());
    console_.pause();
}

void Cli::handle_withdraw() {
    if (!require_session()) {
        return;
    }
    bool ok = false;
    double amount_units = console_.read_double("Monto a retirar: ", ok);
    if (!ok || amount_units <= 0) {
        console_.print_error("Monto invalido");
        return;
    }
    std::string pin = console_.read_password("Confirme su PIN: ");
    std::string detail;
    OperationResult result = bank_.withdraw(session_account_, pin, Money::from_units(amount_units), detail);
    if (result != OperationResult::Success) {
        console_.print_error(detail);
        return;
    }
    console_.print_success("Retiro realizado");
    const Account* account = bank_.find_account(session_account_);
    if (account != nullptr) {
        console_.print_money("Nuevo saldo", account->balance());
    }
    console_.pause();
}

void Cli::handle_deposit() {
    if (!require_session()) {
        return;
    }
    bool ok = false;
    double amount_units = console_.read_double("Monto a depositar: ", ok);
    if (!ok || amount_units <= 0) {
        console_.print_error("Monto invalido");
        return;
    }
    std::string detail;
    OperationResult result = bank_.deposit(session_account_, Money::from_units(amount_units), detail);
    if (result != OperationResult::Success) {
        console_.print_error(detail);
        return;
    }
    console_.print_success("Deposito realizado");
    console_.pause();
}

void Cli::handle_transfer() {
    if (!require_session()) {
        return;
    }
    std::string target = trim(console_.read_line("Cuenta destino: "));
    bool ok = false;
    double amount_units = console_.read_double("Monto a transferir: ", ok);
    if (!ok || amount_units <= 0) {
        console_.print_error("Monto invalido");
        return;
    }
    std::string pin = console_.read_password("Confirme su PIN: ");
    std::string detail;
    OperationResult result = bank_.transfer(session_account_, pin, target, Money::from_units(amount_units), detail);
    if (result != OperationResult::Success) {
        console_.print_error(detail);
        return;
    }
    console_.print_success("Transferencia realizada");
    console_.pause();
}

void Cli::handle_history() {
    if (!require_session()) {
        return;
    }
    auto entries = bank_.history_for(session_account_);
    if (entries.empty()) {
        console_.print_line("Sin movimientos registrados.");
        console_.pause();
        return;
    }
    console_.print_header("Historial de movimientos");
    for (const auto& tx : entries) {
        console_.print_transaction(tx);
    }
    console_.pause();
}

void Cli::handle_profile() {
    if (!require_session()) {
        return;
    }
    const Account* account = bank_.find_account(session_account_);
    if (account == nullptr) {
        console_.print_error("Cuenta no encontrada");
        return;
    }
    console_.print_header("Datos del perfil");
    console_.print_account_summary(*account);
    console_.pause();
}

void Cli::handle_change_pin() {
    if (!require_session()) {
        return;
    }
    std::string old_pin = console_.read_password("PIN actual: ");
    std::string new_pin = console_.read_password("PIN nuevo: ");
    std::string detail;
    OperationResult result = bank_.change_pin(session_account_, old_pin, new_pin, detail);
    if (result != OperationResult::Success) {
        console_.print_error(detail);
        return;
    }
    console_.print_success("PIN actualizado");
    console_.pause();
}

void Cli::handle_change_phone() {
    if (!require_session()) {
        return;
    }
    std::string pin = console_.read_password("Confirme su PIN: ");
    std::string phone = trim(console_.read_line("Nuevo telefono: "));
    std::string detail;
    OperationResult result = bank_.change_phone(session_account_, pin, phone, detail);
    if (result != OperationResult::Success) {
        console_.print_error(detail);
        return;
    }
    console_.print_success("Telefono actualizado");
    console_.pause();
}

void Cli::handle_logout() {
    session_account_.clear();
    console_.print_line("Sesion cerrada.");
}

bool Cli::require_session() {
    if (session_account_.empty()) {
        console_.print_error("Debe iniciar sesion primero");
        return false;
    }
    return true;
}

bool Cli::prompt_credentials() {
    std::string account = trim(console_.read_line("Numero de cuenta: "));
    std::string pin = console_.read_password("PIN: ");
    std::string detail;
    OperationResult result = bank_.authenticate(account, pin, detail);
    if (result != OperationResult::Success) {
        console_.print_error(detail);
        return false;
    }
    session_account_ = account;
    return true;
}

bool parse_arguments(int argc, char** argv, CliOptions& options, std::string& error) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--accounts") {
            if (i + 1 >= argc) {
                error = "Falta el valor para --accounts";
                return false;
            }
            options.accounts_path = argv[++i];
        } else if (arg == "--transactions") {
            if (i + 1 >= argc) {
                error = "Falta el valor para --transactions";
                return false;
            }
            options.transactions_path = argv[++i];
        } else if (arg == "--seed-demo") {
            options.seed_demo_data = true;
        } else if (arg == "--batch") {
            if (i + 1 >= argc) {
                error = "Falta el comando para --batch";
                return false;
            }
            options.non_interactive = true;
            options.command = argv[++i];
            while (i + 1 < argc) {
                options.arguments.emplace_back(argv[++i]);
            }
        } else if (arg == "--help" || arg == "-h") {
            error = "Uso: atm [--accounts ruta] [--transactions ruta] [--seed-demo] [--batch comando args...]";
            return false;
        } else {
            error = "Argumento desconocido: " + arg;
            return false;
        }
    }
    return true;
}

std::vector<Account> make_demo_accounts() {
    std::vector<Account> accounts;
    accounts.emplace_back("1000001", "Usuario Demo Uno", hash_pin("1234"), Money::from_units(15000.0),
                          AccountStatus::Active);
    accounts.back().set_phone_number("0000000001");
    accounts.emplace_back("1000002", "Usuario Demo Dos", hash_pin("2345"), Money::from_units(8500.0),
                          AccountStatus::Active);
    accounts.back().set_phone_number("0000000002");
    accounts.emplace_back("1000003", "Usuario Demo Tres", hash_pin("3456"), Money::from_units(3200.0),
                          AccountStatus::Active);
    accounts.back().set_phone_number("0000000003");
    return accounts;
}

void seed_if_empty(Bank& bank) {
    if (bank.account_count() > 0) {
        return;
    }
    for (const auto& account : make_demo_accounts()) {
        bank.add_account(account);
    }
}

}  // namespace atm
