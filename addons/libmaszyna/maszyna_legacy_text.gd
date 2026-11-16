@tool
extends RefCounted
class_name MaszynaLegacyText

## Text of the original's data, whose files are written in cp1250, in UTF-8, or mixed: a valid UTF-8
## sequence throughout is UTF-8, anything else is cp1250 - handed on as Unicode either way

## UTF-8: the top two bits of a byte, what they are on a continuation byte, and the lead bytes of
## a sequence of two, three and four bytes
const UTF8_CONTINUATION_MASK:int = 0xC0
const UTF8_CONTINUATION:int = 0x80
const UTF8_LEAD_TWO:int = 0xC0
const UTF8_LEAD_THREE:int = 0xE0
const UTF8_LEAD_FOUR:int = 0xF0


static func decode(bytes:PackedByteArray) -> String:
    var utf8:bool = true
    var continuation_bytes:int = 0
    for byte:int in bytes:
        if continuation_bytes > 0:
            utf8 = (byte & UTF8_CONTINUATION_MASK) == UTF8_CONTINUATION
            continuation_bytes -= 1
        elif byte >= UTF8_LEAD_FOUR:
            continuation_bytes = 3
        elif byte >= UTF8_LEAD_THREE:
            continuation_bytes = 2
        elif byte >= UTF8_LEAD_TWO:
            continuation_bytes = 1
        else:
            utf8 = byte < UTF8_CONTINUATION
        if not utf8:
            break
    return bytes.get_string_from_utf8() if utf8 and continuation_bytes == 0 else Windows1250.decode(bytes)
