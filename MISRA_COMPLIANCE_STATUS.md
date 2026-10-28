# MISRA C review status

This project is a MISRA-oriented review baseline, not a certified or fully verified MISRA C implementation.

## Scheduling lead-time rule added

- Regular/running train schedules: the journey date must be at least one calendar month after the current local date.
- Special train schedules: the journey date must be at least seven calendar days after the current local date.
- The current local timezone configured by `date_time.c` is `Asia/Kolkata`.
- Month-end dates are clamped to the final day of the target month (for example, January 31 plus one calendar month becomes February 28 or 29).
- A past date, an invalid date, or a date earlier than the minimum lead time is rejected. The schedule entry operation allows three date attempts and then abandons that operation.

## Example

If today's date is 26:10:2026:

- A regular train scheduled for 26:11:2026 is eligible (exactly one calendar month ahead).
- A regular train scheduled for 25:11:2026 is not eligible.
- A special train scheduled for 02:11:2026 is eligible (seven days ahead).
- A special train scheduled for 01:11:2026 is not eligible.
- A schedule date in February 2026 is in the past and is rejected.

## Compliance disclaimer

Enabling GCC warnings does not establish MISRA compliance. Full compliance requires selecting the project's applicable MISRA C edition, running a MISRA-capable static analyzer, reviewing all findings, fixing violations, documenting justified deviations, and retaining evidence of the review. The current build still reports warnings for numeric conversions and unused functions. Do not describe this ZIP as fully MISRA-compliant or certified.
