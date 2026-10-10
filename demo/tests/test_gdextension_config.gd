extends MaszynaGutTest

## The extension's configuration. It is not reloadable: Godot tracks the instances of a reloadable
## extension in a set it does not lock (GDExtension::_track_instance(), core/extension/
## gdextension.cpp), in the editor only, and libmaszyna creates and frees its objects on the
## streaming's worker threads - the set broke and the editor crashed
## (docs/findings-archive.md, 2026-10-03 the editor ran the scenario).

const GDEXTENSION_PATH:String = "res://addons/libmaszyna/libmaszyna.gdextension"


func test_the_extension_is_not_reloadable() -> void:
    var config:ConfigFile = ConfigFile.new()
    assert_eq(config.load(GDEXTENSION_PATH), OK, "the extension's configuration is not read")
    assert_false(config.get_value("configuration", "reloadable", false),
            "a reloadable extension must not create or free objects off the main thread")
