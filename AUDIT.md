# Audit Project 2

Đối chiếu yêu cầu đính kèm và đặc tả Project 2 với mã C++17, tài liệu, cấu hình CMake và test. Trước khi sửa, CTest đạt 1/2. Ca lõi đạt. Ca console thất bại do sao chép danh sách tài khoản đang dùng nhưng chỉ sao chép hồ sơ mẫu.

| Yêu cầu | Tệp triển khai | Kiểm chứng | Vấn đề phát hiện và hướng chỉnh sửa |
| --- | --- | --- | --- |
| Quản trị viên đăng nhập, thêm, kiểm tra trùng, tìm, sửa từng trường, xem và xóa có xác nhận | `src/Application.cpp`, `src/Services.cpp`, `src/Personnel.cpp` | Test lõi và console | Dùng exception để biểu diễn việc chọn Quay lại. Thay bằng giá trị tùy chọn ở hai luồng cập nhật. |
| Nhân viên đăng nhập đúng vai trò, giới hạn ba lần, che mật khẩu, xem đúng hồ sơ và đổi mật khẩu bắt buộc | `src/Application.cpp`, `src/Services.cpp`, `src/ConsoleUI.cpp` | Test lõi, console và Windows ConPTY | Bộ test phụ thuộc vào mật khẩu trong thư mục dữ liệu thật. Tạo tài khoản và hồ sơ cố định trong thư mục tạm. |
| Kiểm tra username, hồ sơ và định dạng tệp | `src/Personnel.cpp`, `src/FileManager.cpp` | Test lõi và đọc mã | Kiểm tra dữ liệu ở điểm đọc và ghi nghiệp vụ. Giữ quy tắc hiện hành. |
| Lưu hồ sơ, khởi động lại và xử lý lỗi khi thay đổi hai tệp | `src/FileManager.cpp`, `src/Services.cpp` | Test khởi động lại và khóa tệp đích trên Windows | Nhật ký nhiều giai đoạn khó giải thích. Ghi mỗi bản mới vào tệp tạm. Khi lỗi ở tệp tài khoản, bỏ hồ sơ vừa tạo hoặc đổi tên bản sao hồ sơ về chỗ cũ. Giữ khóa thư mục để tránh hai tiến trình ghi đè từ hai danh sách RAM khác nhau. |
| Linear Search, Merge Sort và Linked List sở hữu dữ liệu | `include/LinkedList.h`, `src/Services.cpp` | Test sao chép sâu, lỗi cấp phát, sắp xếp, địa chỉ node ổn định | Comparator ném lỗi giữa lúc nối node có thể làm mất đường dẫn tới một phần danh sách. Bắt buộc comparator `noexcept` ngay khi biên dịch. |
| Stack, Queue và AVL | `include/Stack.h`, `include/Queue.h`, `include/AvlTree.h`, `src/Application.cpp`, `src/Services.cpp` | Kiểm thử LIFO, FIFO và các phép AVL | AVL phục vụ tìm kiếm theo username. Stack và Queue nằm trong bộ kiểm thử sau khi bỏ menu phụ. |
| OOP, template, toán tử luồng và Coding Standard | `include/Personnel.h`, `src/Personnel.cpp`, các header template | Đọc mã và biên dịch C++17 | Các lớp dữ liệu dùng sao chép và hủy mặc định cho chuỗi. Linked List có Rule of Three, AVL dùng `unique_ptr`. Toán tử luồng áp dụng cho hồ sơ. Mã ứng dụng dùng `class` và template, định dạng K&R, không dùng container STL thay danh sách hay cây. |
| Console tiếng Việt, màu, che mật khẩu và căn lề | `src/ConsoleUI.cpp`, `include/Utf8.h` | Console chuyển hướng và Windows ConPTY | Giữ cách tính cột UTF-8 và nhập Unicode vì hai nhánh phục vụ các chế độ chạy khác nhau. |

## Chạy kiểm thử

```powershell
cmake --build build-cmake
ctest --test-dir build-cmake --output-on-failure
python tests/windows_console_test.py build-cmake/personnel.exe
```

Ca lõi tạo lỗi bằng cách khóa `Employees.txt` trên Windows và kiểm tra RAM cùng dữ liệu đọc lại sau khi mở chương trình lần nữa. Ca console và ConPTY tự tạo dữ liệu trong thư mục tạm để có thể chạy lại sau khi dữ liệu demo đã thay đổi.

Sau chỉnh sửa, CTest đạt **2/2** và ca Windows ConPTY đạt. Lệnh biên dịch trực tiếp trong README với `-std=c++17 -Wall -Wextra -Wpedantic -Werror` cũng thành công. Ca console đạt trên bản biên dịch trực tiếp.

## Giới hạn lưu tệp

Thêm và xóa là hai lần thay đổi tệp. Chúng có thể hoàn tác khi lỗi I/O được trả về trong cùng lần chạy. Nếu hoàn tác cũng thất bại, ứng dụng dừng và giữ bản sao hồ sơ nếu có. Sau khi tài khoản đã được xóa thành công, lỗi dọn dẹp có thể để lại tệp `.bak`.

Nếu tiến trình bị đóng cưỡng bức giữa hai lần sửa tệp, cần kiểm tra `Employees.txt` cùng các tệp `.tmp`, `.bak` và hồ sơ tương ứng trước khi tiếp tục. Ví dụ, nếu tài khoản vẫn còn nhưng hồ sơ đã được chuyển thành `[username].txt.bak`, đổi tên bản sao về `[username].txt` khi ứng dụng đã đóng. Nếu gặp thư mục `.transaction` của phiên bản trước, bản mới dừng và giữ nguyên thư mục để bản trước thực hiện phục hồi.

Merge Sort dùng thứ tự chuỗi UTF-8, chưa dùng quy tắc xếp tên tiếng Việt theo locale. Kết quả kiểm tra vòng đời giới hạn trong các ca đã chạy. Chưa chạy trình phát hiện rò rỉ bộ nhớ chuyên dụng.
