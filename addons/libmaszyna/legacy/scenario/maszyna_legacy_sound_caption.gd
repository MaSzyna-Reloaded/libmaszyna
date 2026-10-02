@tool
extends RefCounted
class_name MaszynaLegacySoundCaption

## A sound's transcript out of the original's caption file, `<sound>-<language>.txt` beside the
## sound (openal_buffer::fetch_caption(), audio.cpp:84-94): lines `[show][hide]text`, the times in
## the original's own units (TTranscripts::Add(), uitranscripts.cpp:37-60). Most files are in
## cp1250, a few in UTF-8.

const EXTENSION:String = ".txt"
## A caption line's show time, per unit of its first bracket (uitranscripts.cpp:51)
const SHOW_SECONDS_PER_UNIT:float = 0.10
## A caption line's hide time, per unit of its second bracket (uitranscripts.cpp:51)
const HIDE_SECONDS_PER_UNIT:float = 0.12
## A line without times is shown from the start, this long per character (uitranscripts.cpp:58)
const SECONDS_PER_CHARACTER:float = 0.12
## The marks a caption is split by (uitranscripts.cpp:43)
const SEPARATORS:PackedStringArray = ["[", "]", "\n"]
## A legacy caption's new line mark, shown as a space (uitranscripts.cpp:24)
const LINE_MARK:String = "|"
## UTF-8: the top two bits of a byte, what they are on a continuation byte, and the lead bytes of
## a sequence of two, three and four bytes
const UTF8_CONTINUATION_MASK:int = 0xC0
const UTF8_CONTINUATION:int = 0x80
const UTF8_LEAD_TWO:int = 0xC0
const UTF8_LEAD_THREE:int = 0xE0
const UTF8_LEAD_FOUR:int = 0xF0


## The transcript of the sound file (its full path, without its extension) in the simulation's
## language, beside it wherever it is - the sounds or a scenery - or null when it has none
static func from_sound_file(sound_path:String) -> Transcript:
    var base_dir:String = sound_path.get_base_dir()
    var filename:String = "%s-%s%s" % [sound_path.get_file(), MaszynaTranslationServer.language, EXTENSION]
    var path:String = base_dir.path_join(MaszynaDataPath.resolve(base_dir, filename))
    if not FileAccess.file_exists(path):
        return null
    var bytes:PackedByteArray = FileAccess.get_file_as_bytes(path)
    # a valid UTF-8 sequence throughout is UTF-8, anything else is cp1250
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
    return parse(bytes.get_string_from_utf8() if utf8 and continuation_bytes == 0 else Windows1250.decode(bytes))


## The transcript of a caption's text: `[show][hide]text` a line; whatever is left with no times
## is shown from the start for as long as it is long (TTranscripts::Add(), uitranscripts.cpp:37-60)
static func parse(caption:String) -> Transcript:
    var tokens:PackedStringArray = []
    var token:String = ""
    for character:String in caption + SEPARATORS[0]:
        if not character in SEPARATORS:
            token += character
            continue
        if token.strip_edges():
            tokens.append(token.strip_edges().replace(LINE_MARK, " "))
        token = ""
    var transcript:Transcript = Transcript.new()
    var index:int = 0
    while index + 2 < tokens.size() and tokens[index].is_valid_float() and tokens[index + 1].is_valid_float():
        transcript.add_line(
                tokens[index + 2],
                float(tokens[index]) * SHOW_SECONDS_PER_UNIT,
                float(tokens[index + 1]) * HIDE_SECONDS_PER_UNIT)
        index += 3
    for text:String in tokens.slice(index):
        transcript.add_line(text, 0.0, text.length() * SECONDS_PER_CHARACTER)
    return transcript
