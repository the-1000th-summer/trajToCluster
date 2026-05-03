#pragma once

#include <stdexcept>

class DateTimeException : public std::runtime_error {
public:
    explicit DateTimeException(const std::string &message) : std::runtime_error(message) {}
};
