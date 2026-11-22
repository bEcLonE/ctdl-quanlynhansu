#include "Personnel.h"
#include "TextIO.h"
#include "Utf8.h"

#include <initializer_list>
#include <ostream>

namespace {
bool alnum(char value) noexcept {
    return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z')
        || (value >= '0' && value <= '9');
}
}

std::string Validation::accountKey(std::string_view username) {
    std::string result(username);
    for (auto& letter : result) {
        if (letter >= 'A' && letter <= 'Z') {
            letter = static_cast<char>(letter + ('a' - 'A'));
        }
    }
    return result;
}

bool Validation::username(std::string_view value) {
    if (value.empty() || value.size() > 48) {
        return false;
    }
    for (const auto ch : value) {
        if (!alnum(ch) && ch != '_' && ch != '-') {
            return false;
        }
    }
    const auto key = accountKey(value);
    if (key == "administrators" || key == "employees" || key == "con"
        || key == "prn" || key == "aux" || key == "nul") {
        return false;
    }
    return !(key.size() == 4 && (key.compare(0, 3, "com") == 0
        || key.compare(0, 3, "lpt") == 0) && key[3] >= '0' && key[3] <= '9');
}

bool Validation::password(std::string_view value) {
    if (value.empty() || value.size() > 128) {
        return false;
    }
    for (const auto ch : value) {
        if (ch < '!' || ch > '~') {
            return false;
        }
    }
    return true;
}

bool Validation::text(std::string_view value) {
    if (value.empty() || value.size() > 512) {
        return false;
    }
    bool nonSpace = false;
    std::size_t offset = 0;
    while (offset < value.size()) {
        std::uint32_t code = 0;
        if (!Utf8::next(value, offset, code) || code < 0x20 || (code >= 0x7f && code <= 0x9f)) {
            return false;
        }
        nonSpace = nonSpace || (code != ' ' && code != 0xa0 && code != 0x3000);
    }
    return nonSpace;
}

bool Validation::phone(std::string_view value) {
    if (value.empty() || value.size() > 32) {
        return false;
    }
    if (value.front() == '+') {
        value.remove_prefix(1);
    }
    int digits = 0;
    for (const auto ch : value) {
        if (ch >= '0' && ch <= '9') {
            ++digits;
        } else if (ch != ' ' && ch != '-' && ch != '(' && ch != ')') {
            return false;
        }
    }
    return digits >= 7 && digits <= 15;
}

bool Validation::email(std::string_view value) {
    const auto at = value.find('@');
    if (value.size() > 254 || at == std::string_view::npos || at == 0 || at > 64
        || value.back() == '.' || value[at - 1] == '.' || value.front() == '.'
        || value.find("..") != std::string_view::npos) {
        return false;
    }
    for (const auto ch : value.substr(0, at)) {
        if (!alnum(ch) && ch != '.' && ch != '_' && ch != '+' && ch != '-') {
            return false;
        }
    }
    auto domain = value.substr(at + 1);
    if (domain.find('.') == std::string_view::npos) {
        return false;
    }
    while (!domain.empty()) {
        const auto dot = domain.find('.');
        const auto label = domain.substr(0, dot);
        if (label.empty() || label.size() > 63 || !alnum(label.front()) || !alnum(label.back())) {
            return false;
        }
        for (const auto ch : label) {
            if (!alnum(ch) && ch != '-') {
                return false;
            }
        }
        if (dot == std::string_view::npos) {
            return true;
        }
        domain.remove_prefix(dot + 1);
    }
    return false;
}

void User::swapAccount(User& other) noexcept {
    username_.swap(other.username_);
    password_.swap(other.password_);
}

void Employee::setField(ProfileField field, std::string value) {
    switch (field) {
    case ProfileField::Name:
    case ProfileField::Address:
        if (!Validation::text(value)) {
            throw std::invalid_argument("Họ tên hoặc địa chỉ không hợp lệ.");
        }
        (field == ProfileField::Name ? name_ : address_) = std::move(value);
        return;
    case ProfileField::Phone:
        if (!Validation::phone(value)) {
            throw std::invalid_argument("Số điện thoại phải có 7–15 chữ số.");
        }
        phone_ = std::move(value);
        return;
    case ProfileField::Email:
        if (!Validation::email(value)) {
            throw std::invalid_argument("Email không hợp lệ.");
        }
        email_ = std::move(value);
        return;
    }
    throw std::invalid_argument("Trường hồ sơ không hợp lệ.");
}

void Employee::swap(Employee& other) noexcept {
    swapAccount(other);
    name_.swap(other.name_);
    address_.swap(other.address_);
    phone_.swap(other.phone_);
    email_.swap(other.email_);
}

std::istream& operator>>(std::istream& input, Employee& employee) {
    Employee candidate(employee);
    for (const auto field : {ProfileField::Name, ProfileField::Address, ProfileField::Phone,
                             ProfileField::Email}) {
        std::string line;
        if (!readLine(input, line)) {
            input.setstate(std::ios::failbit);
            return input;
        }
        if (field == ProfileField::Name && line.compare(0, 3, "\xef\xbb\xbf") == 0) {
            line.erase(0, 3);
        }
        try {
            candidate.setField(field, std::move(line));
        } catch (const std::invalid_argument&) {
            input.setstate(std::ios::failbit);
            return input;
        }
    }
    employee.swap(candidate);
    return input;
}

std::ostream& operator<<(std::ostream& output, const Employee& employee) {
    return output << "Username: " << employee.username() << '\n'
                  << "Họ tên: " << employee.name() << '\n'
                  << "Địa chỉ: " << employee.address() << '\n'
                  << "Điện thoại: " << employee.phone() << '\n'
                  << "Email: " << employee.email() << '\n';
}
