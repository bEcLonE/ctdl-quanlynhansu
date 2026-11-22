#include "Personnel.h"

EmployeeService::EmployeeService(FileManager& files) : files_(files) {
    files_.loadEmployees(employees_);
    // Loading builds the index on the same stable nodes used by the list.
    employees_.traverseMutable([&](Employee& employee) {
        index_.insert(Validation::accountKey(employee.username()), &employee);
    });
}

const Employee* EmployeeService::findEmployee(std::string_view username) const {
    const auto item = index_.search(Validation::accountKey(username));
    return item ? *item : nullptr;
}

const Employee* EmployeeService::linearSearch(std::string_view username) const {
    const auto key = Validation::accountKey(username);
    return employees_.find([&](const Employee& employee) {
        return Validation::accountKey(employee.username()) == key;
    });
}

Employee& EmployeeService::require(std::string_view username) const {
    const auto item = index_.search(Validation::accountKey(username));
    if (!item) {
        throw std::invalid_argument("Không tìm thấy nhân viên.");
    }
    return **item;
}

void EmployeeService::add(Employee employee) {
    if (!Validation::username(employee.username()) || !employee.matchesPassword(Employee::DEFAULT_PASSWORD)
        || !Validation::text(employee.name()) || !Validation::text(employee.address())
        || !Validation::phone(employee.phone()) || !Validation::email(employee.email())) {
        throw std::invalid_argument("Thông tin nhân viên mới không hợp lệ.");
    }
    const auto key = Validation::accountKey(employee.username());
    if (index_.search(key)) {
        throw std::invalid_argument("Username đã tồn tại.");
    }
    auto& inserted = employees_.insertTail(employee);
    try {
        index_.insert(key, &inserted);
        files_.addEmployee(employees_, inserted);
    } catch (...) {
        index_.remove(key);
        employees_.removeByKey([&](const Employee& entry) { return &entry == &inserted; });
        throw;
    }
}

void EmployeeService::remove(std::string_view username) {
    auto& employee = require(username);
    const auto key = Validation::accountKey(employee.username());
    files_.deleteEmployee(employees_, employee);
    index_.remove(key);
    employees_.removeByKey([&](const Employee& entry) { return &entry == &employee; });
}

void EmployeeService::update(std::string_view username, ProfileField field, std::string value) {
    auto& employee = require(username);
    auto replacement = employee;
    replacement.setField(field, std::move(value));
    files_.updateProfile(replacement);
    employee.swap(replacement);
}

void EmployeeService::sortByName() {
    employees_.sort([](const Employee& first, const Employee& second) noexcept {
        return first.name() < second.name();
    });
}

Employee EmployeeService::readProfile(std::string_view username) const {
    return files_.readProfile(require(username));
}

void EmployeeService::replacePassword(Employee& employee, const std::string& password) {
    auto replacement = employee;
    replacement.password_ = password;
    files_.updatePassword(employees_, replacement);
    employee.swap(replacement);
}

AuthService::AuthService(FileManager& files, EmployeeService& employees) : employees_(employees) {
    files.loadAdministrators(admins_);
}

bool AuthService::authenticateAdmin(std::string_view username, std::string_view password) const {
    const auto key = Validation::accountKey(username);
    return admins_.find([&](const Administrator& account) {
        return Validation::accountKey(account.username()) == key && account.matchesPassword(password);
    }) != nullptr;
}

const Employee* AuthService::authenticateEmployee(std::string_view username,
                                                   std::string_view password) const {
    const auto* employee = employees_.findEmployee(username);
    return employee && employee->matchesPassword(password) ? employee : nullptr;
}

void AuthService::changePassword(std::string_view username, std::string_view current,
                                 const std::string& next, std::string_view confirmation) {
    auto& employee = employees_.require(username);
    if (!employee.matchesPassword(current)) {
        throw std::invalid_argument("Mật khẩu hiện tại không đúng.");
    }
    if (next != confirmation) {
        throw std::invalid_argument("Mật khẩu xác nhận không khớp.");
    }
    if (!Validation::password(next) || next.size() < 6 || next == Employee::DEFAULT_PASSWORD
        || next == current) {
        throw std::invalid_argument("Mật khẩu mới phải có 6–128 ký tự ASCII in được, không có "
                                    "khoảng trắng và khác mật khẩu hiện tại hoặc mặc định.");
    }
    employees_.replacePassword(employee, next);
}
