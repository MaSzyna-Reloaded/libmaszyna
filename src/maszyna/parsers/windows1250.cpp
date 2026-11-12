#include "windows1250.hpp"
#include <cstddef>
#include <iterator>

namespace windows1250 {
    namespace {
        /// Unicode code points of the bytes 0x80-0xFF (U+FFFD for undefined bytes) - the table of
        /// Windows1250.gd
        constexpr char32_t HIGH[] = {
                0x20AC, 0xFFFD, 0x201A, 0xFFFD, 0x201E, 0x2026, 0x2020, 0x2021, 0xFFFD, 0x2030, 0x0160, 0x2039, 0x015A,
                0x0164, 0x017D, 0x0179, 0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0xFFFD, 0x2122,
                0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A, 0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6,
                0x00A7, 0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B, 0x00B0, 0x00B1, 0x02DB, 0x0142,
                0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C, 0x0154,
                0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7, 0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD,
                0x00CE, 0x010E, 0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7, 0x0158, 0x016E, 0x00DA,
                0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF, 0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
                0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F, 0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4,
                0x0151, 0x00F6, 0x00F7, 0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9,
        };
        /// What a character cp1250 has no byte for is written as
        constexpr char UNMAPPED = '?';

        /// A UTF-8 sequence of more than one byte: the lead byte's marker under its mask, and the length
        struct Lead {
                uint8_t mask;
                uint8_t marker;
                size_t length;
        };
        constexpr Lead LEADS[] = {{0xE0, 0xC0, 2}, {0xF0, 0xE0, 3}, {0xF8, 0xF0, 4}};
        /// A continuation byte: its marker under its mask, and the bits of the code point it carries
        constexpr uint8_t CONTINUATION_MASK = 0xC0;
        constexpr uint8_t CONTINUATION = 0x80;
        constexpr uint8_t CONTINUATION_BITS = 6;
        constexpr uint8_t CONTINUATION_PAYLOAD = 0x3F;
    } // namespace

    /// The cp1250 byte of a code point, UNMAPPED when it has none
    static char encode_code_point(const char32_t p_code_point) {
        if (p_code_point < ASCII_END) {
            return static_cast<char>(p_code_point);
        }
        for (size_t i = 0; i < std::size(HIGH); i++) {
            if (HIGH[i] == p_code_point) {
                return static_cast<char>(ASCII_END + i);
            }
        }
        return UNMAPPED;
    }

    char32_t decode_byte(const uint8_t p_byte) {
        return p_byte < ASCII_END ? static_cast<char32_t>(p_byte) : HIGH[p_byte - ASCII_END];
    }

    std::string encode(const std::string &p_utf8) {
        std::string result;
        result.reserve(p_utf8.size());
        size_t i = 0;
        while (i < p_utf8.size()) {
            const auto byte = static_cast<uint8_t>(p_utf8[i]);
            if (byte < ASCII_END) {
                result += static_cast<char>(byte);
                i++;
                continue;
            }
            size_t length = 0;
            char32_t code_point = 0;
            for (const Lead &lead: LEADS) {
                if ((byte & lead.mask) == lead.marker) {
                    length = lead.length;
                    code_point = byte & static_cast<uint8_t>(~lead.mask);
                }
            }
            bool valid = length > 0 && i + length <= p_utf8.size();
            for (size_t j = 1; valid && j < length; j++) {
                const auto continuation = static_cast<uint8_t>(p_utf8[i + j]);
                valid = (continuation & CONTINUATION_MASK) == CONTINUATION;
                code_point = (code_point << CONTINUATION_BITS) | (continuation & CONTINUATION_PAYLOAD);
            }
            result += valid ? encode_code_point(code_point) : UNMAPPED;
            i += valid ? length : 1;
        }
        return result;
    }
} // namespace windows1250
