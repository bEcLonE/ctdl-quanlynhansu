#ifndef APPLICATION_H
#define APPLICATION_H

#include "ConsoleUI.h"
#include "Queue.h"
#include "Stack.h"

class Application {
public:
    Application(ConsoleUI& ui, std::filesystem::path directory);
    void run();

private:
    class Update {
    public:
        Update(std::string username, ProfileField field, std::string value)
            : username_(std::move(username)), field_(field), value_(std::move(value)) {}
        const std::string& username() const noexcept { return username_; }
        ProfileField field() const noexcept { return field_; }
        const std::string& value() const noexcept { return value_; }

    private:
        std::string username_;
        ProfileField field_;
        std::string value_;
    };

    void login(bool administrator);
    void adminMenu();
    void employeeMenu(std::string username);
    void changePassword(std::string_view username);
    void addEmployee();
    void deleteEmployee();
    void showEmployee(const Employee* employee) const;
    void listEmployees() const;
    void operationsMenu();
    void indexMenu() const;
    bool enqueueUpdate();
    void processUpdate();

    template<class Action>
    void logOperation(std::string entry, Action action) {
        history_.push(entry);
        try {
            action();
        } catch (...) {
            history_.pop();
            throw;
        }
    }

    ConsoleUI& ui_;
    FileManager files_;
    EmployeeService employees_;
    AuthService auth_;
    Stack<std::string> history_;
    Queue<Update> updates_;
};

#endif
