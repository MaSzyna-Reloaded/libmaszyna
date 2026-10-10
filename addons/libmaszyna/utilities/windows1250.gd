@tool
extends RefCounted
class_name Windows1250

## Text in cp1250, the encoding of the original's data files (sceneries, timetables...)

## The first byte past ASCII
const ASCII_END:int = 0x80
## Unicode code points of the bytes 0x80-0xFF (U+FFFD for undefined bytes)
const HIGH_CODE_POINTS:PackedInt32Array = [
    0x20AC, 0xFFFD, 0x201A, 0xFFFD, 0x201E, 0x2026, 0x2020, 0x2021,
    0xFFFD, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179,
    0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0xFFFD, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A,
    0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6, 0x00A7,
    0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B,
    0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
    0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C,
    0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
    0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
    0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
    0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
    0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
    0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
    0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
    0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9,
]
## Polish letters and their ASCII (win1250_to_ascii(), utilities.cpp:246-258)
const ASCII_LETTERS:Dictionary[String, String] = {
    "Ą": "A", "Ć": "C", "Ę": "E", "Ł": "L", "Ń": "N", "Ó": "O", "Ś": "S", "Ź": "Z", "Ż": "Z",
    "ą": "a", "ć": "c", "ę": "e", "ł": "l", "ń": "n", "ó": "o", "ś": "s", "ź": "z", "ż": "z",
}


static func decode(bytes:PackedByteArray) -> String:
    var text:String = ""
    for byte:int in bytes:
        text += String.chr(byte if byte < ASCII_END else HIGH_CODE_POINTS[byte - ASCII_END])
    return text


## The text back in cp1250 bytes - for a parser that reads cp1250 (MaszynaParser); a character
## cp1250 has no byte for becomes "?"
static func encode(text:String) -> PackedByteArray:
    var bytes:PackedByteArray = []
    for index:int in text.length():
        var code:int = text.unicode_at(index)
        if code < ASCII_END:
            bytes.append(code)
            continue
        var high:int = HIGH_CODE_POINTS.find(code)
        bytes.append(ASCII_END + high if high >= 0 else "?".unicode_at(0))
    return bytes


## The Polish letters of the text made plain ASCII, as the original compares names
static func to_ascii(text:String) -> String:
    var ascii:String = text
    for letter:String in ASCII_LETTERS:
        ascii = ascii.replace(letter, ASCII_LETTERS[letter])
    return ascii
