extends GutTest

## MaSzyna's cab implementation, registered as libmaszyna starts, lists the roles of an MMD file's
## cab and makes their widgets with the catalog's defaults; a cab built of an MMD file names it


func test_the_train_brake_handle_is_listed_as_a_knob_sending_its_commands() -> void:
    var listed:Array[CabinComponentInfo] = CabinSystem.component_list(MaszynaCabinImplementation.NAME)
    assert_eq(listed.size(), MmdSemanticCatalog.get_labels().size(), "every role of the catalog")
    var brake:Array = listed.filter(func(info:CabinComponentInfo) -> bool: return info.name == &"brakectrl")
    assert_eq(brake.size(), 1)
    assert_eq(brake[0].widget_class, &"CabinKnob")
    assert_has(brake[0].commands, "brake_level_set")
    assert_ne(brake[0].title, "")


func test_the_train_brake_handle_is_made_as_the_catalog_has_it() -> void:
    var widget:Node3D = CabinSystem.component_create(MaszynaCabinImplementation.NAME, &"brakectrl")
    assert_true(widget is CabinKnob)
    assert_eq(widget.control_id, &"brakectrl")
    assert_eq(widget.command, "brake_level_set")
    widget.free()


func test_a_role_the_catalog_has_not_is_not_made() -> void:
    assert_null(CabinSystem.component_create(MaszynaCabinImplementation.NAME, &"no_such_role"))


func test_a_cab_of_an_mmd_file_names_the_implementation() -> void:
    var cabin := MaszynaDynamicTrainCabin.new()
    assert_eq(cabin.implementation, MaszynaCabinImplementation.NAME)
    cabin.free()
