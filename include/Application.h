#ifndef APPLICATION_H
#define APPLICATION_H

#include "ConsoleUI.h"

class Application {
public:
    Application(ConsoleUI& ui, std::filesystem::path directory);
    void run();

private:
    bool login(bool administrator);
    void adminMenu();
    void employeeMenu(std::string username);
    void changePassword(std::string_view username);
    void addEmployee();
    void deleteEmployee();
    void showEmployee(const Employee* employee) const;
    void listEmployees() const;
    ConsoleUI& ui_;
    FileManager files_;
    EmployeeService employees_;
    AuthService auth_;
};

#endif
