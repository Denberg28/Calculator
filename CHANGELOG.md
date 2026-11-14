# Changelog

## 0.1.0-rc.1 — 2026-11-14

Initial Windows x64 test candidate.

- Native offline calculator with arithmetic, decimals, unary signs, parentheses and precedence.
- Mouse/keyboard input, Backspace, Clear, result chaining and copy result.
- Bounded input, helpful errors, correction without losing the expression.
- Resizable native UI, per-monitor DPI manifest and Windows high-contrast colors.
- Independent arithmetic engine, regression tests, Windows UI smoke tests and gated portable packaging.

Compatibility: Windows 10/11 x64. No application runtime installer; no saved data or migration. Unsigned. Desktop validation remains required before stable; see docs/TESTING.md.

Rollback: close the app and reopen a retained previous version. This is the first release; there is no previous stable executable.
