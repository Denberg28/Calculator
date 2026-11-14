#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace calculator {
inline constexpr std::size_t max_input = 256;
struct Result {
    bool ok;
    double value;
    std::string message;
};
Result evaluate(std::string_view expression);
std::string format(double value);

class Session {
public:
    void input(char character);
    void equals();
    void backspace();
    void clear();
    const std::string& expression() const { return expression_; }
    const std::string& display() const { return display_; }
    const std::string& status() const { return status_; }
private:
    std::string expression_;
    std::string display_ = "0";
    std::string status_;
    bool completed_ = false;
};
}
