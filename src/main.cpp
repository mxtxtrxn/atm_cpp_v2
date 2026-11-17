#include <iostream>
#include <string>

#include "atm/bank.hpp"
#include "atm/cli.hpp"
#include "atm/ui.hpp"

int main(int argc, char** argv) {
    atm::CliOptions options;
    std::string parse_error;
    if (!atm::parse_arguments(argc, argv, options, parse_error)) {
        std::cerr << parse_error << '\n';
        return 2;
    }

    atm::Bank bank;
    std::string load_error;
    if (!bank.load(options.accounts_path, options.transactions_path, load_error)) {
        if (!options.seed_demo_data) {
            std::cerr << "No se pudo cargar el estado: " << load_error << '\n';
            std::cerr << "Use --seed-demo para inicializar cuentas de demostracion.\n";
            return 1;
        }
        atm::seed_if_empty(bank);
        std::cerr << "[INFO] No se encontro estado previo; se crearon datos de demostracion.\n";
    } else if (options.seed_demo_data) {
        atm::seed_if_empty(bank);
    }

    atm::Console console(std::cin, std::cout);
    atm::Cli cli(bank, console, options);

    int exit_code = 0;
    if (options.non_interactive) {
        exit_code = cli.run_batch();
    } else {
        exit_code = cli.run_interactive();
    }

    std::string save_error;
    if (!bank.save(options.accounts_path, options.transactions_path, save_error)) {
        std::cerr << "No se pudo guardar el estado: " << save_error << '\n';
        return 1;
    }
    return exit_code;
}
