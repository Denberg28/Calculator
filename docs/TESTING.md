# Verification and release gates

Evidence is self-review and automation, not independent QA. The baseline was empty. Core regressions cover arithmetic, session transitions, error correction and input bounds. No physical control path or hardware HIL applies (N/A).

## Required automated gates

| Gate | Check | Local initial result | Release evidence |
| --- | --- | --- | --- |
| A | C++17 warnings treated as errors; Linux and Windows builds | Linux PASS; Windows delegated to CI | Actions logs and compiler metadata |
| A | Source/version/manifest checks, no third-party app dependencies, focused secret/security review | See check_source.py output | Actions log; source self-review |
| B/C | Core arithmetic/state/locale/boundaries plus deterministic generated arithmetic and malformed input | PASS: 11,059 checks with GCC | ctest log |
| B/C | Linux AddressSanitizer/UndefinedBehaviorSanitizer | PASS with local leak detection disabled; local LeakSanitizer BLOCKED by ptrace environment | CI runs leak detection enabled; ctest log, abort on failure |
| B/D | Real Windows executable: 20 buttons, keyboard message flow, errors/recovery, three window sizes, screenshot, four lifecycle cycles | NOT TESTED on Linux host | Windows ctest log, ui-smoke.png |
| F | Portable ZIP version/commit/checksum, path with spaces, packaged executable UI smoke | NOT TESTED on Linux host | package-build.json and Windows gate log |
| E | Physical signal path / HIL / link faults | N/A: calculator has no hardware link | Scope in SPECIFICATION.md |

The workflow is the authoritative result for its exact source SHA. Package `BUILD-INFO.json` identifies commit, version, compiler, environment and evidence; `TEST-RESULTS.txt` contains ctest output. A published candidate means the required automated gates passed; it does not imply manual desktop validation. Source scanners are defense in depth, not a comprehensive security audit. Self-review verifies no code evaluation, shell execution, network, file writes, credentials, privileged operation, or unbounded input path in application code.

## Commands

Use the build/test commands in README. Windows `tests/windows-ui.ps1 -Exe <path>` starts an actual app process, sends button and keyboard messages, asserts native display text, checks bounds, captures its window and checks clean shutdown. It does not inject real physical keyboard input or verify a screen reader. UI waits are bounded; failure or timeout is a release blocker. The packaging script extracts the produced ZIP and reruns this test against the extracted executable before publication.

Linux sanitizer build:

```sh
cmake -S . -B build-sanitized -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build build-sanitized
ctest --test-dir build-sanitized --output-on-failure
```

## Required manual checks before stable

Run on clean Windows 10 and Windows 11 x64 standard-user accounts; record OS/build, display scaling, app version, source SHA and actual results. These checks are **NOT TESTED** until observed and recorded. Do not publish a stable tag while they remain open.

1. Verify SHA-256, extract to a path with spaces on HDD/SSD, launch without additional runtime or elevated permissions. Confirm version in file Properties.
2. With mouse and physical keyboard/numpad, enter `2+3*4` (14), `(2+3)*4` (20), `7/2` (3.5), `-5*2` (-10), and `0.1+0.2` (0.3). Check Enter, equals, Esc, Delete, Backspace and Tab/Shift+Tab/Space. With focus on a button and each display, confirm digits and operators still work.
3. Calculate `1/0`, correct the zero to `2`, obtain `0.5`; fix incomplete parentheses; clear and resume. Copy a result with Ctrl+C and paste into Notepad. Confirm no accidental stale calculation after Clear.
4. At 100%, 150%, and 200% scaling, resize between minimum, normal and maximized sizes. Move between differently scaled monitors. Confirm readable expression/result/error, all labels visible, no overlaps, clipped buttons or lost focus. Check a 256-character expression and a result such as `1e100` and `1/3`.
5. Enable Windows high contrast and use Narrator: confirm focus visibility and readable button names/result. Test keyboard-only operation. Record any accessibility limitations before promotion.
6. Reopen four times, minimize/restore, Alt+Tab, and close. Confirm startup within 10 seconds and close within 3 seconds; idle app stays responsive. Open the package offline and verify no runtime installer is requested.
7. Upgrade to a separate folder, reopen previous executable to roll back, and delete the new folder to uninstall. Confirm no data migration or administrator rights are required.

## Release tiers

Current version: `0.1.0-rc.1`, candidate only. No existing stable baseline. Candidate publication is authorized after automated source, Linux/Windows behavior, and packaging gates pass with no known critical/high defects. Stable requires the manual matrix above; the current workflow explicitly rejects stable tags. Add observed evidence and update the policy before enabling stable delivery. Published tags/artifacts must never be reused or replaced to hide a regression.
