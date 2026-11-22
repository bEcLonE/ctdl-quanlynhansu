#include "Personnel.h"
#include "Queue.h"
#include "Stack.h"
#include "Utf8.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {
void check(bool condition, const char* description) {
    if (!condition) {
        throw std::runtime_error(description);
    }
}

template<class Action>
void expectFailure(Action action, const char* description) {
    try {
        action();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(description);
}

class Counted {
public:
    Counted() { ++live_; }
    Counted(const Counted&) {
        if (copiesBeforeFailure_ == 0) {
            throw std::bad_alloc{};
        }
        --copiesBeforeFailure_;
        ++live_;
    }
    ~Counted() { --live_; }
    static int live_;
    static int copiesBeforeFailure_;
};

int Counted::live_ = 0;
int Counted::copiesBeforeFailure_ = 10;

class TestDir {
public:
    TestDir()
        : path(fs::temp_directory_path() / ("ctdl-unit-" + std::to_string(
              std::chrono::steady_clock::now().time_since_epoch().count()))) {
        check(fs::create_directory(path), "create fixture");
    }
    ~TestDir() {
        std::error_code ignored;
        fs::remove_all(path, ignored);
    }
    fs::path path;
};

void write(const fs::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    file << content;
    file.close();
    check(static_cast<bool>(file), "write fixture");
}

Employee person(const std::string& username, const std::string& name) {
    Employee employee(username, Employee::DEFAULT_PASSWORD);
    employee.setField(ProfileField::Name, name);
    employee.setField(ProfileField::Address, "Hà Nội");
    employee.setField(ProfileField::Phone, "0901234567");
    employee.setField(ProfileField::Email, "hr@example.com");
    return employee;
}

void listAndAdapters() {
    LinkedList<int> list;
    list.insertTail(3);
    auto& stable = list.insertHead(2);
    list.insertTail(1);
    LinkedList<int> copy(list);
    LinkedList<int> assigned;
    assigned = list;
    const auto& alias = assigned;
    assigned = alias;
    list.sort([](int a, int b) noexcept { return a < b; });
    check(*list.front() == 1 && *copy.front() == 2 && *assigned.front() == 2,
          "deep copies and sort");
    check(list.find([](int value) { return value == 2; }) == &stable, "sort preserves nodes");
    check(list.removeByKey([](int value) { return value == 3; }), "remove tail");
    list.insertTail(4);
    int order = 0;
    list.traverse([&](int value) { order = order * 10 + value; });
    check(order == 124 && list.getSize() == 3, "tail maintained");
    list.clear();
    check(list.isEmpty() && !list.removeHead(), "empty operations");

    Stack<int> stack;
    Queue<int> queue;
    for (int value : {1, 2, 3}) {
        stack.push(value);
        queue.enqueue(value);
    }
    check(*stack.peek() == 3 && stack.pop() && *stack.peek() == 2, "LIFO");
    queue.removeMatching([](int value) { return value == 2; });
    check(*queue.front() == 1 && queue.dequeue() && *queue.front() == 3, "FIFO");
    stack.clear();
    queue.clear();
    check(stack.isEmpty() && queue.isEmpty() && !stack.pop() && !queue.dequeue(), "empty adapters");

    {
        Counted item;
        LinkedList<Counted> source;
        source.insertTail(item);
        source.insertTail(item);
        LinkedList<Counted> destination;
        destination.insertTail(item);
        Counted::copiesBeforeFailure_ = 1;
        expectFailure([&] { LinkedList<Counted> failed(source); }, "copy should throw");
        check(Counted::live_ == 4, "copy constructor cleans partial allocation");
        Counted::copiesBeforeFailure_ = 1;
        expectFailure([&] { destination = source; }, "assignment should throw");
        check(Counted::live_ == 4 && destination.getSize() == 1, "strong copy assignment");
    }
    check(Counted::live_ == 0, "all tracked values destroyed");
}

void treeAndUnicode() {
    AvlTree<int> tree;
    for (const auto* sequence : {"321", "123", "312", "132"}) {
        tree.clear();
        for (const char ch : std::string_view(sequence)) {
            check(tree.insert(std::string(1, ch), ch - '0'), "unique AVL insert");
        }
        check(!tree.insert("2", 9) && *tree.search("2") == 2, "duplicate AVL insert");
        std::string pre;
        std::string in;
        std::string post;
        tree.preorder([&](const auto& key, int) { pre += key; });
        tree.inorder([&](const auto& key, int) { in += key; });
        tree.postorder([&](const auto& key, int) { post += key; });
        check(pre == "213" && in == "123" && post == "132", "AVL rotation and traversal");
        check(tree.remove("2") && !tree.remove("2") && tree.search("1") && tree.search("3"),
              "remove node with two children");
    }
    tree.clear();
    for (int i = 1000; i < 1500; ++i) {
        tree.insert(std::to_string(i), i);
    }
    for (int i = 1000; i < 1500; i += 2) {
        check(tree.remove(std::to_string(i)), "remove from large AVL");
    }
    int expected = 1001;
    tree.inorder([&](const auto&, int value) {
        check(value == expected, "AVL order after deletions");
        expected += 2;
    });
    check(expected == 1501, "remaining AVL nodes");
    check(Validation::text("Nguyễn") && Validation::text("A\xcc\x81")
          && !Validation::text("\xc0\xaf"), "UTF-8 text validation");
    check(Utf8::width(0x1ec5) == 1 && Utf8::width(0x301) == 0
          && Utf8::width(0x754c) == 2, "Unicode column width");
    for (const auto* name : {"", "../name", "CON", "lPt2", "Employees", "Administrators"}) {
        check(!Validation::username(name), "reject invalid username");
    }
    check(!Validation::email("a@bad_.com") && !Validation::email("a@foo..com")
          && !Validation::phone("abcdefg"), "reject malformed contact details");
}

void services() {
    TestDir dir;
    const auto credentials = dir.path / "Employees.txt";
    write(dir.path / "Administrators.txt", "admin1 P1\r\nadmin2 P2\r\nadmin3 P3\r\n");
    write(credentials, "");
    {
        FileManager files(dir.path);
        expectFailure([&] { FileManager second(dir.path); }, "data lock must exclude writers");
        EmployeeService employees(files);
        AuthService auth(files, employees);
        check(auth.authenticateAdmin("ADMIN2", "P2") && !auth.authenticateAdmin("admin2", "P1"),
              "admin authentication");
        employees.add(person("lan", "Lan"));
        employees.add(person("anh", "Anh"));
        const auto* lan = employees.findEmployee("LAN");
        check(lan && lan == employees.linearSearch("lan"), "AVL and linear agree");
        expectFailure([&] { employees.add(person("LAN", "Khác")); }, "reject case duplicate");
        expectFailure([&] { employees.remove("missing"); }, "reject missing removal");
        employees.sortByName();
        check(employees.employees().front()->username() == "anh" && employees.findEmployee("lan") == lan,
              "list sort preserves AVL pointers");
        employees.update("lan", ProfileField::Name, "Nguyễn Lan");
        employees.update("lan", ProfileField::Address, "Đà Nẵng");
        employees.update("lan", ProfileField::Phone, "+84 123456789");
        employees.update("lan", ProfileField::Email, "lan@example.vn");
        const auto saved = employees.readProfile("lan");
        check(saved.name() == "Nguyễn Lan" && saved.address() == "Đà Nẵng"
              && saved.phone() == "+84 123456789" && saved.email() == "lan@example.vn", "saved fields");
        expectFailure([&] { employees.update("lan", ProfileField::Email, "bad"); }, "reject bad update");
        check(employees.readProfile("lan").email() == "lan@example.vn", "invalid update keeps file");
        check(auth.authenticateEmployee("lan", "111111") == lan
              && !auth.authenticateEmployee("lan", "wrong") && !auth.authenticateEmployee("admin2", "P2")
              && !auth.authenticateAdmin("lan", "111111"), "role and employee authentication");
        expectFailure([&] { auth.changePassword("lan", "bad", "First!", "First!"); }, "current required");
        expectFailure([&] { auth.changePassword("lan", "111111", "First!", "Other!"); }, "confirm required");
        auth.changePassword("lan", "111111", "First!", "First!");
        expectFailure([&] { auth.changePassword("lan", "First!", "111111", "111111"); }, "default reserved");
        auth.changePassword("lan", "First!", "Second!", "Second!");

        const auto pendingProfile = dir.path / "lan.txt.tmp";
        write(pendingProfile, "unfinished write");
        expectFailure([&] { employees.update("lan", ProfileField::Email, "new@example.vn"); },
                      "do not overwrite a pending profile");
        check(fs::exists(pendingProfile) && lan->email() == "lan@example.vn"
              && employees.readProfile("lan").email() == "lan@example.vn",
              "failed profile save preserves memory and both files");
        fs::remove(pendingProfile);

#ifdef _WIN32
        const HANDLE hold = CreateFileW(credentials.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        check(hold != INVALID_HANDLE_VALUE, "fixture lock on second target");
        try {
            expectFailure([&] { employees.add(person("blocked", "Blocked")); }, "second rename must fail");
            expectFailure([&] { employees.remove("lan"); }, "delete second rename must fail");
            expectFailure([&] { auth.changePassword("lan", "Second!", "Third!", "Third!"); },
                          "password rename must fail");
        } catch (...) {
            CloseHandle(hold);
            throw;
        }
        CloseHandle(hold);
        check(!fs::exists(dir.path / "blocked.txt") && !employees.findEmployee("blocked")
              && employees.findEmployee("lan") == lan && fs::exists(dir.path / "lan.txt")
              && auth.authenticateEmployee("lan", "Second!") == lan
              && !fs::exists(dir.path / "lan.txt.bak") && !fs::exists(dir.path / "Employees.txt.tmp"),
              "rollback restores both RAM indexes and profiles");
#endif
        employees.remove("anh");
        check(!employees.findEmployee("anh") && !employees.linearSearch("anh")
              && !fs::exists(dir.path / "anh.txt"), "delete updates list, index, and disk");
    }
    const auto journal = dir.path / ".transaction";
    fs::create_directory(journal);
    write(journal / "prepared", "");
    expectFailure([&] { FileManager files(dir.path); }, "old journal requires recovery using old version");
    check(fs::exists(journal / "prepared"), "old journal is preserved");
    fs::remove(journal / "prepared");
    fs::remove(journal);
    {
        FileManager files(dir.path);
        EmployeeService employees(files);
        AuthService auth(files, employees);
        check(auth.authenticateEmployee("lan", "Second!") && !auth.authenticateEmployee("lan", "111111"),
              "password survives restart");
        check(!employees.findEmployee("anh") && !employees.findEmployee("blocked")
              && employees.readProfile("lan").email() == "lan@example.vn",
              "restart sees saved data including the restored profile");
    }
    write(credentials, "lan 123456 extra\n");
    FileManager files(dir.path);
    expectFailure([&] { EmployeeService employees(files); }, "reject malformed credentials");
    write(credentials, "ghost 111111\n");
    expectFailure([&] { EmployeeService employees(files); }, "reject missing profile");
    write(dir.path / "ghost.txt", "");
    expectFailure([&] { EmployeeService employees(files); }, "reject empty profile");
    fs::remove(credentials);
    expectFailure([&] { EmployeeService employees(files); }, "reject missing credentials");
}
}

int main() {
    try {
        listAndAdapters();
        treeAndUnicode();
        services();
        std::cout << "Linked list, stack, queue, AVL, Unicode, CRUD, authentication and recovery passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
