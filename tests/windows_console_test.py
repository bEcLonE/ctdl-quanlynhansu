"""Exercise native console I/O through a headless Windows ConPTY."""

import ctypes as c
from ctypes import wintypes as w
from pathlib import Path
from queue import Queue, Empty
import os
import subprocess
import sys
import tempfile
from threading import Thread
import msvcrt
from console_test import seed_data

k = c.WinDLL("kernel32", use_last_error=True)


class Startup(c.Structure):
    _fields_ = [
        ("cb", w.DWORD), ("reserved", w.LPWSTR), ("desktop", w.LPWSTR), ("title", w.LPWSTR),
        ("coordinates", w.DWORD * 8), ("show", w.WORD), ("size", w.WORD),
        ("reserved2", c.c_void_p), ("handles", w.HANDLE * 3), ("attributes", c.c_void_p),
    ]


class Process(c.Structure):
    _fields_ = [("process", w.HANDLE), ("thread", w.HANDLE), ("pid", w.DWORD), ("tid", w.DWORD)]


def bind(name, arguments, result=w.BOOL):
    f = getattr(k, name)
    f.argtypes = arguments
    f.restype = result
    return f


create_console = bind("CreatePseudoConsole", [w.DWORD, w.HANDLE, w.HANDLE, w.DWORD, c.POINTER(w.HANDLE)], w.LONG)
close_console = bind("ClosePseudoConsole", [w.HANDLE], None)
init = bind("InitializeProcThreadAttributeList", [c.c_void_p, w.DWORD, w.DWORD, c.POINTER(c.c_size_t)])
update = bind("UpdateProcThreadAttribute", [c.c_void_p, w.DWORD, c.c_size_t, c.c_void_p, c.c_size_t, c.c_void_p, c.c_void_p])
delete = bind("DeleteProcThreadAttributeList", [c.c_void_p], None)
spawn = bind("CreateProcessW", [w.LPCWSTR, w.LPWSTR, c.c_void_p, c.c_void_p, w.BOOL,
                              w.DWORD, c.c_void_p, w.LPCWSTR, c.POINTER(Startup), c.POINTER(Process)])
wait = bind("WaitForSingleObject", [w.HANDLE, w.DWORD], w.DWORD)
terminate = bind("TerminateProcess", [w.HANDLE, w.UINT])
close = bind("CloseHandle", [w.HANDLE])
get_exit = bind("GetExitCodeProcess", [w.HANDLE, c.POINTER(w.DWORD)])
get_std = bind("GetStdHandle", [w.DWORD], w.HANDLE)
set_std = bind("SetStdHandle", [w.DWORD, w.HANDLE])


def check(value):
    if not value:
        raise c.WinError(c.get_last_error())


def exercise(executable, data, columns):
    input_read, input_write = os.pipe()
    output_read, output_write = os.pipe()
    hpc = w.HANDLE()
    status = create_console((35 << 16) | columns, msvcrt.get_osfhandle(input_read),
                            msvcrt.get_osfhandle(output_write), 0, c.byref(hpc))
    if status:
        raise RuntimeError(f"CreatePseudoConsole failed: {status:#x}")
    os.close(input_read)
    os.close(output_write)
    size = c.c_size_t()
    init(None, 1, 0, c.byref(size))
    attributes = c.create_string_buffer(size.value)
    check(init(attributes, 1, 0, c.byref(size)))
    process = Process()
    received = Queue()

    def drain():
        with os.fdopen(output_read, "rb", buffering=0) as stream:
            while chunk := stream.read(4096):
                received.put(chunk)

    reader = Thread(target=drain, daemon=True)
    reader.start()
    output = b""

    def until(value):
        nonlocal output
        try:
            while value.encode() not in output:
                output += received.get(timeout=5)
        except Empty:
            raise AssertionError((value, output.decode(errors="replace"))) from None
        before, _, output = output.partition(value.encode())
        return before

    try:
        check(update(attributes, 0, 0x00020016, hpc, c.sizeof(hpc), None, None))
        startup = Startup()
        startup.cb = c.sizeof(startup)
        startup.attributes = c.addressof(attributes)
        command = c.create_unicode_buffer(subprocess.list2cmdline([str(executable), str(data)]))
        std = {which: get_std(which) for which in (-10, -11, -12)}
        try:
            for which in std:
                check(set_std(which, None))
            check(spawn(str(executable), command, None, None, False, 0x00080000, None, None,
                        c.byref(startup), c.byref(process)))
        finally:
            for which, handle in std.items():
                set_std(which, handle)
        startup_screen = until("Nhập lựa chọn")
        assert "HỆ THỐNG QUẢN LÝ NHÂN SỰ".encode() in startup_screen
        assert ("██".encode() in startup_screen) == (columns == 100)
        os.write(input_write, b"1\r")
        login_screen = until("ĐĂNG NHẬP QUẢN TRỊ VIÊN")
        # ConPTY renders the cleared screen as a redraw from the home position.
        assert b"\x1b[H" in login_screen and "██".encode() not in login_screen, login_screen
        until("Username".ljust(22) + ":")
        os.write(input_write, b"admin1\r")
        until("Mật khẩu".ljust(22) + ":")
        os.write(input_write, b"DemoAdminX\x7f1\r")
        secret_output = until("QUẢN TRỊ VIÊN")
        assert b"DemoAdmin" not in secret_output, secret_output
        assert secret_output.count(b"*") >= 10, secret_output
        until("Nhập lựa chọn")
        os.write(input_write, b"1\r")
        until("Username".ljust(22) + ":")
        os.write(input_write, b"ptystaff\r")
        until("Họ và tên".ljust(22) + ":")
        os.write(input_write, "Nguyễn Thị Ánh\r".encode())
        until("Địa chỉ".ljust(22) + ":")
        os.write(input_write, "Đà Nẵng\r".encode())
        until("Số điện thoại".ljust(22) + ":")
        os.write(input_write, b"0901234567\r")
        until("Email".ljust(22) + ":")
        os.write(input_write, b"pty@example.com\r")
        until("Đã thêm nhân viên.")
        until("Nhấn Enter để quay lại")
        os.write(input_write, b"\r")
        until("Nhập lựa chọn")
        os.write(input_write, b"5\r")
        until("DANH SÁCH NHÂN VIÊN")
        if columns < 61:
            until("Thông tin")
        until("Nguyễn Thị Ánh")
        until("Nhấn Enter để quay lại")
        os.write(input_write, b"\r")
        until("Nhập lựa chọn")
        os.write(input_write, b"6\r")
        until("Nhập lựa chọn")
        os.write(input_write, b"0\r")
        assert wait(process.process, 5000) == 0
        code = w.DWORD()
        check(get_exit(process.process, c.byref(code)))
        assert code.value == 0
        assert (data / "ptystaff.txt").read_text(encoding="utf-8").splitlines()[:2] == [
            "Nguyễn Thị Ánh", "Đà Nẵng",
        ]
    finally:
        if process.process:
            if wait(process.process, 0) != 0:
                terminate(process.process, 1)
                wait(process.process, 5000)
            close(process.thread)
            close(process.process)
        delete(attributes)
        os.close(input_write)
        close_console(hpc)
        reader.join(timeout=5)


def main():
    executable = Path(sys.argv[1]).resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="ctdl-pty-") as temporary:
        data = Path(temporary)
        for columns in (100, 58):
            fixture = data / str(columns)
            fixture.mkdir()
            seed_data(fixture)
            exercise(executable, fixture, columns)
    print("Windows ConPTY banner, clear, prompts, masking, pause and tables passed at 100 and 58 columns.")


if __name__ == "__main__":
    main()
