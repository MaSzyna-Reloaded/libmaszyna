@tool
extends Node

## A missing sound plays this instead: a vehicle whose data lacks a file stays quiet there rather
## than failing, as the original only logs it (audio.cpp:147). A tenth of a second of 16-bit mono.
const SILENCE_MIX_RATE:int = 44100
const SILENCE_BYTES:int = 8820

## An Ogg page: "OggS", version, header type, then the granule position at this offset (RFC 3533)
const OGG_CAPTURE_PATTERN:String = "OggS"
const OGG_GRANULE_OFFSET:int = 6
const OGG_SEGMENT_COUNT_OFFSET:int = 26
const OGG_PAGE_HEADER_SIZE:int = 27
## The largest page: its header, 255 lacing values and 255 segments of 255 bytes
const OGG_MAX_PAGE_SIZE:int = 65307
## The last page is looked for in this much of the end of the file first - it is rarely longer
const OGG_TAIL_SIZE:int = 8192
## The Vorbis identification header (the first packet): type 1, "vorbis", version, channels, then
## the sample rate at this offset (Vorbis I specification, 4.2.2)
const VORBIS_SAMPLE_RATE_OFFSET:int = 12
const VORBIS_SAMPLE_RATE_SIZE:int = 4

## The extensions a sound's file is looked for with, in the original's order (audio.cpp:184-191)
const OGG_EXTENSION:String = "ogg"
const FLAC_EXTENSION:String = "flac"
const WAV_EXTENSION:String = "wav"
const SOUND_EXTENSIONS:PackedStringArray = [OGG_EXTENSION, FLAC_EXTENSION, WAV_EXTENSION]

var _silence:AudioStreamWAV = AudioStreamWAV.new()
## The .wav files read so far, by their path - Godot's resource cache takes .ogg only
var _wav_streams:Dictionary[String, AudioStreamWAV] = {}


func _init() -> void:
    _silence.format = AudioStreamWAV.FORMAT_16_BITS
    _silence.mix_rate = SILENCE_MIX_RATE
    var data:PackedByteArray = PackedByteArray()
    data.resize(SILENCE_BYTES)
    _silence.data = data


## The file a sound of the game is read from, looked for where the original looks
## (buffer_manager::create, audio.cpp:107-147): in the vehicle's directory, then under the path the
## name gives, then in sounds/, with each of the original's extensions in turn (audio.cpp:184-191);
## "" when there is none.
func find_sound(name:String, vehicle_dir:String) -> String:
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var directories:Array[String] = []
    if vehicle_dir:
        directories.append(vehicle_dir)
    if "/" in name:
        directories.append(game_dir)
    directories.append(game_dir.path_join("sounds"))
    for directory:String in directories:
        for extension:String in SOUND_EXTENSIONS:
            var path:String = directory.path_join(MaszynaDataPath.resolve(directory, name + "." + extension))
            if FileAccess.file_exists(path):
                return path
    return ""


func get_stream(name:String, loop:bool, vehicle_dir:String) -> AudioStream:
    var full_path:String = find_sound(name, vehicle_dir)
    if not full_path:
        push_warning("[%s] file does not exist: %s" % [self, name])
        return _silence
    match full_path.get_extension().to_lower():
        WAV_EXTENSION:
            var wav:AudioStreamWAV = _load_wav(full_path)
            if wav and not (wav.loop_mode == AudioStreamWAV.LOOP_FORWARD) == loop:
                wav = wav.duplicate(0)
                wav.loop_mode = AudioStreamWAV.LOOP_FORWARD if loop else AudioStreamWAV.LOOP_DISABLED
                wav.loop_begin = 0
                # the end of the loop is the last sample frame: the data over the bytes of a frame
                wav.loop_end = wav.data.size() / ((2 if wav.format == AudioStreamWAV.FORMAT_16_BITS else 1)
                        * (2 if wav.stereo else 1))
            return wav if wav else _silence
        FLAC_EXTENSION:
            # Godot reads no FLAC; the game data has none either (TODO.md)
            push_warning("[%s] FLAC is not read: %s" % [self, full_path])
            return _silence
    var stream:AudioStreamOggVorbis = load(full_path)  # uses godot's builtin resource cache
    if stream and not stream.loop == loop:
        stream = stream.duplicate(0)
        stream.loop = loop
    return stream


func _load_wav(path:String) -> AudioStreamWAV:
    if not _wav_streams.has(path):
        _wav_streams[path] = AudioStreamWAV.load_from_file(path)
    return _wav_streams[path]


## The duration [s] of a sound of the game, read off its Ogg pages without loading or decoding it:
## the samples the last page ends at over the sample rate of the identification header. 0.0 when
## the file is missing or is no Ogg Vorbis - silently, as an unknown length is an expected outcome
## where it is asked for (a voice's start placed before the file is read).
func get_stream_length(name:String, vehicle_dir:String) -> float:
    if not name:
        return 0.0
    var path:String = find_sound(name, vehicle_dir)
    if not path:
        return 0.0
    if path.get_extension().to_lower() == WAV_EXTENSION:
        var wav:AudioStreamWAV = _load_wav(path)
        return wav.get_length() if wav else 0.0
    var file:FileAccess = FileAccess.open(path, FileAccess.READ)
    if not file:
        return 0.0
    var first_page:PackedByteArray = file.get_buffer(OGG_PAGE_HEADER_SIZE)
    if first_page.size() < OGG_PAGE_HEADER_SIZE \
            or not first_page.slice(0, OGG_CAPTURE_PATTERN.length()).get_string_from_ascii() == OGG_CAPTURE_PATTERN:
        return 0.0
    file.seek(OGG_PAGE_HEADER_SIZE + first_page[OGG_SEGMENT_COUNT_OFFSET])
    var identification:PackedByteArray = file.get_buffer(VORBIS_SAMPLE_RATE_OFFSET + VORBIS_SAMPLE_RATE_SIZE)
    var sample_rate:int = identification.decode_u32(VORBIS_SAMPLE_RATE_OFFSET)
    if sample_rate <= 0:
        return 0.0
    for tail_size:int in [OGG_TAIL_SIZE, OGG_MAX_PAGE_SIZE + OGG_PAGE_HEADER_SIZE]:
        var start:int = maxi(file.get_length() - tail_size, 0)
        file.seek(start)
        var tail:PackedByteArray = file.get_buffer(file.get_length() - start)
        var at:int = tail.rfind(OGG_CAPTURE_PATTERN.unicode_at(0), tail.size() - OGG_PAGE_HEADER_SIZE)
        while at >= 0:
            if tail.slice(at, at + OGG_CAPTURE_PATTERN.length()).get_string_from_ascii() == OGG_CAPTURE_PATTERN:
                var granule:int = tail.decode_s64(at + OGG_GRANULE_OFFSET)
                # -1: no packet ends on this page
                if granule >= 0:
                    return float(granule) / sample_rate
            at = tail.rfind(OGG_CAPTURE_PATTERN.unicode_at(0), at - 1) if at > 0 else -1
        if start == 0:
            break
    return 0.0
