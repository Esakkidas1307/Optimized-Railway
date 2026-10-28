TRAIN SCHEDULE LEAD-TIME RULE

Regular/running train: schedule at least one calendar month ahead.
Special train: schedule at least seven calendar days ahead.

Examples for today's date 26:10:2026:
- Regular train date 26:11:2026: accepted.
- Regular train date 25:11:2026: rejected.
- Special train date 02:11:2026: accepted.
- Special train date 01:11:2026: rejected.
- Any past date: rejected.

After three invalid train-number, schedule-type, or journey-date attempts,
Add Schedule abandons the operation and returns to its caller (the admin menu).
The schedule is saved and availability initialized only after validation passes.

Note: this is a MISRA-oriented baseline, not proof of full MISRA C compliance.
A MISRA-capable static analyzer and documented rule-by-rule review are still required.
