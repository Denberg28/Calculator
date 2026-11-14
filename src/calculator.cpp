#include "calculator.h"

#include <charconv>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace calculator {
namespace {
bool digit(char c) { return c >= '0' && c <= '9'; }
bool operation(char c) { return c == '+' || c == '-' || c == '*' || c == '/'; }

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}
    double run() {
        skip();
        if (pos_ == text_.size()) fail("Enter a calculation");
        const double value = sum();
        skip();
        if (pos_ != text_.size()) fail("Check the expression");
        return value;
    }
private:
    std::string_view text_;
    std::size_t pos_ = 0;
    [[noreturn]] static void fail(const char* reason) { throw std::runtime_error(reason); }
    void skip() {
        while (pos_ < text_.size() && (text_[pos_] == ' ' || text_[pos_] == '\t')) ++pos_;
    }
    bool take(char c) {
        skip();
        if (pos_ < text_.size() && text_[pos_] == c) { ++pos_; return true; }
        return false;
    }
    static double finite(double value) {
        if (!std::isfinite(value)) fail("Result out of range");
        return value;
    }
    double sum() {
        double value = product();
        for (;;) {
            if (take('+')) value = finite(value + product());
            else if (take('-')) value = finite(value - product());
            else return value;
        }
    }
    double product() {
        double value = unary();
        for (;;) {
            if (take('*')) value = finite(value * unary());
            else if (take('/')) {
                const double divisor = unary();
                if (divisor == 0.0) fail("Cannot divide by zero");
                value = finite(value / divisor);
            } else return value;
        }
    }
    double unary() {
        if (take('+')) return unary();
        if (take('-')) return -unary();
        if (take('(')) {
            const double value = sum();
            if (!take(')')) fail("Close the parentheses");
            return value;
        }
        skip();
        const auto start = pos_;
        bool has_digit = false;
        while (pos_ < text_.size() && digit(text_[pos_])) { ++pos_; has_digit = true; }
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            while (pos_ < text_.size() && digit(text_[pos_])) { ++pos_; has_digit = true; }
        }
        if (!has_digit) fail("Check the expression");
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            const auto exponent_start = pos_;
            while (pos_ < text_.size() && digit(text_[pos_])) ++pos_;
            if (exponent_start == pos_) fail("Check the exponent");
        }
        double value = 0;
        const char* begin = text_.data() + start;
        const char* end = text_.data() + pos_;
        const auto parsed = std::from_chars(begin, end, value);
        if (parsed.ec == std::errc::result_out_of_range) fail("Number out of range");
        if (parsed.ec != std::errc{} || parsed.ptr != end) fail("Check the number");
        return finite(value);
    }
};
}

Result evaluate(std::string_view expression) {
    if (expression.size() > max_input) return {false, 0, "Input limit: 256 characters"};
    try { return {true, Parser(expression).run(), {}}; }
    catch (const std::runtime_error& error) { return {false, 0, error.what()}; }
}

std::string format(double value) {
    if (value == 0) return "0";
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(15) << value;
    return stream.str();
}

void Session::input(char c) {
    if (!(digit(c) || operation(c) || c == '.' || c == '(' || c == ')' || c == 'e' || c == 'E')) return;
    if (completed_) {
        expression_ = operation(c) ? display_ : "";
        completed_ = false;
    }
    if (expression_.size() >= max_input) { status_ = "Input limit: 256 characters"; return; }
    expression_ += c;
    display_ = "0";
    status_.clear();
}
void Session::equals() {
    if (completed_ || expression_.empty()) return;
    const auto result = evaluate(expression_);
    if (!result.ok) { status_ = result.message; return; }
    display_ = format(result.value);
    status_.clear();
    completed_ = true;
}
void Session::backspace() {
    if (completed_) { expression_ = display_; completed_ = false; }
    if (!expression_.empty()) expression_.pop_back();
    display_ = "0";
    status_.clear();
}
void Session::clear() {
    expression_.clear();
    display_ = "0";
    status_.clear();
    completed_ = false;
}
}
