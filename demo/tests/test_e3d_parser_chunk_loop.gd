extends MaszynaGutTest

## _parse_file()'s chunk loop used to run while (!file.eof_reached()): past the real end of the
## file, seek() clears the flag the next header's empty read just set, so it reads another empty
## header, seeks another ~4 GB (the wrapped-around data_len of a zero-length header), and repeats -
## on Linux until lseek hits ext4's 2^44 limit (some 4096 iterations), never on Windows
## (MaSzyna-Reloaded/reports#91, a regression of libmaszyna#388). The loop now runs while the
## position is before the E3D0 chunk's own declared end, as the original does.

## A file is "E3D0" + its declared chunk length (data_len = length - 8); a length of 8 declares no
## chunks follow - the file ends exactly where a scenery's last real chunk also leaves it
const EMPTY_E3D0: PackedByteArray = [69, 51, 68, 48, 8, 0, 0, 0]  # "E3D0", length 8 (data_len 0)
## Measured: the old loop's ~4096 wasted seeks took 10402 usec parsing this file; the fix, 54 usec.
## Comfortably above the fix's time and two orders of magnitude under the old loop's
const MAX_PARSE_USEC: int = 2000


func _write_temp(bytes: PackedByteArray) -> String:
    var path: String = "user://test_e3d_parser_chunk_loop.e3d"
    var file: FileAccess = FileAccess.open(path, FileAccess.WRITE)
    file.store_buffer(bytes)
    file.close()
    return path


func test_a_file_ending_exactly_at_its_declared_length_parses_without_the_old_wasted_loop() -> void:
    var path: String = _write_temp(EMPTY_E3D0)
    var file: FileAccess = FileAccess.open(path, FileAccess.READ)
    var start_usec: int = Time.get_ticks_usec()
    var model: MaszynaE3DModel = MaszynaE3DParser.parse(file)
    var elapsed_usec: int = Time.get_ticks_usec() - start_usec

    assert_not_null(model)
    assert_eq(model.submodels.size(), 0)
    assert_lt(elapsed_usec, MAX_PARSE_USEC, "an empty model parses at once, not after wasted seeks")
