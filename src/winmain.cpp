#include "calculator.h"
#include <windows.h>
#include <algorithm>
#include <array>
#include <string>

namespace {
constexpr wchar_t class_name[] = L"Denberg28.Calculator";
constexpr int expression_id = 200, result_id = 201, status_id = 202;
constexpr std::array<const wchar_t*, 20> labels = {
    L"C", L"(", L")", L"Backspace", L"7", L"8", L"9", L"\u00f7",
    L"4", L"5", L"6", L"\u00d7", L"1", L"2", L"3", L"\u2212",
    L"0", L".", L"=", L"+"
};
constexpr std::array<char, 20> tokens = {
    'C', '(', ')', '\b', '7', '8', '9', '/', '4', '5', '6', '*',
    '1', '2', '3', '-', '0', '.', '=', '+'
};
struct App {
    calculator::Session session;
    HWND title = nullptr, expression = nullptr, result = nullptr, status = nullptr, hint = nullptr;
    std::array<HWND, 20> buttons{};
    HFONT small = nullptr, body = nullptr, number = nullptr, button = nullptr;
    HBRUSH background = nullptr;
    UINT dpi = 96;
    bool high_contrast = false;
} app;
int scaled(int value) { return MulDiv(value, static_cast<int>(app.dpi), 96); }
std::wstring wide(const std::string& text) { return {text.begin(), text.end()}; }
void set_font(HWND window, HFONT font) { SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE); }
HFONT font(int pixels, int weight = FW_NORMAL) {
    return CreateFontW(-scaled(pixels), 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}
void release_fonts() {
    for (auto handle : {app.small, app.body, app.number, app.button}) if (handle) DeleteObject(handle);
}
void refresh_fonts() {
    release_fonts();
    app.small = font(12);
    app.body = font(18);
    app.button = font(22);
    app.number = font(40, FW_SEMIBOLD);
    set_font(app.title, app.small);
    set_font(app.expression, app.body);
    set_font(app.status, app.small);
    set_font(app.hint, app.small);
    set_font(app.result, app.number);
    for (const auto button : app.buttons) set_font(button, app.button);
}
void fit_result() {
    RECT area{};
    GetClientRect(app.result, &area);
    if (area.right <= 0) return;
    const auto text = wide(app.session.display());
    HDC dc = GetDC(app.result);
    HFONT replacement = nullptr;
    for (int size = 40; size >= 16; --size) {
        replacement = font(size, FW_SEMIBOLD);
        const auto old = SelectObject(dc, replacement);
        SIZE extent{};
        GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &extent);
        SelectObject(dc, old);
        if (extent.cx <= area.right - scaled(8) || size == 16) break;
        DeleteObject(replacement);
    }
    set_font(app.result, replacement);
    if (app.number) DeleteObject(app.number);
    app.number = replacement;
    ReleaseDC(app.result, dc);
}
void update() {
    SetWindowTextW(app.expression, wide(app.session.expression()).c_str());
    SendMessageW(app.expression, EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1));
    SendMessageW(app.expression, EM_SCROLLCARET, 0, 0);
    SetWindowTextW(app.result, wide(app.session.display()).c_str());
    SetWindowTextW(app.status, wide(app.session.status()).c_str());
    fit_result();
}
void action(char token) {
    if (token == 'C') app.session.clear();
    else if (token == '\b') app.session.backspace();
    else if (token == '=') app.session.equals();
    else app.session.input(token);
    update();
}
void copy_result(HWND owner) {
    const auto text = wide(app.session.display());
    if (!OpenClipboard(owner)) return;
    const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
    const auto block = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (block) {
        auto* memory = GlobalLock(block);
        if (memory) {
            CopyMemory(memory, text.c_str(), bytes);
            GlobalUnlock(block);
            if (!EmptyClipboard() || !SetClipboardData(CF_UNICODETEXT, block)) GlobalFree(block);
        } else GlobalFree(block);
    }
    CloseClipboard();
}
void layout(HWND window) {
    if (!app.result) return;
    RECT area{};
    GetClientRect(window, &area);
    const int margin = scaled(16), gap = scaled(8), width = area.right - margin * 2;
    auto place = [](HWND child, int x, int y, int w, int h) { MoveWindow(child, x, y, std::max(1, w), std::max(1, h), TRUE); };
    place(app.title, margin, scaled(16), width, scaled(22));
    place(app.expression, margin, scaled(46), width, scaled(30));
    place(app.result, margin, scaled(80), width, scaled(58));
    place(app.status, margin, scaled(148), width, scaled(24));
    const int top = scaled(188), bottom = area.bottom - scaled(36);
    const int cell_width = (width - gap * 3) / 4;
    const int cell_height = (bottom - top - gap * 4) / 5;
    for (int index = 0; index < 20; ++index) {
        const int col = index % 4, row = index / 4;
        const int x = margin + col * (cell_width + gap);
        const int y = top + row * (cell_height + gap);
        place(app.buttons[static_cast<std::size_t>(index)], x, y,
            col == 3 ? area.right - margin - x : cell_width,
            row == 4 ? bottom - y : cell_height);
    }
    place(app.hint, margin, area.bottom - scaled(26), width, scaled(20));
    fit_result();
    InvalidateRect(window, nullptr, TRUE);
}
void draw_button(const DRAWITEMSTRUCT& item) {
    const int index = static_cast<int>(item.CtlID) - 100;
    if (index < 0 || index >= 20) return;
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    const bool equals = index == 18;
    const bool utility = index < 4;
    COLORREF background = equals ? RGB(25, 94, 190) : utility ? RGB(227, 233, 241) : RGB(255, 255, 255);
    COLORREF foreground = equals ? RGB(255, 255, 255) : RGB(29, 39, 55);
    if (pressed) background = equals ? RGB(15, 65, 140) : RGB(205, 218, 236);
    if (app.high_contrast) {
        background = GetSysColor(pressed ? COLOR_HIGHLIGHT : COLOR_BTNFACE);
        foreground = GetSysColor(pressed ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT);
    }
    const auto brush = CreateSolidBrush(background);
    FillRect(item.hDC, &item.rcItem, brush);
    DeleteObject(brush);
    const auto border = CreateSolidBrush(app.high_contrast ? GetSysColor(COLOR_WINDOWTEXT) : RGB(211, 219, 230));
    FrameRect(item.hDC, &item.rcItem, border);
    DeleteObject(border);
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, foreground);
    const auto old = SelectObject(item.hDC, index == 3 ? app.small : app.button);
    RECT text_area = item.rcItem;
    DrawTextW(item.hDC, labels[static_cast<std::size_t>(index)], -1, &text_area, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(item.hDC, old);
    if ((item.itemState & ODS_FOCUS) != 0) {
        RECT focus = item.rcItem;
        InflateRect(&focus, -scaled(4), -scaled(4));
        DrawFocusRect(item.hDC, &focus);
    }
}
void theme(HWND window) {
    HIGHCONTRASTW contrast{sizeof(HIGHCONTRASTW), 0, nullptr};
    SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0);
    app.high_contrast = (contrast.dwFlags & HCF_HIGHCONTRASTON) != 0;
    if (app.background) DeleteObject(app.background);
    app.background = CreateSolidBrush(app.high_contrast ? GetSysColor(COLOR_WINDOW) : RGB(244, 247, 251));
    InvalidateRect(window, nullptr, TRUE);
    for (const auto child : app.buttons) InvalidateRect(child, nullptr, TRUE);
}
LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        app.dpi = GetDpiForWindow(window);
        const auto instance = reinterpret_cast<LPCREATESTRUCTW>(lparam)->hInstance;
        auto control = [&](const wchar_t* type, const wchar_t* text, DWORD style, int id) {
            return CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style,
                0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
        };
        app.title = control(L"STATIC", L"STANDARD", SS_LEFT, 203);
        app.expression = control(L"EDIT", L"", ES_READONLY | ES_RIGHT | ES_AUTOHSCROLL, expression_id);
        app.result = control(L"EDIT", L"0", ES_READONLY | ES_RIGHT | ES_AUTOHSCROLL, result_id);
        app.status = control(L"STATIC", L"", SS_LEFT, status_id);
        app.hint = control(L"STATIC", L"Enter: calculate    Esc: clear    Ctrl+C: copy", SS_LEFT, 204);
        for (std::size_t index = 0; index < app.buttons.size(); ++index)
            app.buttons[index] = control(L"BUTTON", labels[index], WS_TABSTOP | BS_OWNERDRAW, 100 + static_cast<int>(index));
        if (!app.title || !app.expression || !app.result || !app.status || !app.hint ||
            std::any_of(app.buttons.begin(), app.buttons.end(), [](HWND child) { return child == nullptr; })) return -1;
        refresh_fonts();
        theme(window);
        layout(window);
        return 0;
    }
    case WM_COMMAND:
        if (HIWORD(wparam) == BN_CLICKED && LOWORD(wparam) >= 100 && LOWORD(wparam) < 120) {
            action(tokens[LOWORD(wparam) - 100]);
        }
        return 0;
    case WM_SIZE: layout(window); return 0;
    case WM_DPICHANGED:
        app.dpi = HIWORD(wparam);
        refresh_fonts();
        if (const auto rect = reinterpret_cast<RECT*>(lparam))
            SetWindowPos(window, nullptr, rect->left, rect->top, rect->right - rect->left,
                rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        layout(window);
        return 0;
    case WM_GETMINMAXINFO: {
        auto* limits = reinterpret_cast<MINMAXINFO*>(lparam);
        RECT minimum{0, 0, scaled(320), scaled(480)};
        AdjustWindowRectExForDpi(&minimum, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CONTROLPARENT, app.dpi);
        limits->ptMinTrackSize = {minimum.right - minimum.left, minimum.bottom - minimum.top};
        return 0;
    }
    case WM_DRAWITEM: draw_button(*reinterpret_cast<DRAWITEMSTRUCT*>(lparam)); return TRUE;
    case WM_CTLCOLORSTATIC: {
        const auto dc = reinterpret_cast<HDC>(wparam);
        SetBkMode(dc, TRANSPARENT);
        SetBkColor(dc, app.high_contrast ? GetSysColor(COLOR_WINDOW) : RGB(244, 247, 251));
        SetTextColor(dc, app.high_contrast ? GetSysColor(COLOR_WINDOWTEXT) :
            reinterpret_cast<HWND>(lparam) == app.status ? RGB(165, 35, 35) : RGB(29, 39, 55));
        return reinterpret_cast<LRESULT>(app.background);
    }
    case WM_ERASEBKGND: {
        RECT area{};
        GetClientRect(window, &area);
        FillRect(reinterpret_cast<HDC>(wparam), &area, app.background);
        return TRUE;
    }
    case WM_SETTINGCHANGE: case WM_SYSCOLORCHANGE: theme(window); return 0;
    case WM_SETFOCUS: SetFocus(app.buttons[0]); return 0;
    case WM_DESTROY:
        release_fonts();
        if (app.background) DeleteObject(app.background);
        PostQuitMessage(0);
        return 0;
    default: return DefWindowProcW(window, message, wparam, lparam);
    }
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    WNDCLASSEXW type{};
    type.cbSize = sizeof(type);
    type.lpfnWndProc = procedure;
    type.hInstance = instance;
    type.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    type.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    type.lpszClassName = class_name;
    if (!RegisterClassExW(&type)) return 1;
    app.dpi = GetDpiForSystem();
    RECT bounds{0, 0, scaled(360), scaled(540)};
    AdjustWindowRectExForDpi(&bounds, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CONTROLPARENT, app.dpi);
    const auto window = CreateWindowExW(WS_EX_CONTROLPARENT, class_name, L"Calculator",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left,
        bounds.bottom - bounds.top, nullptr, nullptr, instance, nullptr);
    if (!window) return 1;
    ShowWindow(window, show);
    MSG message{};
    for (;;) {
        const auto state = GetMessageW(&message, nullptr, 0, 0);
        if (state == 0) return static_cast<int>(message.wParam);
        if (state == -1) return 1;
        const bool belongs = message.hwnd == window || IsChild(window, message.hwnd);
        if (belongs && message.message == WM_KEYDOWN) {
            if (message.wParam == VK_RETURN) { action('='); continue; }
            if (message.wParam == VK_ESCAPE || message.wParam == VK_DELETE) { action('C'); continue; }
            if (message.wParam == VK_BACK) { action('\b'); continue; }
            if (message.wParam == 'C' && (GetKeyState(VK_CONTROL) & 0x8000)) { copy_result(window); continue; }
        }
        if (belongs && message.message == WM_CHAR) {
            if (message.wParam == 'c' || message.wParam == 'C') action('C');
            else if (message.wParam < 128 && message.wParam != ' ') action(static_cast<char>(message.wParam));
            else { TranslateMessage(&message); DispatchMessageW(&message); }
            continue;
        }
        if (IsDialogMessageW(window, &message)) continue;
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}
