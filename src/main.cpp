#include "Application.h"

#include <exception>
#include <new>

int main(int argc, char* argv[]) {
    ConsoleUI ui;
    try {
        if (argc > 2) {
            ui.message("Cách dùng: personnel [thư_mục_dữ_liệu]");
            return 1;
        }
        Application app(ui, argc == 2 ? std::filesystem::u8path(argv[1]) : "data");
        app.run();
    } catch (const EndOfInput&) {
        ui.message("\nĐã kết thúc nhập liệu.");
    } catch (const std::bad_alloc&) {
        ui.message("Không đủ bộ nhớ để tiếp tục.");
        return 1;
    } catch (const std::exception& error) {
        ui.message(error.what());
        return 1;
    }
}
