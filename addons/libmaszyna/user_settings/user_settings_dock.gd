@tool
extends Control


func _on_browse_button_up():
    %DirectorySelectorDialog.popup_centered()


## Not while the scene itself is edited - the user's game dir would be saved into the .tscn
## instead of the default "."
func _refresh():
    if is_part_of_edited_scene():
        return
    if visible and is_inside_tree() and UserSettings:
        UserSettings.load_config()
        %LineEdit.text = UserSettings.get_maszyna_game_dir()


func _ready():
    if OS.has_feature("release") and not OS.has_feature("editor"):
        $VBoxContainer/GameDirSection.visible = false
    _refresh()

func _enter_tree():
    get_tree().root.focus_entered.connect(_refresh)


func _exit_tree():
    get_tree().root.focus_entered.disconnect(_refresh)


func _on_directory_selector_dialog_dir_selected(dir):
    %LineEdit.text = dir
    UserSettings.save_maszyna_game_dir(dir)


func _on_clear_cache_button_button_up():
    var fn = func():
        GameDataServer.cache_clear()

    call_func_with_message_window("Clering caches...", "Please wait.\nClearing caches in progress...", fn)


## Everything read from the game directory is read again - the caches on disk stay, "Clear caches"
## is the other button
func _on_reload_game_data_button_button_up():
    call_func_with_message_window(
        "Reloading game data...", "Please wait.\nGame data reloading in progress...", GameDataServer.data_reload)


func _show_message_window(title:String, message: String):
    $InfoMessageWindow/FlowContainer/Label.text = message
    $InfoMessageWindow.title = title
    $InfoMessageWindow.popup_centered()

func _close_message_window():
    $InfoMessageWindow.hide()

func call_func_with_message_window(title: String, message: String, callable: Callable):
    # A tricky method to display properly popup window with the message

    _show_message_window(title, message)

    var do_call = func():
        # Waits until the window is fully rendered (2 frames)
        #
        # The Godot Editor is written in the Godot Engine, so when a long-running task
        # is executed in a signal handler, the execution of main_loop() / _process()
        # will be blocked and the editor UI will not be rendered correctly.
        await Engine.get_main_loop().process_frame
        await Engine.get_main_loop().process_frame
        callable.call()
        _close_message_window()

    # Yes, must be deferred call here.
    do_call.call_deferred()

## Saved once the path is entered, not letter by letter: every change of the game directory reloads
## the game's data
func _on_line_edit_text_submitted(new_text:String) -> void:
    UserSettings.save_maszyna_game_dir(new_text)
