#pragma once

// cp1250, the encoding of the original's data files, for the code that reads them (MaszynaParser) and
// for maszyna-python-host, which gives Python 2.7 its strings in it as the original does - without
// Godot, which the host has none of
#include <cstdint>
#include <string>

namespace windows1250 {
    /// The first byte past ASCII
    constexpr uint8_t ASCII_END = 0x80;

    /// The Unicode code point of a cp1250 byte, U+FFFD for an undefined one
    char32_t decode_byte(uint8_t p_byte);

    /// A UTF-8 text in cp1250; a character cp1250 has no byte for, and each byte of a broken
    /// sequence, is written as '?'
    std::string encode(const std::string &p_utf8);
} // namespace windows1250
