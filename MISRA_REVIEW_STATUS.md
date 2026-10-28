# MISRA C review status

## Important status

This package is a **MISRA-oriented review baseline**, not a certified or fully verified MISRA C implementation. The current program behavior and module interfaces have been preserved; no claim of full compliance is made without running a MISRA-capable static analyzer against a selected edition and reviewing every finding.

## Target to agree before claiming compliance

Select the project's required standard edition (for example MISRA C:2012 including applicable amendments, or MISRA C:2023), required rule set, compiler/toolchain, and deviation policy. The applicable rule set depends on the intended environment and project requirements.

## Build with stronger compiler diagnostics

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 *.c -o railway -lm
```

Compiler warnings are useful but **do not establish MISRA compliance**. Some warnings may be emitted by the existing code and should be reviewed rather than blindly suppressed.

## Areas requiring a source-level/static-analysis review

- Dynamic allocation, allocation failure handling, pointer lifetime and linked-list cleanup.
- Bounded strings and safe input parsing; avoid unchecked `strcpy`, `sprintf`, `atoi`, and `atof` use.
- Numeric conversions, signed/unsigned conversions, overflow, floating-point comparisons and division.
- File open/read/write/close error handling and persistence consistency.
- `setjmp`/`longjmp` navigation flow and its interaction with automatic objects and resource cleanup.
- Function prototypes, unused code, control-flow complexity, and return-value handling.
- Date/time library behavior and platform-specific timezone configuration.
- Transactional booking, payment, cancellation, refund, and inventory rollback paths.

## Validation checklist

1. Compile with the project's target compiler and strict diagnostics.
2. Run a MISRA-capable static analyzer for the selected MISRA edition.
3. Review each finding; correct it or record an approved, justified deviation.
4. Execute regression tests for menus, retry limits, timestamps/logs, booking, payment failure, cancellation/refund failure, inventory persistence, and RAC/WL reallocation.
5. Re-run analysis and tests after each change and retain reports.

Do not describe this project as MISRA compliant until the static-analysis findings and deviations have been reviewed and the required verification evidence is available.
