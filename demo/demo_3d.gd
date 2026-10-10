extends Node3D

## The "Diagnostics" entry of the frame time statistics (DebugMenu), after the DebugHud's
var _frame_times_index: int = -1
## The "Diagnostics" entry showing or hiding all the DebugHud's windows at once (F12, as the game's
## View > Controls)
var _all_windows_index: int = -1


## Before _ready(): the vehicles under this node must not read a cache left by another build
func _enter_tree() -> void:
    GameDataServer.build_check_version()


func _ready() -> void:
    TrackServer.topology_rebuild()
    %DebugHud.fill_menu(%Diagnostics)
    %DebugHud.attach_environment(%MaszynaEnvironmentNode)
    %Diagnostics.add_item("Frame time statistics")
    _frame_times_index = %Diagnostics.item_count - 1
    %Diagnostics.set_item_shortcut(_frame_times_index, ActionShortcut.create(&"cycle_debug_menu"))
    %Diagnostics.add_check_item("All windows")
    _all_windows_index = %Diagnostics.item_count - 1
    %Diagnostics.set_item_shortcut(_all_windows_index, ActionShortcut.create(&"hud_toggle"))


func _on_diagnostics_menu_index_pressed(index: int) -> void:
    if index == _frame_times_index:
        DebugMenu.cycle_style()
        return
    if index == _all_windows_index:
        %Diagnostics.toggle_item_checked(index)
        %DebugHud.windows_set_visible(%Diagnostics.is_item_checked(index))
