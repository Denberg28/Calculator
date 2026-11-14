# Known limitations

- First release is a test candidate. Physical Windows 10/11 desktop checks for mixed DPI, assistive technologies, physical keyboard/numpad and clipboard remain NOT TESTED until reported. CI input events and screenshot capture do not substitute for those checks.
- Windows x64 only; ARM64 emulation and 32-bit Windows are not validated targets.
- Unsigned portable executable; no signing identity existed at baseline.
- 15 significant display digits and floating-point arithmetic; no exact decimal accounting or arbitrary precision. Chaining uses displayed precision. Underflow of arithmetic may round to zero.
- A dot is always the decimal separator; no grouping, percent, implicit multiplication, saved history or repeated-equals replay.
- Long expressions scroll horizontally in the native read-only edit. Backspace edits only from the end. General cursor-based expression editing and paste are outside the MVP.
- If the clipboard is locked by another process, copying may need a retry. No calculations are saved on close or crash.

No known critical/high functional defect at source self-review. Required automated failures block candidate publication; missing desktop validation blocks stable promotion.
