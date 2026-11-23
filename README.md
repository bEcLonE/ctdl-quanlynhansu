# Project 2 – Hệ thống quản lý nhân sự

Ứng dụng console C++17 cho môn Cấu trúc dữ liệu và Giải thuật. Cần Windows, MinGW-w64 và `g++` trên `PATH`. Python 3 dành cho kiểm thử console.

## Biên dịch và chạy

Mở PowerShell tại thư mục dự án.

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude src/main.cpp src/Application.cpp src/ConsoleUI.cpp src/Personnel.cpp src/FileManager.cpp src/Services.cpp -o personnel.exe
.\personnel.exe
```

Chương trình mở thư mục `data` theo mặc định. Có thể truyền thư mục dữ liệu khác, chẳng hạn `.\personnel.exe ban-sao-data`. Mỗi thư mục cần tệp tài khoản `Administrators.txt`, `Employees.txt` và một hồ sơ `[username].txt` cho mỗi nhân viên. Hồ sơ gồm bốn dòng UTF-8 theo thứ tự họ tên, địa chỉ, số điện thoại và email.

Trên Windows Terminal, mở cửa sổ rộng từ 89 cột để xem logo chữ khối. Cửa sổ hẹp hơn hiển thị tiêu đề chữ thường. Bảng danh sách chuyển sang bố cục hai cột khi cửa sổ có dưới 62 cột. Thông tin từng nhân viên hiển thị trong khung riêng, với nội dung dài tự xuống dòng. Sau khi xem kết quả thao tác, nhấn Enter để quay lại menu.

## Tài khoản demo

| Vai trò | Username | Mật khẩu ban đầu |
| --- | --- | --- |
| Quản trị | `admin1` | `DemoAdmin1` |
| Quản trị | `admin2` | `DemoAdmin2` |
| Quản trị | `admin3` | `DemoAdmin3` |
| Nhân viên | `demo` | `111111` |

Mật khẩu của `demo` thay đổi sau lần đăng nhập đầu tiên thành công. Tài khoản được lưu dạng văn bản theo định dạng bài tập.

## Chức năng và cấu trúc dữ liệu

- Quản trị viên thêm, tìm, sửa từng trường, xem danh sách và xác nhận xóa nhân viên.
- Nhân viên xem hồ sơ của mình và đổi mật khẩu. Mật khẩu được che khi nhập. Sau ba lần sai, chương trình thoát chương trình khi đăng nhập nhân viên.
- `LinkedList<Employee>` lưu danh sách nhân viên. Linear Search duyệt danh sách, Merge Sort nối lại các node để sắp xếp theo họ tên.
- AVL lập chỉ mục username. Linear Search, Merge Sort, Stack và Queue được kiểm thử trong bộ test C++.

## Mã nguồn

- `src/` có chương trình, menu, giao diện, mô hình dữ liệu, nghiệp vụ và lưu tệp.
- `include/` có khai báo và các cấu trúc dữ liệu template.
- `data/` chứa dữ liệu sử dụng. `tests/` có dữ liệu kiểm thử tự tạo.
- [AUDIT.md](AUDIT.md) ghi kết quả rà soát, kiểm thử và giới hạn. [REPORT.md](REPORT.md) có nội dung báo cáo Word và kịch bản video.
