# Cajero automático (C++17)

Aplicación de cajero automático por consola, con cuentas, autenticación por PIN,
operaciones bancarias, historial de movimientos y persistencia en archivos CSV.
El proyecto está pensado como una base portable y verificable: no depende de
librerías específicas de Windows ni de bibliotecas externas.

## Funcionalidades

- Inicio de sesión con número de cuenta y PIN de cuatro dígitos.
- Bloqueo automático de la cuenta tras varios intentos fallidos.
- Consulta de saldo, depósitos, retiros y transferencias entre cuentas.
- Límite diario de retiro (también aplicado a transferencias).
- Historial de movimientos por cuenta.
- Cambio de PIN y de número de teléfono.
- Persistencia en CSV con carga y guardado al iniciar y finalizar.
- Modo interactivo y modo por lotes para automatizar operaciones.
- Cuentas de demostración con datos completamente ficticios.

## Requisitos

- Compilador con soporte de C++17 (GCC, Clang o MSVC).
- CMake 3.16 o superior (recomendado), o `make` en entornos tipo Unix/MSYS2.

## Compilación con CMake

```sh
cmake -S . -B build
cmake --build build --config Release
```

En Windows con generador de Visual Studio, el ejecutable queda en
`build/Release/atm.exe`. Con generadores de un solo tipo de configuración
(Ninja, Makefiles) queda en `build/atm` o `build/atm.exe`.

## Compilación con Make

```sh
make
make test
```

Los objetos y binarios se generan dentro de `build/`.

## Uso

Modo interactivo:

```sh
./build/atm
```

Primera ejecución con datos de demostración:

```sh
./build/atm --seed-demo
```

Opciones disponibles:

```text
--accounts <ruta>       Archivo CSV de cuentas (por defecto data/accounts.csv)
--transactions <ruta>   Archivo CSV de movimientos (por defecto data/transactions.csv)
--seed-demo             Crea las cuentas de demostración si el banco está vacío
--batch <comando> ...   Ejecuta una operación y termina
--help, -h              Muestra la ayuda
```

### Modo por lotes

```sh
./build/atm --batch balance 1000001 1234
./build/atm --batch deposit 1000001 250.00
./build/atm --batch withdraw 1000001 1234 100.00
./build/atm --batch transfer 1000001 1234 1000002 50.00
./build/atm --batch history 1000001
```

Los montos aceptan enteros o decimales (`100`, `100.50`, `$1,250.00`).
La sesión por lotes no requiere autenticación previa: cada comando recibe las
credenciales que necesita.

### Cuentas de demostración

| Cuenta  | PIN  | Saldo inicial | Teléfono   |
|---------|------|---------------|------------|
| 1000001 | 1234 | 15000.00      | 0000000001 |
| 1000002 | 2345 | 8500.00       | 0000000002 |
| 1000003 | 3456 | 3200.00       | 0000000003 |

Los nombres, cuentas, PIN y teléfonos son ficticios y existen únicamente
para probar la aplicación. No deben usarse PIN reales.

## Estructura

```text
include/atm/   Interfaces públicas (dinero, cuenta, transacción, banco, etc.)
src/           Implementación de la lógica, persistencia y consola
tests/         Pruebas unitarias y de integración con un arnés propio
data/          Archivos CSV de datos de demostración
CMakeLists.txt Configuración de compilación y pruebas con CTest
Makefile       Compilación alternativa para entornos tipo Unix/MSYS2
```

## Pruebas

```sh
ctest --test-dir build --output-on-failure
```

o, con Make:

```sh
make test
```

El arnés de pruebas no requiere dependencias externas. Cubre aritmética de
dinero, cuentas, autenticación, operaciones bancarias, persistencia CSV y el
flujo de la interfaz de consola.

## Formato de datos

`data/accounts.csv`:

```text
account_number,holder_name,pin_hash,balance_cents,status,failed_attempts,phone_number
```

`data/transactions.csv`:

```text
id,account_number,type,amount_cents,balance_after_cents,timestamp,note
```

Los montos se guardan en centavos para evitar errores de redondeo. Los campos
con comas o comillas se escapan siguiendo las reglas habituales de CSV.

## Notas de diseño

- El dinero se representa con enteros de 64 bits en centavos.
- El límite diario de retiro se mantiene en memoria y se reinicia al reiniciar
  la aplicación.
- El PIN se almacena como un hash estable, no reversible, implementado sin
  dependencias externas. No sustituye a un algoritmo criptográfico de producción.
- La interfaz es portable: no usa `conio.h`, `system("cls")` ni APIs exclusivas
  de Windows.
