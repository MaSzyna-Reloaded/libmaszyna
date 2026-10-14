extends Control
class_name DebugHud

## The diagnostic windows of the player's vehicle and of the scenery - each a HUDWindow child of
## this node, closed at the start. Whoever has a menu for them (the game's top bar, a demo scene)
## fills its own PopupMenu with fill_menu() and connects the menu's index_pressed to
## menu_index_pressed() in its scene; entries it adds after them are its own.

## Menu order snapshot - HUDWindow.move_to_front() reorders the children.
var _windows: Array[HUDWindow] = []
## The menu entry of the developer console (Console), after the windows
var _console_index: int = -1


func _ready() -> void:
    PlayerServer.player_vehicle_changed.connect(_on_player_vehicle_changed)
    for child: Node in get_children():
        var win: HUDWindow = child as HUDWindow
        win.visible = false
        _windows.append(win)


func _exit_tree() -> void:
    PlayerServer.player_vehicle_changed.disconnect(_on_player_vehicle_changed)


## The windows and the developer console as entries of `menu`, with their keys; the menu is
## expected empty, its first entries' indices are this node's
func fill_menu(menu: PopupMenu) -> void:
    for win: HUDWindow in _windows:
        menu.add_item(win.title)
    menu.set_item_shortcut(_windows.find($WeatherAndTime), ActionShortcut.create(&"toggle_weather_controls"))
    # Tab, as the original's map panel (driveruilayer.cpp:136)
    menu.set_item_shortcut(_windows.find($MiniMap), ActionShortcut.create(&"minimap_toggle"))
    menu.add_separator()
    menu.add_item("Developer console")
    _console_index = menu.item_count - 1
    menu.set_item_shortcut(_console_index, ActionShortcut.create(&"console_toggle"))


## An entry of the menu filled by fill_menu() picked: its window shown or hidden, the console
## toggled; an entry past them is not this node's
func menu_index_pressed(index: int) -> void:
    if index == _console_index:
        Console.toggle_console()
        return
    if index >= _windows.size():
        return
    var win: HUDWindow = _windows[index]
    win.visible = not win.visible
    _propagate_vehicle(win, PlayerServer.player_get_vehicle())


## All the windows shown or hidden at once
func windows_set_visible(shown: bool) -> void:
    for win: HUDWindow in _windows:
        win.visible = shown
        _propagate_vehicle(win, PlayerServer.player_get_vehicle())


## The environment of the world being shown, for the weather and time window; null while there is
## no world (the menu)
func attach_environment(environment: MaszynaEnvironmentNode) -> void:
    %WeatherControls.attach_environment(environment)


## The vehicle the player is driving, handed to every widget that shows something about it. The
## widgets used to be given a NodePath into the vehicle's own subtree and resolve it themselves,
## which reached across two scenes and could resolve before the vehicle had been built. They are
## given the vehicle itself now, and PlayerServer says when it changes.
## Each control and section picks the vehicle of its own target from it, as a cab control does.
func _on_player_vehicle_changed(vehicle: RID, _previous: RID) -> void:
    for win: HUDWindow in _windows:
        _propagate_vehicle(win, vehicle)


func _propagate_vehicle(node: Node, vehicle: RID) -> void:
    for child: Node in node.get_children():
        if "vehicle" in child:
            child.vehicle = vehicle
        _propagate_vehicle(child, vehicle)
