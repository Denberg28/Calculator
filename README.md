# Calculator

A small, offline calculator for Windows 10/11 x64. Supports addition, subtraction, multiplication, division, decimal numbers, negative numbers, parentheses, and normal operator precedence.

## Download and run

Download the Windows ZIP from [Releases](https://github.com/Denberg28/Calculator/releases), extract it to any writable folder on your HDD or SSD, and open `Calculator.exe`. No installation, administrator rights, network, account, or additional runtime is required. This first version is a **test candidate**, pending the manual checks in `docs/TESTING.md`.

The executable is unsigned. There is no existing signing certificate in this new repository. Windows may show a reputation prompt; verify the release origin and SHA-256 checksum before running it.

## Use

Click the keys or type a calculation, then press **Enter** or **=**. For example, `2+3*4` gives `14`; `(2+3)*4` gives `20`. Start a negative number with `-`.

| Key | Action |
| --- | --- |
| `0–9`, `.`, `+`, `-`, `*`, `/`, `(`, `)` | Enter a calculation |
| Enter or `=` | Calculate |
| Backspace | Remove the last character; after a result, edit the displayed result |
| Esc, Delete, or C | Clear everything |
| Ctrl+C | Copy the displayed result |
| Tab / Shift+Tab, Space | Move between buttons / press the focused button |

After calculating, an operator continues from the displayed result; a digit, decimal point, or opening parenthesis starts a new calculation. Repeated equals leaves the result unchanged. Errors preserve your input: use Backspace to correct it, or C to start again. No implicit multiplication: write `2*(3+4)`.

Decimal input always uses a dot. Results show up to 15 significant digits; continuing a calculation uses that displayed precision. Very large/small values use scientific notation, and `e`/`E` notation can also be typed. This is a basic floating-point calculator, unsuitable for exact accounting or arbitrary precision calculations. Input is limited to 256 characters.

## Build and test

Windows: install the free Visual Studio 2022 Build Tools with **Desktop development with C++**, the Windows SDK, and CMake 3.20 or later. From a Developer PowerShell at the repository root:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
powershell -NoProfile -File scripts/package.ps1 -BuildDir build
```

The app uses C++17, Win32, the Windows SDK, and the statically linked MSVC runtime. There are no third-party application dependencies. CI uses `windows-2022` (Visual Studio 2022) and `ubuntu-24.04`; exact runner/compiler versions appear in logs and package metadata. Hosted runner images receive updates; the build is reproducible from source with the documented toolchain, not promised byte identical across SDK/compiler updates.

Arithmetic tests also run on Linux with GCC 12 or later:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
python3 scripts/check_source.py
```

## Update, rollback, and remove

Close Calculator, extract a newer version into a separate folder, and launch the new executable. Keep the prior folder to roll back by closing the new app and reopening the previous version. No settings or saved data require migration. Delete the extracted folder to uninstall. The app itself writes no calculation history, settings, or user data to disk; calculations are lost when closed.

## Project records

- [Scope, assumptions, and acceptance](docs/SPECIFICATION.md)
- [Architecture and interfaces](docs/ARCHITECTURE.md)
- [Verification and manual validation](docs/TESTING.md)
- [Known limitations](docs/KNOWN_ISSUES.md)
- [Changelog](CHANGELOG.md)

License: MIT.
