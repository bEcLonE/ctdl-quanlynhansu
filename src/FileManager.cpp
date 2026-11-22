#include "Personnel.h"
#include "TextIO.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {
constexpr const char* ADMIN_FILE = "Administrators.txt";
constexpr const char* EMPLOYEE_FILE = "Employees.txt";

std::runtime_error fileError(const fs::path& path, const char* message) {
    return std::runtime_error(std::string(message) + " [" + path.u8string() + "]");
}

bool regularFile(const fs::path& path) {
    const auto status = fs::symlink_status(path);
    if (status.type() == fs::file_type::not_found) {
        return false;
    }
    if (!fs::is_regular_file(status)) {
        throw fileError(path, "Đường dẫn phải là tệp thường");
    }
    return true;
}

std::ifstream openInput(const fs::path& path) {
    if (!regularFile(path)) {
        throw fileError(path, "Thiếu tệp");
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw fileError(path, "Không mở được tệp");
    }
    return input;
}

void writeFile(const fs::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    output.close();
    if (!output) {
        throw fileError(path, "Không ghi được tệp");
    }
}

template<class Visitor>
void readAccounts(const fs::path& path, Visitor visit) {
    auto input = openInput(path);
    AvlTree<bool> seen;
    std::string line;
    std::size_t number = 0;
    while (readLine(input, line)) {
        if (++number == 1 && line.compare(0, 3, "\xef\xbb\xbf") == 0) {
            line.erase(0, 3);
        }
        if (line.empty()) {
            continue;
        }
        std::istringstream row(line);
        std::string username;
        std::string password;
        std::string extra;
        if (!(row >> username >> password) || row >> extra || !Validation::username(username)
            || !Validation::password(password)
            || !seen.insert(Validation::accountKey(username), true)) {
            throw std::runtime_error(path.u8string() + ": tài khoản sai hoặc trùng ở dòng "
                                     + std::to_string(number));
        }
        visit(username, password);
    }
    if (input.bad() || !input.eof()) {
        throw fileError(path, "Lỗi đọc tài khoản");
    }
}

std::string profileText(const Employee& employee) {
    return employee.name() + '\n' + employee.address() + '\n'
        + employee.phone() + '\n' + employee.email() + '\n';
}

void requireFile(const fs::path& path, const char* message) {
    if (!regularFile(path)) {
        throw fileError(path, message);
    }
}

void saveFile(const fs::path& path, const std::string& content) {
    auto temporary = path;
    temporary += ".tmp";
    if (regularFile(temporary)) {
        throw fileError(temporary, "Còn tệp tạm từ lần ghi trước");
    }
    try {
        writeFile(temporary, content);
        fs::rename(temporary, path);
    } catch (...) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        throw;
    }
}
}

FileManager::FileManager(fs::path directory)
    : directory_(std::move(directory)) {
    if (!fs::is_directory(directory_)) {
        throw fileError(directory_, "Không tìm thấy thư mục dữ liệu");
    }
    if (fs::symlink_status(directory_ / ".transaction").type() != fs::file_type::not_found) {
        throw fileError(directory_ / ".transaction", "Còn nhật ký cũ. Hãy khôi phục bằng bản trước");
    }
    const auto lockPath = directory_ / ".lock";
    regularFile(lockPath);
#ifdef _WIN32
    lock_ = CreateFileW(lockPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (lock_ == INVALID_HANDLE_VALUE) {
        throw fileError(lockPath, "Không khóa được thư mục dữ liệu");
    }
#else
    lock_ = open(lockPath.c_str(), O_CREAT | O_RDWR | O_NOFOLLOW, 0600);
    if (lock_ < 0 || flock(lock_, LOCK_EX | LOCK_NB) != 0) {
        if (lock_ >= 0) {
            close(lock_);
        }
        throw fileError(lockPath, "Không khóa được thư mục dữ liệu");
    }
#endif
}

FileManager::~FileManager() {
#ifdef _WIN32
    CloseHandle(lock_);
#else
    close(lock_);
#endif
}

void FileManager::loadAdministrators(LinkedList<Administrator>& accounts) const {
    readAccounts(directory_ / ADMIN_FILE, [&](const auto& user, const auto& password) {
        accounts.insertTail(Administrator(user, password));
    });
    if (accounts.getSize() < 3) {
        throw fileError(directory_ / ADMIN_FILE, "Cần ít nhất ba tài khoản quản trị");
    }
}

void FileManager::loadEmployees(LinkedList<Employee>& employees) const {
    readAccounts(directory_ / EMPLOYEE_FILE, [&](const auto& user, const auto& password) {
        employees.insertTail(readProfile(Employee(user, password)));
    });
}

Employee FileManager::readProfile(const Employee& employee) const {
    const auto path = directory_ / (employee.username() + ".txt");
    auto input = openInput(path);
    Employee profile(employee);
    std::string extra;
    if (!(input >> profile) || readLine(input, extra) || input.bad() || !input.eof()) {
        throw fileError(path, "Hồ sơ phải có đúng bốn dòng hợp lệ");
    }
    return profile;
}

std::string FileManager::credentials(const LinkedList<Employee>& employees,
                                     const Employee* replacement, std::string_view skip) {
    std::string result;
    employees.traverse([&](const Employee& employee) {
        if (employee.username() != skip) {
            const auto& account = replacement && replacement->username() == employee.username()
                ? *replacement : employee;
            result += account.username() + ' ' + account.password_ + '\n';
        }
    });
    return result;
}

void FileManager::addEmployee(const LinkedList<Employee>& employees, const Employee& employee) {
    const auto profile = directory_ / (employee.username() + ".txt");
    const auto accounts = directory_ / EMPLOYEE_FILE;
    if (regularFile(profile)) {
        throw fileError(profile, "Hồ sơ đã tồn tại");
    }
    requireFile(accounts, "Thiếu tệp tài khoản nhân viên");
    const auto content = credentials(employees, nullptr, {});
    saveFile(profile, profileText(employee));
    try {
        saveFile(accounts, content);
    } catch (...) {
        std::error_code error;
        fs::remove(profile, error);
        if (error) {
            throw RecoveryError{};
        }
        throw;
    }
}

void FileManager::deleteEmployee(const LinkedList<Employee>& employees, const Employee& employee) {
    const auto profile = directory_ / (employee.username() + ".txt");
    const auto accounts = directory_ / EMPLOYEE_FILE;
    auto backup = profile;
    backup += ".bak";
    requireFile(profile, "Thiếu hồ sơ cần xóa");
    requireFile(accounts, "Thiếu tệp tài khoản nhân viên");
    if (regularFile(backup)) {
        throw fileError(backup, "Còn bản sao hồ sơ từ lần xóa trước");
    }
    const auto content = credentials(employees, nullptr, employee.username());
    fs::rename(profile, backup);
    try {
        saveFile(accounts, content);
    } catch (...) {
        std::error_code error;
        fs::rename(backup, profile, error);
        if (error) {
            throw RecoveryError{};
        }
        throw;
    }
    // The employee has been removed. A failed cleanup leaves only the backup.
    std::error_code ignored;
    fs::remove(backup, ignored);
}

void FileManager::updateProfile(const Employee& employee) {
    const auto profile = directory_ / (employee.username() + ".txt");
    requireFile(profile, "Thiếu hồ sơ cần cập nhật");
    saveFile(profile, profileText(employee));
}

void FileManager::updatePassword(const LinkedList<Employee>& employees, const Employee& replacement) {
    const auto accounts = directory_ / EMPLOYEE_FILE;
    requireFile(accounts, "Thiếu tệp tài khoản nhân viên");
    saveFile(accounts, credentials(employees, &replacement, {}));
}
