extends MaszynaGutTest

## `jointcabs:` (DynObj.cpp:6626) - all virtual cabs share one model, so RailVehicle3D hides every
## low-poly "cabN" submodel from inside instead of only the occupied one (DynObj.cpp:1218).

const FIXTURE_DIR:String = "user://gut/mmd_joint_cabs_fixture"


func _write_mmd(file_name:String, content:String) -> String:
    DirAccess.make_dir_recursive_absolute(FIXTURE_DIR)
    var path:String = ProjectSettings.globalize_path(FIXTURE_DIR.path_join(file_name))
    var file:FileAccess = FileAccess.open(path, FileAccess.WRITE)
    file.store_string(content)
    file.close()
    return path


func test_joint_cabs_true_is_read() -> void:
    var path:String = _write_mmd("joint.mmd", "internaldata:\njointcabs: true\ncab1definition:\n")
    assert_true(MmdCabinInstancer.parse_joint_cabs(path, {}))


func test_joint_cabs_defaults_to_false() -> void:
    var path:String = _write_mmd("separate.mmd", "internaldata:\ncab1definition:\n")
    assert_false(MmdCabinInstancer.parse_joint_cabs(path, {}))
