#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include "Personnel.h"
#include <initializer_list>
#include <string>
#include <string_view>

class EndOfInput {};

class ConsoleUI {
public:
    ConsoleUI();
    ~ConsoleUI();
    ConsoleUI(const ConsoleUI&) = delete;
    ConsoleUI& operator=(const ConsoleUI&) = delete;

    void menu(std::string_view title, std::initializer_list<std::string_view> options) const;
    void mainMenu() const;
    void page(std::string_view title) const;
    void clearScreen() const;
    void pauseScreen() const;
    void employeeTable(const LinkedList<Employee>& employees) const;
    int choice(int minimum, int maximum) const;
    std::string line(std::string_view prompt) const;
    std::string password(std::string_view prompt) const;
    bool confirm(std::string_view prompt) const;
    void show(const Employee& employee) const;
    void message(std::string_view message) const;
    void success(std::string_view message) const;
    void error(std::string_view message) const;
    void warning(std::string_view message) const;

private:
    enum class Tone { Default, Blue, Cyan, White, Green, Yellow, Red };
    int columns() const noexcept;
    bool interactive() const noexcept;
    void status(Tone tone, std::string_view label, std::string_view message) const;
    void color(Tone tone) const;
#ifdef _WIN32
    void* input_ = nullptr;
    void* output_ = nullptr;
    unsigned short originalColor_ = 0;
    unsigned int originalOutputEncoding_ = 0;
    unsigned long originalOutputMode_ = 0;
    bool virtualTerminal_ = false;
    bool interactiveInput_ = false;
    bool interactiveOutput_ = false;
#endif
};

#endif
