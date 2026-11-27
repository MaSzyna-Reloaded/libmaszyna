@tool
extends RefCounted
class_name MaszynaSoundCaption

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


## The transcript of the sound file (its full path, without its extension) in the simulation's
## language, beside it wherever it is - the sounds or a scenery - or null when it has none
static func from_sound_file(sound_path:String) -> Transcript:
    var base_dir:String = sound_path.get_base_dir()
    var filename:String = "%s-%s%s" % [sound_path.get_file(), ReloadedTranslationServer.language, EXTENSION]
    var path:String = base_dir.path_join(MaszynaDataPath.resolve(base_dir, filename))
    if not FileAccess.file_exists(path):
        return null
    return parse(MaszynaLegacyText.decode(FileAccess.get_file_as_bytes(path)))


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
