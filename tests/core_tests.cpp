#include "calculator.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <random>
#include <string>

namespace {
int checks = 0;
void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void value(const std::string& expression, double expected) {
    const auto actual = calculator::evaluate(expression);
    check(actual.ok && std::abs(actual.value - expected) <= 1e-12 * std::max(1.0, std::abs(expected)), expression);
}
void error(const std::string& expression, const std::string& expected) {
    const auto actual = calculator::evaluate(expression);
    check(!actual.ok && actual.message == expected, "error: " + expression.substr(0, 40));
}
void type(calculator::Session& session, const std::string& expression) {
    for (const auto c : expression) session.input(c);
}
struct DecimalComma : std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
};
}

int main() {
    value("2+3", 5); value("7-12", -5); value("6*7", 42); value("7/2", 3.5);
    value("2+3*4", 14); value("(2+3)*4", 20); value("8/4/2", 1); value("8-4-2", 2);
    value("-5*2", -10); value("2*-3", -6); value("2--3", 5); value("-(2+3)", -5);
    value(".5+1.", 1.5); value("0.1+0.2", 0.3); value("1e3/2", 500);
    value("1E-3*2", 0.002); value(" 1 +\t2 ", 3); value("+2", 2);
    value("0/-2", 0); value(std::string(100, '(') + "2" + std::string(100, ')'), 2);
    error("", "Enter a calculation"); error(" ", "Enter a calculation");
    error("1/0", "Cannot divide by zero"); error("1/(2-2)", "Cannot divide by zero");
    error("1/-0", "Cannot divide by zero"); error("(1+2", "Close the parentheses");
    for (const auto* text : {"1+", "2..3", "2(3)", "a", "nan", "inf", "1,5", "2**3", ")", ".", "1 2"})
        error(text, "Check the expression");
    error("1e+", "Check the exponent"); error("1e999", "Number out of range");
    error("1e-999", "Number out of range"); error("1e308*2", "Result out of range");
    error(std::string(257, '1'), "Input limit: 256 characters");
    check(calculator::format(-0.0) == "0", "negative zero display");
    check(calculator::format(0.1 + 0.2) == "0.3", "decimal display");
    const auto original_locale = std::locale();
    std::locale::global(std::locale(original_locale, new DecimalComma));
    value("1.5+2.5", 4);
    check(calculator::format(1.5) == "1.5", "locale independent output");
    std::locale::global(original_locale);

    calculator::Session session;
    check(session.display() == "0" && session.expression().empty(), "startup");
    session.equals(); check(session.display() == "0" && session.status().empty(), "empty equals");
    type(session, "2+3*4"); session.equals(); check(session.display() == "14", "calculate");
    session.equals(); check(session.display() == "14", "repeat equals is idempotent");
    type(session, "+6"); session.equals(); check(session.display() == "20", "continue result");
    type(session, "9"); session.equals(); check(session.display() == "9", "new expression after result");
    session.clear(); type(session, "12"); session.backspace();
    check(session.expression() == "1", "edit input");
    session.equals(); session.backspace(); check(session.expression().empty(), "edit result");
    type(session, "1/0"); session.equals();
    check(session.status() == "Cannot divide by zero" && session.expression() == "1/0", "error preserves input");
    session.backspace(); type(session, "2"); session.equals();
    check(session.display() == "0.5" && session.status().empty(), "recover error by editing");
    session.clear(); type(session, std::string(257, '1'));
    check(session.expression().size() == 256 && !session.status().empty(), "input bound");
    session.clear(); session.backspace(); type(session, "@;x");
    check(session.expression().empty() && session.display() == "0", "clear and reject unsupported input");
    type(session, "1e100"); session.equals(); type(session, "*2"); session.equals();
    check(session.display() == "2e+100", "scientific result can be continued");

    std::mt19937 generator(20261114);
    for (int index = 0; index < 1000; ++index) {
        const int a = static_cast<int>(generator() % 2001) - 1000;
        const int b = static_cast<int>(generator() % 2001) - 1000;
        const int c = static_cast<int>(generator() % 99) + 1;
        value("(" + std::to_string(a) + "+" + std::to_string(b) + ")/" + std::to_string(c),
            (static_cast<double>(a) + b) / c);
    }
    // Malformed and adversarial bounded input must fail cleanly or return a finite value.
    constexpr char alphabet[] = "0123456789eE.+-*/()ax; \t";
    for (int index = 0; index < 10000; ++index) {
        std::string text;
        const auto length = generator() % 258;
        for (unsigned n = 0; n < length; ++n) text += alphabet[generator() % (sizeof(alphabet) - 1)];
        const auto result = calculator::evaluate(text);
        check(result.ok ? std::isfinite(result.value) : !result.message.empty(), "bounded fuzz input");
    }
    std::cout << "PASS: " << checks << " arithmetic, state, locale, boundary and fuzz checks\n";
}
