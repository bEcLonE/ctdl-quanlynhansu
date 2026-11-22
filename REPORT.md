# Nội dung báo cáo và video demo

## Nội dung báo cáo Word

**Mục tiêu.** Xây dựng ứng dụng console C++17 quản lý hồ sơ và tài khoản nhân viên, có quyền quản trị riêng, xác thực tối đa ba lần và đổi mật khẩu bắt buộc khi dùng mật khẩu mặc định. Dữ liệu được đọc khi khởi động và lưu lại sau mỗi thay đổi thành công.

**Kiến trúc.** `Application` nhận thao tác từ `ConsoleUI`, sau đó chuyển cho `AuthService` hoặc `EmployeeService`. `Administrator` và `Employee` kế thừa phần tài khoản chung từ `User`. `EmployeeService` sở hữu các bản ghi trong `LinkedList<Employee>`. Mỗi node AVL lưu một khóa username chuẩn hóa và trỏ đến bản ghi trong danh sách. `FileManager` kiểm tra và ghi tệp, đồng thời giữ khóa thư mục. Có thể dùng bảng đối chiếu trong AUDIT làm phụ lục.

**Luồng đăng nhập.** Người dùng chọn vai trò, nhập username và mật khẩu. Hệ thống chỉ đối chiếu trong nguồn tài khoản của vai trò đã chọn. Sau ba lần sai, hệ thống kết thúc lượt đăng nhập. Khi nhân viên còn dùng mật khẩu mặc định, bước đổi mật khẩu kiểm tra mật khẩu cũ và hai bản nhập của mật khẩu mới. Chỉ khi lưu thành công, chương trình mới cấp quyền vào menu nhân viên. Nhân viên xem đúng hồ sơ theo username của phiên.

**Cấu trúc và giải thuật.** Linear Search đi qua từng node để so sánh username. Merge Sort chia đôi danh sách bằng cặp con trỏ nhanh và chậm, đệ quy trên mỗi nửa rồi trộn ổn định bằng cách đổi liên kết. Địa chỉ bản ghi giữ nguyên khi sắp xếp, vì vậy các con trỏ của AVL tiếp tục hợp lệ. AVL giữ chiều cao từng node và quay sau thao tác làm thay đổi cây. Thêm nhân viên cập nhật cả danh sách và cây, đồng thời gỡ cả hai nếu ghi thất bại. Xóa nhân viên chỉ gỡ bản ghi khỏi RAM sau khi giao dịch tệp hoàn thành.

**Stack và Queue.** Stack lưu thao tác quản trị thành công, cho phép xem hoặc bỏ mục gần nhất. Queue lưu các cập nhật từng trường, nhận trước xử lý trước. Thao tác ở đầu Queue chỉ được lấy ra sau khi ghi thành công hoặc được hủy. Khi xóa nhân viên, chương trình loại các cập nhật đang chờ dành cho nhân viên đó.

**Lưu và hoàn tác.** Tệp hồ sơ mang tên username và lưu mỗi trường một dòng. Bản ghi mới được ghi vào tệp tạm trước khi đổi tên. Khi lỗi ghi danh sách tài khoản, thao tác thêm dọn hồ sơ mới và thao tác xóa đưa bản sao hồ sơ về chỗ cũ. Ca kiểm thử Windows khóa tệp tài khoản để tạo lỗi, sau đó kiểm tra cả hồ sơ và hai cấu trúc trong RAM.

## Kịch bản video khoảng 5 phút

| Thời lượng | Thao tác trên màn hình | Nội dung thuyết minh |
| --- | --- | --- |
| 0:00–0:35 | Mở thư mục demo, chỉ ba loại tệp rồi chạy ứng dụng. | Tóm tắt vai trò, định dạng hồ sơ và cách chạy. |
| 0:35–1:30 | Đăng nhập `admin1`, thêm `lan` với họ tên `Nguyễn Lan`, thử thêm `LAN`, tìm `lan`, sửa riêng địa chỉ, rồi hiển thị danh sách. | Chỉ ra kiểm tra trùng, AVL dùng để tìm và Linked List dùng để hiển thị. |
| 1:30–2:35 | Vào mục 7, sắp xếp, tìm tuần tự, xem rồi bỏ mục lịch sử. Đưa hai cập nhật email vào Queue và áp dụng từng mục. Duyệt inorder. | Giải thích Merge Sort nối lại node, Stack theo LIFO, Queue theo FIFO và khóa index AVL. |
| 2:35–3:35 | Đăng xuất quản trị. Thử đăng nhập sai ba lần. Đăng nhập `lan` bằng `111111`, nhập xác nhận sai một lần, sau đó đổi thành công và xem hồ sơ. | Cho thấy dấu sao, giới hạn số lần thử và điều kiện mở menu nhân viên. |
| 3:35–4:30 | Đổi mật khẩu lần nữa, đóng và chạy lại chương trình, đăng nhập bằng mật khẩu mới. Đăng nhập quản trị, hủy xóa một lần rồi xác nhận xóa `lan`. | Kiểm tra dữ liệu qua lần chạy mới, cập nhật tệp và kết quả tìm kiếm sau khi xóa. |
| 4:30–5:00 | Chạy test, mở phần kiểm thử phục hồi và sao chép danh sách. | Giải thích tình huống lỗi tệp thứ hai và việc đếm vòng đời đối tượng khi cấp phát thất bại. |
