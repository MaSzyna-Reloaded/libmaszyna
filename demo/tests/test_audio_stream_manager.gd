extends MaszynaGutTest

## A sound's length read off its Ogg pages, before the file is ever loaded to play; a sound looked
## for where the original looks (buffer_manager::create, audio.cpp:107-147): the vehicle's directory
## first, then sounds/, with the original's extensions .ogg, .flac, .wav in turn (audio.cpp:184-191)

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SOUND:String = "test_loop"
const MISSING_SOUND:String = "no_such_sound"
## A quarter of a second of a .wav (fixtures/sounds/test_beep.wav)
const WAV_SOUND:String = "test_beep"
const WAV_LENGTH:float = 0.25
const LENGTH_TOLERANCE:float = 0.001
## A vehicle directory of the fixtures with its own test_loop
const VEHICLE_DIR:String = "res://tests/fixtures/dynamic/test/animated_v1"

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_length_read_off_the_pages_is_the_decoded_length() -> void:
    var decoded:AudioStream = AudioStreamManager.get_stream(SOUND, false, "")

    assert_gt(decoded.get_length(), 0.0)
    assert_almost_eq(AudioStreamManager.get_stream_length(SOUND, ""), decoded.get_length(), LENGTH_TOLERANCE)


func test_a_missing_sound_has_no_length() -> void:
    assert_eq(AudioStreamManager.get_stream_length(MISSING_SOUND, ""), 0.0)


func test_a_vehicle_sound_is_looked_for_in_its_directory_first() -> void:
    var vehicle_dir:String = ProjectSettings.globalize_path(VEHICLE_DIR)
    assert_eq(AudioStreamManager.find_sound(SOUND, vehicle_dir), vehicle_dir.path_join(SOUND + ".ogg"))
    assert_eq(AudioStreamManager.find_sound(SOUND, ""),
            UserSettings.get_maszyna_game_dir().path_join("sounds").path_join(SOUND + ".ogg"), "no vehicle: sounds/")


func test_a_wav_sound_is_found_and_read() -> void:
    assert_eq(AudioStreamManager.find_sound(WAV_SOUND, "").get_extension(), "wav")
    var stream:AudioStream = AudioStreamManager.get_stream(WAV_SOUND, true, "")
    assert_true(stream is AudioStreamWAV)
    assert_almost_eq(stream.get_length(), WAV_LENGTH, LENGTH_TOLERANCE)
    assert_eq((stream as AudioStreamWAV).loop_mode, AudioStreamWAV.LOOP_FORWARD, "looped as asked")
    assert_almost_eq(AudioStreamManager.get_stream_length(WAV_SOUND, ""), WAV_LENGTH, LENGTH_TOLERANCE)
