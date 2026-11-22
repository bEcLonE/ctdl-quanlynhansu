#ifndef UTF8_H
#define UTF8_H

#include <cstdint>
#include <string_view>

class Utf8 {
public:
    static bool next(std::string_view value, std::size_t& offset, std::uint32_t& code) noexcept {
        if (offset >= value.size()) {
            return false;
        }
        const auto first = static_cast<unsigned char>(value[offset++]);
        if (first < 0x80) {
            code = first;
            return true;
        }
        if (first < 0xc2 || first > 0xf4) {
            return false;
        }
        const int count = first < 0xe0 ? 1 : first < 0xf0 ? 2 : 3;
        code = first & (0x7f >> (count + 1));
        for (int i = 0; i < count; ++i) {
            if (offset >= value.size()) {
                return false;
            }
            const auto byte = static_cast<unsigned char>(value[offset++]);
            if ((byte & 0xc0) != 0x80) {
                return false;
            }
            code = (code << 6) | (byte & 0x3f);
        }
        return (count == 1 || code >= (count == 2 ? 0x800u : 0x10000u))
            && code <= 0x10ffff && (code < 0xd800 || code > 0xdfff);
    }

    static int width(std::uint32_t code) noexcept {
        if ((code >= 0x300 && code <= 0x36f) || (code >= 0xfe00 && code <= 0xfe0f)
            || code == 0x200d) {
            return 0;
        }
        if ((code >= 0x1100 && code <= 0x115f) || (code >= 0x2e80 && code <= 0xa4cf)
            || (code >= 0xac00 && code <= 0xd7a3) || (code >= 0xf900 && code <= 0xfaff)
            || (code >= 0xff01 && code <= 0xff60) || (code >= 0x1f300 && code <= 0x1faff)
            || (code >= 0x20000 && code <= 0x3fffd)) {
            return 2;
        }
        return 1;
    }
};

#endif
