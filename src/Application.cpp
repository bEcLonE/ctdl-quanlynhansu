#include "Application.h"

#include <optional>
#include <stdexcept>

namespace {
std::optional<ProfileField> selectField(const ConsoleUI& ui) {
    ui.clearScreen();
    ui.menu("CHỌN TRƯỜNG HỒ SƠ", {"[1] Họ tên", "[2] Địa chỉ", "[3] Số điện thoại",
                                    "[4] Email", "[0] Quay lại"});
    switch (ui.choice(0, 4)) {
    case 1: return ProfileField::Name;
    case 2: return ProfileField::Address;
    case 3: return ProfileField::Phone;
    case 4: return ProfileField::Email;
    default: return std::nullopt;
    }
}
}

Application::Application(ConsoleUI& ui, std::filesystem::path directory)
    : ui_(ui), files_(std::move(directory)), employees_(files_), auth_(files_, employees_) {}

void Application::run() {
    for (;;) {
        ui_.mainMenu();
        const auto option = ui_.choice(0, 2);
        if (option == 0) {
            return;
        }
        login(option == 1);
    }
}

void Application::login(bool administrator) {
    ui_.page(administrator ? "ĐĂNG NHẬP QUẢN TRỊ VIÊN" : "ĐĂNG NHẬP NHÂN VIÊN");
    ui_.message("Nhấn Enter ở Username để quay lại.");
    constexpr int MAX_ATTEMPTS = 3;
    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
        const auto username = ui_.line("Username");
        if (username.empty()) {
            return;
        }
        const auto password = ui_.password("Mật khẩu");
        if (administrator && auth_.authenticateAdmin(username, password)) {
            adminMenu();
            return;
        }
        const auto* employee = administrator ? nullptr : auth_.authenticateEmployee(username, password);
        if (employee) {
            if (employee->matchesPassword(Employee::DEFAULT_PASSWORD)) {
                ui_.page("ĐỔI MẬT KHẨU LẦN ĐẦU");
                ui_.warning("Đăng nhập lần đầu. Cần đổi mật khẩu trước khi vào menu nhân viên.");
                for (;;) {
                    try {
                        changePassword(employee->username());
                        ui_.pauseScreen();
                        break;
                    } catch (const std::runtime_error& error) {
                        ui_.error(error.what());
                    } catch (const std::invalid_argument& error) {
                        ui_.error(error.what());
                    }
                    if (!ui_.confirm("Thử lại (Y/N)")) {
                        return;
                    }
                }
            }
            employeeMenu(employee->username());
            return;
        }
        ui_.error("Sai username hoặc mật khẩu. Số lần còn lại: "
                    + std::to_string(MAX_ATTEMPTS - attempt));
    }
    ui_.warning("Đã sai 3 lần. Kết thúc phiên đăng nhập.");
    ui_.pauseScreen();
}

void Application::adminMenu() {
    for (;;) {
        ui_.clearScreen();
        ui_.menu("QUẢN TRỊ VIÊN",
                 {"[1] Thêm nhân viên", "[2] Xóa nhân viên", "[3] Tìm kiếm nhân viên",
                  "[4] Cập nhật nhân viên", "[5] Hiển thị danh sách", "[6] Đăng xuất",
                  "[7] Công cụ quản trị"});
        try {
            switch (ui_.choice(1, 7)) {
            case 1: addEmployee(); break;
            case 2: deleteEmployee(); break;
            case 3:
                ui_.page("TÌM KIẾM NHÂN VIÊN");
                showEmployee(employees_.findEmployee(ui_.line("Username cần tìm")));
                break;
            case 4: {
                ui_.page("CẬP NHẬT NHÂN VIÊN");
                const auto username = ui_.line("Username cần cập nhật");
                const auto* employee = employees_.findEmployee(username);
                if (!employee) {
                    throw std::invalid_argument("Không tìm thấy nhân viên.");
                }
                const auto field = selectField(ui_);
                if (!field) {
                    continue;
                }
                const auto value = ui_.line("Giá trị mới");
                logOperation("Cập nhật " + employee->username(), [&] {
                    employees_.update(username, *field, value);
                });
                ui_.success("Đã cập nhật hồ sơ.");
                break;
            }
            case 5: listEmployees(); break;
            case 6: return;
            case 7: operationsMenu(); continue;
            }
        } catch (const std::runtime_error& error) {
            ui_.error(error.what());
        } catch (const std::invalid_argument& error) {
            ui_.error(error.what());
        }
        ui_.pauseScreen();
    }
}

void Application::employeeMenu(std::string username) {
    for (;;) {
        ui_.clearScreen();
        ui_.menu("NHÂN VIÊN", {"[1] Xem thông tin cá nhân", "[2] Đổi mật khẩu", "[3] Đăng xuất"});
        try {
            switch (ui_.choice(1, 3)) {
            case 1:
                ui_.page("HỒ SƠ CÁ NHÂN");
                ui_.show(employees_.readProfile(username));
                break;
            case 2:
                ui_.page("ĐỔI MẬT KHẨU");
                changePassword(username);
                break;
            case 3: return;
            }
        } catch (const std::runtime_error& error) {
            ui_.error(error.what());
        } catch (const std::invalid_argument& error) {
            ui_.error(error.what());
        }
        ui_.pauseScreen();
    }
}

void Application::changePassword(std::string_view username) {
    const auto current = ui_.password("Mật khẩu hiện tại");
    const auto next = ui_.password("Mật khẩu mới");
    const auto confirmation = ui_.password("Nhập lại mật khẩu mới");
    auth_.changePassword(username, current, next, confirmation);
    ui_.success("Đã đổi mật khẩu.");
}

void Application::addEmployee() {
    ui_.page("THÊM NHÂN VIÊN");
    const auto username = ui_.line("Username");
    if (!Validation::username(username)) {
        throw std::invalid_argument("Username không hợp lệ. Dùng 1–48 chữ cái ASCII, số, _ hoặc -.");
    }
    if (employees_.findEmployee(username)) {
        throw std::invalid_argument("Username đã tồn tại.");
    }
    Employee employee(username, Employee::DEFAULT_PASSWORD);
    employee.setField(ProfileField::Name, ui_.line("Họ và tên"));
    employee.setField(ProfileField::Address, ui_.line("Địa chỉ"));
    employee.setField(ProfileField::Phone, ui_.line("Số điện thoại"));
    employee.setField(ProfileField::Email, ui_.line("Email"));
    logOperation("Thêm " + username, [&] { employees_.add(employee); });
    ui_.success("Đã thêm nhân viên.");
}

void Application::deleteEmployee() {
    ui_.page("XÓA NHÂN VIÊN");
    const auto username = ui_.line("Username cần xóa");
    const auto* employee = employees_.findEmployee(username);
    if (!employee) {
        throw std::invalid_argument("Không tìm thấy nhân viên.");
    }
    ui_.show(*employee);
    if (!ui_.confirm("Xác nhận xóa (Y/N)")) {
        ui_.warning("Đã hủy xóa.");
        return;
    }
    const auto canonicalUsername = employee->username();
    logOperation("Xóa " + canonicalUsername, [&] { employees_.remove(username); });
    updates_.removeMatching([&](const Update& update) { return update.username() == canonicalUsername; });
    ui_.success("Đã xóa nhân viên.");
}

void Application::showEmployee(const Employee* employee) const {
    if (employee) {
        ui_.show(*employee);
    } else {
        ui_.warning("Không tìm thấy nhân viên.");
    }
}

void Application::listEmployees() const {
    ui_.page("DANH SÁCH NHÂN VIÊN");
    ui_.employeeTable(employees_.employees());
}

void Application::operationsMenu() {
    for (;;) {
        ui_.clearScreen();
        ui_.menu("CÔNG CỤ QUẢN TRỊ",
                 {"[1] Sắp xếp danh sách theo họ tên", "[2] Tìm tuần tự theo username",
                  "[3] Xem thao tác quản trị gần nhất", "[4] Bỏ mục lịch sử gần nhất",
                  "[5] Đưa cập nhật vào hàng đợi", "[6] Xử lý cập nhật đầu hàng",
                  "[7] Hủy cập nhật đầu hàng", "[8] Duyệt chỉ mục AVL", "[0] Quay lại"});
        try {
            switch (ui_.choice(0, 8)) {
            case 0: return;
            case 1: employees_.sortByName(); listEmployees(); break;
            case 2:
                ui_.page("TÌM KIẾM TUẦN TỰ");
                showEmployee(employees_.linearSearch(ui_.line("Username cần tìm")));
                break;
            case 3: {
                ui_.page("LỊCH SỬ THAO TÁC");
                const auto* entry = history_.peek();
                ui_.message(entry ? *entry : "Lịch sử đang rỗng.");
                break;
            }
            case 4:
                ui_.page("BỎ MỤC LỊCH SỬ");
                if (history_.pop()) {
                    ui_.success("Đã bỏ mục lịch sử.");
                } else {
                    ui_.warning("Lịch sử đang rỗng.");
                }
                break;
            case 5:
                if (!enqueueUpdate()) {
                    continue;
                }
                break;
            case 6: processUpdate(); break;
            case 7:
                ui_.page("HỦY CẬP NHẬT ĐẦU HÀNG");
                if (updates_.dequeue()) {
                    ui_.success("Đã hủy cập nhật đầu hàng.");
                } else {
                    ui_.warning("Hàng đợi đang rỗng.");
                }
                break;
            case 8: indexMenu(); continue;
            }
        } catch (const std::runtime_error& error) {
            ui_.error(error.what());
        } catch (const std::invalid_argument& error) {
            ui_.error(error.what());
        }
        ui_.pauseScreen();
    }
}

bool Application::enqueueUpdate() {
    ui_.page("ĐƯA CẬP NHẬT VÀO HÀNG ĐỢI");
    const auto* employee = employees_.findEmployee(ui_.line("Username cần cập nhật"));
    if (!employee) {
        throw std::invalid_argument("Không tìm thấy nhân viên.");
    }
    const auto field = selectField(ui_);
    if (!field) {
        return false;
    }
    const auto value = ui_.line("Giá trị mới");
    auto candidate = *employee;
    candidate.setField(*field, value);
    updates_.enqueue(Update(employee->username(), *field, value));
    ui_.success("Đã đưa cập nhật vào hàng đợi.");
    return true;
}

void Application::processUpdate() {
    ui_.page("XỬ LÝ CẬP NHẬT ĐẦU HÀNG");
    const auto* update = updates_.front();
    if (!update) {
        ui_.warning("Hàng đợi đang rỗng.");
        return;
    }
    logOperation("Cập nhật từ hàng đợi " + update->username(), [&] {
        employees_.update(update->username(), update->field(), update->value());
    });
    updates_.dequeue();
    ui_.success("Đã lưu cập nhật đầu hàng.");
}

void Application::indexMenu() const {
    ui_.clearScreen();
    ui_.menu("DUYỆT CHỈ MỤC AVL", {"[1] Inorder", "[2] Preorder", "[3] Postorder", "[0] Quay lại"});
    const auto visit = [&](const std::string& key, const Employee*) { ui_.message(key); };
    switch (ui_.choice(0, 3)) {
    case 1:
        ui_.page("AVL · INORDER");
        employees_.index().inorder(visit);
        break;
    case 2:
        ui_.page("AVL · PREORDER");
        employees_.index().preorder(visit);
        break;
    case 3:
        ui_.page("AVL · POSTORDER");
        employees_.index().postorder(visit);
        break;
    default: return;
    }
    ui_.pauseScreen();
}
