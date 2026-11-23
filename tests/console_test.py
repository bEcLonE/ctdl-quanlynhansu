"""Run the public console entry point against disposable data."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unicodedata


def seed_data(directory):
    (directory / "Administrators.txt").write_text(
        "admin1 DemoAdmin1\nadmin2 DemoAdmin2\nadmin3 DemoAdmin3\n", encoding="utf-8"
    )
    (directory / "Employees.txt").write_text("demo 111111\n", encoding="utf-8")
    (directory / "demo.txt").write_text(
        "Nguyễn Minh Anh\n12 Nguyễn Huệ, Thành phố Hồ Chí Minh\n0901234567\ndemo@example.com\n",
        encoding="utf-8",
    )


def width(text):
    return sum(
        0 if unicodedata.combining(ch) else 2 if unicodedata.east_asian_width(ch) in "WF" else 1
        for ch in text
    )


def run(executable, data, lines):
    result = subprocess.run(
        [executable, str(data)],
        input="\n".join(lines) + "\n",
        encoding="utf-8",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=15,
        check=True,
    )
    frame_width = None
    for line in result.stdout.splitlines():
        row = line.lstrip()
        if row.startswith("╔"):
            frame_width = width(row)
        if row.startswith(("╔", "║", "╠", "╚")):
            assert width(row) == frame_width, repr(row)
            assert len(line) <= 300
    assert "\x1b" not in result.stdout
    return result.stdout


def main():
    executable = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="ctdl-console-") as temporary:
        data = Path(temporary)
        seed_data(data)

        output = run(executable, data, [
            "x", "9", "2", "demo", "bad", "demo", "bad", "demo", "bad",
            "1", "admin1", "DemoAdmin1",
        ])
        assert output.count("Lựa chọn không hợp lệ") == 2
        assert output.count("Sai username hoặc mật khẩu.") == 3
        assert "Đã sai 3 lần. Thoát chương trình." in output
        assert "ĐĂNG NHẬP QUẢN TRỊ VIÊN" not in output
        output = run(executable, data, [
            "2", "demo", "111111", "111111", "First!9", "DoNotMatch", "k", "0",
        ])
        assert "Mật khẩu xác nhận không khớp." in output
        assert all(row.strip(" ║") != "NHÂN VIÊN" for row in output.splitlines())
        assert "HỆ THỐNG QUẢN LÝ NHÂN SỰ" in output and "██" in output
        logo = [line.strip() for line in output.splitlines() if line.strip().startswith(("++", "||"))]
        assert logo and all(width(line) == 88 for line in logo)
        assert any(line.startswith("||  ▄█████▄ ▄▄ ▄▄") for line in logo)
        assert all(line.startswith(" " * 6 + line.lstrip()) for line in output.splitlines()
                   if line.lstrip().startswith(("++", "||")))
        assert "demo 111111" in (data / "Employees.txt").read_text()

        output = run(executable, data, [
            "2", "DEMO", "111111", "111111", "First!9", "First!9", "1",
            "2", "First!9", "Second!9", "Second!9", "1", "3", "0",
        ])
        assert output.count("Đã đổi mật khẩu.") == 2
        assert output.count("Nguyễn Minh Anh") == 2
        assert output.count("THÔNG TIN NHÂN VIÊN") == 2
        assert "Mật khẩu".ljust(22) + ": ******" in output
        assert all(secret not in output for secret in ("111111", "First!9", "Second!9"))
        output = run(executable, data, ["2", "demo", "Second!9", "1", "3", "0"])
        assert "Nguyễn Minh Anh" in output and "Đăng nhập lần đầu." not in output

        long_name = "A\u0301nh " + "界" * 40
        output = run(executable, data, [
            "1", "admin3", "DemoAdmin3",
            "7",
            "1", "CON",
            "1", "test_user", long_name, "Đà Nẵng", "0901234567", "initial@example.com",
            "1", "TEST_USER",
            "3", "test_user",
            "4", "test_user", "0",
            "4", "test_user", "2", "Hà Nội",
            "4", "test_user", "3", "0912345678",
            "4", "test_user", "4", "second@example.com",
            "5",
            "3", "test_user",
            "2", "test_user", "n",
            "6", "0",
        ])
        assert "Username không hợp lệ." in output and "Username đã tồn tại." in output
        assert "Đã thêm nhân viên." in output and "Số nhân viên: 2" in output
        table = [row for row in output.splitlines() if row.startswith("║") and row.count("║") == 7]
        assert any(all(label in row for label in ("STT", "Username", "Họ và tên", "Địa chỉ",
                                                  "Điện thoại", "Email")) for row in table)
        assert any("test_user" in row and "..." in row for row in table)
        assert any("demo" in row for row in table)
        boundaries = [width(table[0].split("║")[0])]
        for cell in table[0].split("║")[1:-1]:
            boundaries.append(boundaries[-1] + width(cell) + 1)
        for row in table:
            actual = [width(row[:i]) for i, char in enumerate(row) if char == "║"]
            assert actual == boundaries, repr(row)
        assert "Đã cập nhật hồ sơ." in output and "Hà Nội" in output
        assert "Lựa chọn không hợp lệ" in output and "Công cụ quản trị" not in output
        assert output.count("Đã cập nhật hồ sơ.") == 3
        assert "Đã hủy xóa." in output and "second@example.com" in output
        profile = [row for row in output.splitlines() if row.startswith("║") and row.count("║") == 2]
        assert any("THÔNG TIN NHÂN VIÊN" in row for row in profile)
        assert any("界界界" in row and "..." not in row for row in profile)
        assert (data / "test_user.txt").read_text(encoding="utf-8").splitlines() == [
            long_name, "Hà Nội", "0912345678", "second@example.com"
        ]
        output = run(executable, data, [
            "1", "admin1", "DemoAdmin1", "2", "test_user", "y", "3", "test_user", "6", "0",
        ])
        assert "Đã xóa nhân viên." in output and "Không tìm thấy nhân viên." in output
        assert not (data / "test_user.txt").exists()
        assert (data / "Employees.txt").read_text() == "demo Second!9\n"
    print("Admin and employee menus, password prompts, restart, and Unicode frame widths passed.")


if __name__ == "__main__":
    main()
