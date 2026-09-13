extends PanelContainer


func _ready() -> void:
    _auto_user_settings_visibility()
    UserSettings.game_dir_changed.connect(_on_gamedir_changed)
    UserSettings.config_changed.connect(_update_render_settings)
    UserSettings.setting_changed.connect(_on_user_setting_changed)
    _update_render_settings()


func _input(event):
    if event.is_action_pressed("ui_cancel"):
        visible = not visible


func _on_gamedir_changed():
    _auto_user_settings_visibility()
    E3DModelManager.clear_cache()
    MaterialManager.clear_cache()
    _reload_all_models()


func _auto_user_settings_visibility():
    var game_dir = UserSettings.get_maszyna_game_dir()
    visible = not game_dir or FileAccess.file_exists(game_dir)


func _update_render_settings():
    var viewport: Viewport = get_tree().root.get_viewport()
    viewport.anisotropic_filtering_level = UserSettings.get_setting("render", "anisotropic_filtering_level", 2)
    viewport.screen_space_aa = UserSettings.get_setting("render", "screen_space_aa", Viewport.SCREEN_SPACE_AA_FXAA)
    viewport.msaa_3d = UserSettings.get_setting("render", "msaa_3d", 2)
    viewport.use_taa = UserSettings.get_setting("render", "use_taa", true)
    viewport.fsr_sharpness = UserSettings.get_setting("render", "fsr_sharpness", 0.2)
    DisplayServer.window_set_vsync_mode(
        DisplayServer.VSYNC_ENABLED
        if UserSettings.get_setting("window", "vsync_enabled", true)
        else DisplayServer.VSYNC_DISABLED
    )

func _on_user_setting_changed(section, key):
    if section == "e3d" and key == "use_alpha_transparency":
        _reload_all_models()


func _reload_all_models():
    var instances = get_tree().root.find_children("", "E3DModelInstance", true, false)
    for instance in instances:
        instance.reload()


func _on_visibility_changed() -> void:
    var game_dir = UserSettings.get_maszyna_game_dir()
    $VBoxContainer/GameDirNotSet.visible = not game_dir or FileAccess.file_exists(game_dir)
