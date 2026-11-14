# Scope and baseline

Repository: `https://github.com/Denberg28/Calculator`, public, default branch `main`.
Baseline inspected 2026-11-14: empty repository, no commits, files, releases, package identity, signing key, or known-good executable. No existing work was replaced. Starting rules copied from `Denberg28/Agents_md/AGENTS.md`, blob `a00e681e755ce38fe9ac0129efc43139536865fc`, then project settings appended.

## MVP and assumptions

Portable Windows 10/11 x64 desktop calculator. Local mouse and keyboard input; basic arithmetic, negatives, decimals, parentheses, editing, result chaining, copying results, and clear error recovery. Application identity: `Denberg28.Calculator`; filename `Calculator.exe`. No administrator rights or installation. HDD/SSD stores the executable; session data stays in memory. Native C++17/Win32 avoids a web runtime or bundled language interpreter. No paid services or external runtime dependencies.

Non-goals: scientific functions, currency, arbitrary precision, history/database, cloud sync, accounts, telemetry, installer, ARM64/native x86 packages, or physical equipment control.

## User flow and recovery

Launch → enter expression → calculate → read/copy result → continue with operator or begin a new expression. The full expression remains visible and horizontally scrolls when long. Invalid syntax, incomplete parentheses, division by zero, and numeric overflow show a specific message and preserve input. Backspace repairs input; Clear resets to zero. Closing ends the session; relaunch starts empty. Failed window creation exits nonzero. Clipboard contention leaves the calculation unchanged; retry Ctrl+C when the clipboard is available.

## Acceptance criteria

| ID | Measurable requirement | Evidence |
| --- | --- | --- |
| AC1 | `2+3=5`, `7-12=-5`, `6*7=42`, `7/2=3.5` | Core and Windows UI tests |
| AC2 | `2+3*4=14`, `(2+3)*4=20`, `.5+1.=1.5`, negatives work | Core and UI tests |
| AC3 | Division by zero and invalid input preserve input; correction then succeeds | Core and UI tests |
| AC4 | Entry stops at 256 characters; malformed bounded inputs never yield non-finite success | Boundary tests and 10,000 deterministic fuzz cases |
| AC5 | Mouse and keyboard flows use the same controller, repeat equals is stable, new/continued expressions behave as documented | Core state and Windows UI tests |
| AC6 | All 20 buttons remain in client bounds at minimum/default/enlarged sizes, no UI message timeout over 3 seconds | Windows UI automation |
| AC7 | Four startup/shutdown cycles finish cleanly (startup within 10 seconds, shutdown within 3 seconds in CI) | Windows UI automation |
| AC8 | Release ZIP extracts to a path with spaces; packaged executable passes the UI suite; version matches tag and checksums match | Windows package gate |
| AC9 | Clean Windows 10/11 x64 usage, keyboard/numpad, clipboard, scaling 100/150/200%, multi-monitor movement, and screen-reader/button navigation are manually usable | Required before stable; NOT TESTED initially |

Physical hardware validation is N/A: the application is a calculator and has no device link, motor, sensor, driver, or physical control path. Windows desktop validation is still required before stable promotion.
