#ifndef PERSONNEL_H
#define PERSONNEL_H

#include "AvlTree.h"
#include "LinkedList.h"
#include <filesystem>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

class Validation {
public:
    static std::string accountKey(std::string_view username);
    static bool username(std::string_view value);
    static bool password(std::string_view value);
    static bool text(std::string_view value);
    static bool phone(std::string_view value);
    static bool email(std::string_view value);
};

class User {
public:
    User() = default;
    User(std::string username, std::string password)
        : username_(std::move(username)), password_(std::move(password)) {}

    const std::string& username() const noexcept { return username_; }
    bool matchesPassword(std::string_view value) const noexcept { return password_ == value; }

protected:
    void swapAccount(User& other) noexcept;

private:
    friend class FileManager;
    friend class EmployeeService;
    std::string username_;
    std::string password_;
};

class RecoveryError : public std::exception {
public:
    const char* what() const noexcept override {
        return "Không hoàn tác được thay đổi tệp. Hãy kiểm tra quyền và các tệp .bak.";
    }
};

class Administrator : public User {
public:
    using User::User;
};

enum class ProfileField { Name, Address, Phone, Email };

class Employee : public User {
public:
    static constexpr const char* DEFAULT_PASSWORD = "111111";
    using User::User;

    const std::string& name() const noexcept { return name_; }
    const std::string& address() const noexcept { return address_; }
    const std::string& phone() const noexcept { return phone_; }
    const std::string& email() const noexcept { return email_; }
    void setField(ProfileField field, std::string value);
    void swap(Employee& other) noexcept;

    friend std::istream& operator>>(std::istream& input, Employee& employee);
    friend std::ostream& operator<<(std::ostream& output, const Employee& employee);

private:
    std::string name_;
    std::string address_;
    std::string phone_;
    std::string email_;
};

class FileManager {
public:
    explicit FileManager(std::filesystem::path directory);
    ~FileManager();
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;

private:
    friend class EmployeeService;
    friend class AuthService;

    void loadAdministrators(LinkedList<Administrator>& accounts) const;
    void loadEmployees(LinkedList<Employee>& employees) const;
    Employee readProfile(const Employee& employee) const;
    void addEmployee(const LinkedList<Employee>& employees, const Employee& employee);
    void deleteEmployee(const LinkedList<Employee>& employees, const Employee& employee);
    void updateProfile(const Employee& employee);
    void updatePassword(const LinkedList<Employee>& employees, const Employee& replacement);

    static std::string credentials(const LinkedList<Employee>& employees,
                                   const Employee* replacement, std::string_view skip);
    std::filesystem::path directory_;
#ifdef _WIN32
    void* lock_ = nullptr;
#else
    int lock_ = -1;
#endif
};

class AuthService;

class EmployeeService {
public:
    explicit EmployeeService(FileManager& files);
    EmployeeService(const EmployeeService&) = delete;
    EmployeeService& operator=(const EmployeeService&) = delete;

    const Employee* findEmployee(std::string_view username) const;
    const Employee* linearSearch(std::string_view username) const;
    void add(Employee employee);
    void remove(std::string_view username);
    void update(std::string_view username, ProfileField field, std::string value);
    void sortByName();
    Employee readProfile(std::string_view username) const;
    const LinkedList<Employee>& employees() const noexcept { return employees_; }
    const AvlTree<Employee*>& index() const noexcept { return index_; }

private:
    friend class AuthService;
    Employee& require(std::string_view username) const;
    void replacePassword(Employee& employee, const std::string& password);
    FileManager& files_;
    LinkedList<Employee> employees_;
    AvlTree<Employee*> index_;
};

class AuthService {
public:
    AuthService(FileManager& files, EmployeeService& employees);
    bool authenticateAdmin(std::string_view username, std::string_view password) const;
    const Employee* authenticateEmployee(std::string_view username, std::string_view password) const;
    void changePassword(std::string_view username, std::string_view current,
                        const std::string& next, std::string_view confirmation);

private:
    LinkedList<Administrator> admins_;
    EmployeeService& employees_;
};

#endif
