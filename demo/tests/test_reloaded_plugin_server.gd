extends MaszynaGutTest

## ReloadedPluginServer lists the plugins the library runs, each with what the player is shown of it.


func test_the_maszyna_plugin_is_listed_with_its_name_and_logo() -> void:
    var plugins:Array[ReloadedPlugin] = ReloadedPluginServer.plugin_get_list()
    var maszyna:ReloadedPlugin = null
    for plugin:ReloadedPlugin in plugins:
        if plugin is MaszynaPlugin:
            maszyna = plugin
    assert_not_null(maszyna, "the MaSzyna plugin is not listed")
    if not maszyna:
        return
    assert_eq(maszyna.name, "MaSzyna")
    assert_true(maszyna.description.length() > 0, "the plugin has no description")
    assert_not_null(maszyna.logo, "the plugin's logo was not loaded")
