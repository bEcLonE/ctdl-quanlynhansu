#include "ConsoleUI.h"
#include "Utf8.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {
#ifndef _WIN32
class TerminalMode {
public:
    TerminalMode() {
        if (tcgetattr(STDIN_FILENO, &original_) != 0) {
            throw std::runtime_error("Không đọc được chế độ terminal.");
        }
        auto mode = original_;
        mode.c_lflag &= ~(ECHO | ICANON);
        mode.c_cc[VMIN] = 1;
        mode.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) != 0) {
            throw std::runtime_error("Không bật được chế độ che mật khẩu.");
        }
    }
    ~TerminalMode() { tcsetattr(STDIN_FILENO, TCSANOW, &original_); }

private:
    termios original_{};
};
#endif

template<class ReadKey>
std::string masked(ReadKey readKey) {
    std::string result;
    result.reserve(129);
    std::size_t overflow = 0;
    for (;;) {
        const int key = readKey();
        if (key < 0 || key == 4 || key == 26) {
            std::cout << '\n';
            throw EndOfInput{};
        }
        if (key == '\r' || key == '\n') {
            std::cout << '\n';
            if (overflow) {
                result.push_back('\x01');
            }
            return result;
        }
        if (key == 8 || key == 127) {
            if (overflow || !result.empty()) {
                if (overflow) {
                    --overflow;
                } else {
                    result.pop_back();
                }
                std::cout << "\b \b" << std::flush;
            }
        } else if (key >= 32) {
            if (result.size() == 128) {
                ++overflow;
            } else {
                result.push_back(key <= 126 ? static_cast<char>(key) : '\x01');
            }
            std::cout << '*' << std::flush;
        }
    }
}

int cellWidth(std::string_view value) {
    int width = 0;
    std::size_t offset = 0;
    while (offset < value.size()) {
        std::uint32_t code = 0;
        Utf8::next(value, offset, code);
        width += Utf8::width(code);
    }
    return width;
}

std::string fit(std::string_view value, int limit, bool center = false) {
    const bool shortened = cellWidth(value) > limit;
    const int dots = shortened ? std::min(3, limit) : 0;
    std::size_t offset = 0;
    std::size_t end = 0;
    int used = 0;
    while (offset < value.size()) {
        std::uint32_t code = 0;
        Utf8::next(value, offset, code);
        if (used + Utf8::width(code) > limit - dots) {
            break;
        }
        used += Utf8::width(code);
        end = offset;
    }
    const int padding = limit - used - dots;
    const int left = center ? padding / 2 : 0;
    return std::string(left, ' ') + std::string(value.substr(0, end))
        + std::string(dots, '.') + std::string(padding - left, ' ');
}

void rule(int width, std::string_view left, std::string_view right) {
    std::cout << left;
    for (int i = 0; i < width; ++i) {
        std::cout << "═";
    }
    std::cout << right << '\n';
}

std::string promptFor(std::string_view label, int columns) {
    return "  " + fit(label, std::clamp(columns - 5, 1, 22)) + ": ";
}

constexpr std::string_view LOGO[] = {
    "++====================================================================================++",
    "||  ▄█████▄ ▄▄ ▄▄  ▄▄▄  ▄▄  ▄▄   ▄▄  ▄▄ ▄▄   ▄▄  ▄▄ ▄▄ ▄▄  ▄▄▄  ▄▄  ▄▄    ▄▄▄▄ ▄▄ ▄▄  ||",
    "||  ██ ▄ ██ ██ ██ ██▀██ ███▄██   ██  ▀███▀   ███▄██ ██▄██ ██▀██ ███▄██   ███▄▄ ██ ██  ||",
    "||  ▀█████▀ ▀███▀ ██▀██ ██ ▀██   ██▄▄▄ █     ██ ▀██ ██ ██ ██▀██ ██ ▀██   ▄▄██▀ ▀███▀  ||",
    "||     ▀▀                                                                             ||",
    "++====================================================================================++",
};
constexpr int LOGO_WIDTH = 88;
}

ConsoleUI::ConsoleUI() {
#ifdef _WIN32
    input_ = GetStdHandle(STD_INPUT_HANDLE);
    output_ = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    interactiveInput_ = GetConsoleMode(input_, &mode) != 0;
    CONSOLE_SCREEN_BUFFER_INFO info{};
    interactiveOutput_ = GetConsoleScreenBufferInfo(output_, &info) != 0;
    if (interactiveOutput_) {
        originalColor_ = info.wAttributes;
        originalOutputEncoding_ = GetConsoleOutputCP();
        SetConsoleOutputCP(CP_UTF8);
        if (GetConsoleMode(output_, &originalOutputMode_)) {
            virtualTerminal_ = SetConsoleMode(output_,
                originalOutputMode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
        }
    }
#endif
}

ConsoleUI::~ConsoleUI() {
#ifdef _WIN32
    if (interactiveOutput_) {
        std::cout.flush();
        if (virtualTerminal_) {
            SetConsoleMode(output_, originalOutputMode_);
        }
        SetConsoleTextAttribute(output_, originalColor_);
        SetConsoleOutputCP(originalOutputEncoding_);
    }
#endif
}

bool ConsoleUI::interactive() const noexcept {
#ifdef _WIN32
    return interactiveInput_ && interactiveOutput_;
#else
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
#endif
}

int ConsoleUI::columns() const noexcept {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (interactiveOutput_ && GetConsoleScreenBufferInfo(output_, &info)) {
        return info.srWindow.Right - info.srWindow.Left + 1;
    }
#else
    winsize size{};
    if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col) {
        return size.ws_col;
    }
#endif
    return 100;
}

void ConsoleUI::color(Tone tone) const {
#ifdef _WIN32
    if (interactiveOutput_) {
        constexpr WORD PALETTE[] = {0, 9, 11, 15, 10, 14, 12};
        std::cout.flush();
        SetConsoleTextAttribute(output_, tone == Tone::Default ? originalColor_
            : (originalColor_ & 0xf0) | PALETTE[static_cast<int>(tone)]);
    }
#else
    if (isatty(STDOUT_FILENO)) {
        constexpr const char* PALETTE[] = {"0", "94", "96", "97", "92", "93", "91"};
        std::cout << "\x1b[" << PALETTE[static_cast<int>(tone)] << 'm';
    }
#endif
}

void ConsoleUI::clearScreen() const {
#ifdef _WIN32
    if (!interactiveOutput_) {
        std::cout << '\n';
        return;
    }
    if (!virtualTerminal_) {
        std::cout.flush();
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (GetConsoleScreenBufferInfo(output_, &info)) {
            const COORD origin{0, info.srWindow.Top};
            const DWORD cells = info.dwSize.X * (info.srWindow.Bottom - info.srWindow.Top + 1);
            DWORD written = 0;
            FillConsoleOutputCharacterW(output_, L' ', cells, origin, &written);
            FillConsoleOutputAttribute(output_, originalColor_, cells, origin, &written);
            SetConsoleCursorPosition(output_, origin);
        }
        return;
    }
#else
    if (!isatty(STDOUT_FILENO)) {
        std::cout << '\n';
        return;
    }
#endif
    std::cout << "\x1b[2J\x1b[H" << std::flush;
}

void ConsoleUI::mainMenu() const {
    clearScreen();
    const int available = columns() - 1;
    if (available >= LOGO_WIDTH) {
        const std::string margin((available + 1 - LOGO_WIDTH) / 2, ' ');
        color(Tone::Cyan);
        for (const auto row : LOGO) {
            std::cout << margin << row << '\n';
        }
    }
    color(Tone::Blue);
    if (available >= 25) {
        std::cout << fit("HỆ THỐNG QUẢN LÝ NHÂN SỰ", available, true) << '\n';
    } else if (available >= 16) {
        std::cout << fit("QUẢN LÝ NHÂN SỰ", available, true) << '\n';
    } else {
        const int width = std::max(1, available);
        std::cout << fit("QUẢN LÝ", width, true) << '\n'
                  << fit("NHÂN SỰ", width, true) << '\n';
    }
    std::cout << '\n';
    color(Tone::Default);
    menu("ĐĂNG NHẬP", {"[1] Quản trị viên", "[2] Nhân viên", "[0] Thoát chương trình"});
}

void ConsoleUI::page(std::string_view title) const {
    clearScreen();
    menu(title, {});
}

void ConsoleUI::menu(std::string_view title,
                     std::initializer_list<std::string_view> options) const {
    const int inner = std::max(1, std::min(52, columns() - 3));
    const std::string margin(std::max(0, (columns() - inner - 3) / 2), ' ');
    color(Tone::Cyan);
    std::cout << margin;
    rule(inner, "╔", "╗");
    std::cout << margin << "║" << fit(title, inner, true) << "║\n";
    if (options.size()) {
        std::cout << margin;
        rule(inner, "╠", "╣");
        for (const auto option : options) {
            std::cout << margin << "║";
            color(Tone::White);
            std::cout << fit("  " + std::string(option), inner);
            color(Tone::Cyan);
            std::cout << "║\n";
        }
    }
    std::cout << margin;
    rule(inner, "╚", "╝");
    color(Tone::Default);
    std::cout << '\n';
}

void ConsoleUI::pauseScreen() const {
    if (interactive()) {
        line("Nhấn Enter để quay lại");
    }
}

void ConsoleUI::status(Tone tone, std::string_view label, std::string_view value) const {
    color(tone);
    std::cout << '[' << label << "] " << value << '\n';
    color(Tone::Default);
}

void ConsoleUI::success(std::string_view value) const { status(Tone::Green, "THÀNH CÔNG", value); }
void ConsoleUI::error(std::string_view value) const { status(Tone::Red, "LỖI", value); }
void ConsoleUI::warning(std::string_view value) const { status(Tone::Yellow, "CẢNH BÁO", value); }

std::string ConsoleUI::line(std::string_view prompt) const {
    std::cout << promptFor(prompt, columns()) << std::flush;
#ifdef _WIN32
    if (interactiveInput_) {
        std::wstring wide;
        wchar_t buffer[256];
        DWORD count = 0;
        do {
            if (!ReadConsoleW(input_, buffer, 256, &count, nullptr) || count == 0) {
                throw EndOfInput{};
            }
            wide.append(buffer, count);
        } while (wide.back() != L'\n');
        if (wide.front() == L'\x1a') {
            throw EndOfInput{};
        }
        wide.pop_back();
        if (!wide.empty() && wide.back() == L'\r') {
            wide.pop_back();
        }
        if (wide.empty()) {
            return {};
        }
        const int length = static_cast<int>(wide.size());
        const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(),
                                              length, nullptr, 0, nullptr, nullptr);
        if (bytes <= 0) {
            throw std::invalid_argument("Dữ liệu Unicode không hợp lệ.");
        }
        std::string result(bytes, '\0');
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), length,
                            result.data(), bytes, nullptr, nullptr);
        return result;
    }
#endif
    std::string result;
    if (!std::getline(std::cin, result)) {
        throw EndOfInput{};
    }
    if (!result.empty() && result.back() == '\r') {
        result.pop_back();
    }
    return result;
}

std::string ConsoleUI::password(std::string_view prompt) const {
#ifdef _WIN32
    if (interactiveInput_) {
        std::cout << promptFor(prompt, columns()) << std::flush;
        WORD repeats = 0;
        int key = 0;
        return masked([&] {
            while (repeats == 0) {
                INPUT_RECORD event{};
                DWORD count = 0;
                if (!ReadConsoleInputW(input_, &event, 1, &count) || count == 0) {
                    return -1;
                }
                if (event.EventType == KEY_EVENT && event.Event.KeyEvent.bKeyDown
                    && event.Event.KeyEvent.uChar.UnicodeChar != 0) {
                    key = event.Event.KeyEvent.uChar.UnicodeChar;
                    repeats = event.Event.KeyEvent.wRepeatCount;
                }
            }
            --repeats;
            return key;
        });
    }
#else
    if (isatty(STDIN_FILENO)) {
        TerminalMode terminal;
        std::cout << promptFor(prompt, columns()) << std::flush;
        return masked([] { return std::cin.get(); });
    }
#endif
    auto result = line(prompt);
    std::cout << std::string(result.size() < 128 ? result.size() : 128, '*') << '\n';
    return result;
}

int ConsoleUI::choice(int minimum, int maximum) const {
    for (;;) {
        const auto input = line("Nhập lựa chọn");
        int value = 0;
        const auto end = input.data() + input.size();
        const auto parsed = std::from_chars(input.data(), end, value);
        if (parsed.ec == std::errc{} && parsed.ptr == end && value >= minimum && value <= maximum) {
            return value;
        }
        error("Lựa chọn không hợp lệ. Vui lòng nhập một số trong menu.");
    }
}

bool ConsoleUI::confirm(std::string_view prompt) const {
    for (;;) {
        color(Tone::Yellow);
        const auto result = line(prompt);
        color(Tone::Default);
        if (result == "c" || result == "C" || result == "y" || result == "Y") {
            return true;
        }
        if (result == "k" || result == "K" || result == "n" || result == "N" || result.empty()) {
            return false;
        }
        warning("Nhập Y để xác nhận hoặc N để hủy.");
    }
}


void ConsoleUI::employeeTable(const LinkedList<Employee>& employees) const {
    if (employees.isEmpty()) {
        warning("Danh sách nhân viên đang rỗng.");
        return;
    }
    const int width = std::max(8, std::min(110, columns() - 1));
    const std::array<std::string_view, 6> labels{
        "STT", "Username", "Họ và tên", "Địa chỉ", "Điện thoại", "Email"
    };
    std::array<int, 6> sizes{3, 8, 9, 7, 10, 5};
    const int minimum = 61;
    const bool compact = width < minimum;
    if (compact) {
        sizes[0] = std::min(12, (width - 3) / 2);
        sizes[1] = width - 3 - sizes[0];
    } else {
        constexpr int GROWTH[] = {5, 2, 3, 1, 4, 5, 2, 3, 5, 2, 3, 5};
        for (int extra = 0; extra < width - minimum; ++extra) {
            ++sizes[GROWTH[extra % std::size(GROWTH)]];
        }
    }
    const std::size_t count = compact ? 2 : sizes.size();
    const auto border = [&](std::string_view left, std::string_view middle, std::string_view right) {
        color(Tone::Cyan);
        std::cout << left;
        for (std::size_t i = 0; i < count; ++i) {
            for (int cell = 0; cell < sizes[i] + (compact ? 0 : 2); ++cell) {
                std::cout << "═";
            }
            std::cout << (i + 1 == count ? right : middle);
        }
        std::cout << '\n';
    };
    const auto row = [&](const auto& values, Tone textTone) {
        color(Tone::Cyan);
        std::cout << "║";
        for (std::size_t i = 0; i < count; ++i) {
            color(textTone);
            const auto space = compact ? "" : " ";
            std::cout << space << fit(values[i], sizes[i], !compact && i == 0) << space;
            color(Tone::Cyan);
            std::cout << "║";
        }
        std::cout << '\n';
    };

    message("Số nhân viên: " + std::to_string(employees.getSize()));
    border("╔", "╦", "╗");
    if (compact) {
        row(std::array<std::string_view, 2>{"Trường", "Thông tin"}, Tone::Cyan);
    } else {
        row(labels, Tone::Cyan);
    }
    std::size_t number = 0;
    employees.traverse([&](const Employee& employee) {
        border("╠", "╬", "╣");
        const auto ordinal = std::to_string(++number);
        const std::array<std::string_view, 6> values{ordinal, employee.username(), employee.name(),
            employee.address(), employee.phone(), employee.email()};
        if (compact) {
            for (std::size_t i = 0; i < labels.size(); ++i) {
                row(std::array<std::string_view, 2>{labels[i], values[i]}, Tone::White);
            }
        } else {
            row(values, Tone::White);
        }
    });
    border("╚", "╩", "╝");
    color(Tone::Default);
}

void ConsoleUI::show(const Employee& employee) const {
    std::cout << '\n';
    const int width = std::max(2, std::min(68, columns() - 5));
    const auto row = [&](std::string_view value, bool title = false) {
        color(Tone::Cyan);
        std::cout << "║ ";
        color(title ? Tone::Cyan : Tone::White);
        std::cout << fit(value, width, title);
        color(Tone::Cyan);
        std::cout << " ║\n";
    };

    color(Tone::Cyan);
    rule(width + 2, "╔", "╗");
    row("THÔNG TIN NHÂN VIÊN", true);
    rule(width + 2, "╠", "╣");

    std::ostringstream profile;
    profile << employee;
    std::istringstream lines(profile.str());
    std::string line;
    while (std::getline(lines, line)) {
        const std::string_view value(line);
        std::size_t start = 0;
        std::size_t offset = 0;
        int used = 0;
        while (offset < value.size()) {
            const auto previous = offset;
            std::uint32_t code = 0;
            Utf8::next(value, offset, code);
            const int cells = Utf8::width(code);
            if (used + cells > width) {
                row(value.substr(start, previous - start));
                start = previous;
                used = 0;
            }
            used += cells;
        }
        row(value.substr(start));
    }
    rule(width + 2, "╚", "╝");
    color(Tone::Default);
}

void ConsoleUI::message(std::string_view message) const {
    std::cout << message << '\n';
}
