@tool
extends VBoxContainer

## Every MaSzyna vehicle of the catalogue (VehicleCatalogServer) to search through and add to the
## edited scene: "Spawn" adds the selected vehicle in the selected livery - its provider's, a skin -
## as a MaszynaRailVehicle3D - on the
## named track at the offset when a track is named, else where the 3D view's centre looks. A tile
## dragged into the 3D view, as a nodebank model is, adds its vehicle where it is dropped.

const VehicleProfileQueue = preload("./vehicle_profile_queue.gd")
const ITEMS_PER_PAGE:int = 50
## Size of a vehicle's tile [px] - a side view of a long car is about 5 times as wide as it is
## high (MaszynaVehicleProfileManager.PROFILE_PIXELS_PER_METRE)
const TILE_SIZE:Vector2 = Vector2(180.0, 60.0)
## Size of the popup the start track is picked in [px]
const TRACK_PICKER_SIZE:Vector2i = Vector2i(480, 400)

## The catalogue's MaSzyna vehicles, by name
var _vehicles:Array[VehicleInfo] = []
## The skins of each of them - its provider's liveries, by name - by the vehicle's name
var _skins:Dictionary[String, PackedStringArray] = {}
## The vehicles the search finds, all pages
var _found:Array[VehicleInfo] = []
var _page:int = 0
var _selected:VehicleInfo = null
var _profile_queue:VehicleProfileQueue = VehicleProfileQueue.new()


func _enter_tree() -> void:
    GameDataServer.data_reload_requested.connect(_on_data_reload_requested)


func _exit_tree() -> void:
    GameDataServer.data_reload_requested.disconnect(_on_data_reload_requested)


## The vehicles are taken from the catalogue again and searched anew
func load_vehicles() -> void:
    _vehicles.clear()
    _skins.clear()
    for category:VehicleInfo.VehicleCategory in VehicleInfo.VehicleCategory.values():
        for info:VehicleInfo in VehicleCatalogServer.vehicle_list(category):
            if not info.vehicle_provider == MaszynaVehicleData.VEHICLE_PROVIDER:
                continue
            _vehicles.append(info)
            var skins:PackedStringArray = []
            for livery:VehicleLivery in VehicleCatalogServer.livery_list(info):
                # a player's livery is not drawn yet
                if not livery.base:
                    skins.append(livery.name)
            _skins[info.vehicle] = skins
    _vehicles.sort_custom(func(a:VehicleInfo, b:VehicleInfo) -> bool: return a.vehicle < b.vehicle)
    search_vehicles()


## The vehicles whose name or a skin contains the search text (any case), from the first page; all
## of them for none
func search_vehicles() -> void:
    var text:String = %Search.text.strip_edges().to_lower()
    _found = _vehicles.filter(func(info:VehicleInfo) -> bool:
            return not text or info.vehicle.to_lower().contains(text) \
                    or " ".join(_skins[info.vehicle]).to_lower().contains(text))
    show_page(0)


func show_page(page:int) -> void:
    var page_count:int = maxi(ceili(float(_found.size()) / ITEMS_PER_PAGE), 1)
    _page = clampi(page, 0, page_count - 1)
    %Count.text = "%d vehicles" % _found.size()
    %PageNumber.text = "%d/%d" % [_page + 1, page_count]
    %PagePrev.disabled = _page <= 0
    %PageNext.disabled = _page >= page_count - 1
    _profile_queue.clear()
    for tile:Node in %Vehicles.get_children():
        tile.queue_free()
    var start:int = _page * ITEMS_PER_PAGE
    for vehicle:VehicleInfo in _found.slice(start, start + ITEMS_PER_PAGE):
        var skins:PackedStringArray = _skins[vehicle.vehicle]
        var tile:Button = Button.new()
        tile.custom_minimum_size = TILE_SIZE
        tile.text = vehicle.title
        tile.tooltip_text = vehicle.vehicle
        tile.expand_icon = true
        tile.icon_alignment = HORIZONTAL_ALIGNMENT_CENTER
        tile.vertical_icon_alignment = VERTICAL_ALIGNMENT_TOP
        tile.clip_text = true
        tile.gui_input.connect(_on_tile_gui_input.bind(vehicle))
        %Vehicles.add_child(tile)
        if skins:
            _profile_queue.enqueue(
                    vehicle.vehicle.get_base_dir(), vehicle.vehicle.get_file(), skins[0], "", tile.set_button_icon)


## The vehicle whose skins are listed and which "Spawn" adds, with its first skin selected
func select_vehicle(vehicle:VehicleInfo) -> void:
    _selected = vehicle
    var skins:PackedStringArray = _skins[vehicle.vehicle]
    %VehicleName.text = vehicle.title
    %VehiclePath.text = vehicle.vehicle.get_base_dir()
    %Skins.clear()
    for skin:String in skins:
        %Skins.add_item(skin)
    %SpawnButton.disabled = not skins
    %SpawnStatus.text = ""
    if skins:
        %Skins.select(0)
        select_skin(0)


## The side view of the selected vehicle in the skin at index
func select_skin(index:int) -> void:
    %Profile.texture = null
    _profile_queue.enqueue(_selected.vehicle.get_base_dir(), _selected.vehicle.get_file(),
            _skins[_selected.vehicle][index], "", %Profile.set_texture)


## The popup the start track is picked in, every named track listed and its search box ready
func open_track_picker() -> void:
    %TrackSearch.text = ""
    list_tracks()
    %TrackPicker.popup_centered(TRACK_PICKER_SIZE)
    %TrackSearch.grab_focus()


## The named tracks TrackServer knows, by name - those whose name contains the searched text
func list_tracks() -> void:
    var query:String = %TrackSearch.text.strip_edges().to_lower()
    var names:Array[String] = []
    for track:RID in TrackServer.track_get_rids():
        var track_name:String = TrackServer.track_get_name(track)
        if track_name and (not query or track_name.to_lower().contains(query)):
            names.append(track_name)
    names.sort()
    %Tracks.clear()
    %SelectTrackButton.disabled = true
    for track_name:String in names:
        %Tracks.add_item(track_name)


## The selected vehicle in the selected skin goes into the edited scene - on the track named at
## track_offset, else at world_position - as one action to undo; null when it cannot
func spawn_vehicle(world_position:Vector3, track_name:String, track_offset:float) -> MaszynaRailVehicle3D:
    var scene_root:Node3D = EditorInterface.get_edited_scene_root() as Node3D
    if not scene_root:
        %SpawnStatus.text = "A vehicle can only be added to a 3D scene."
        return null
    if track_name and not TrackServer.track_get_rid_by_name(track_name).is_valid():
        %SpawnStatus.text = "No track named \"%s\"." % track_name
        return null
    %SpawnStatus.text = ""

    var vehicle:MaszynaRailVehicle3D = MaszynaRailVehicle3D.new()
    vehicle.name = _selected.title
    vehicle.data_path = _selected.vehicle.get_base_dir()
    vehicle.file_name = _selected.vehicle.get_file()
    vehicle.skin = _skins[_selected.vehicle][%Skins.get_selected_items()[0]]
    if track_name:
        vehicle.start_track_name = track_name
        vehicle.start_track_offset = track_offset
    else:
        # a vehicle on no track stands where its node does (RailVehicle3D.cpp:177)
        vehicle.position = scene_root.global_transform.affine_inverse() * world_position

    var undo_redo:EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
    undo_redo.create_action("Spawn vehicle %s" % _selected.title)
    undo_redo.add_do_method(scene_root, "add_child", vehicle, true)
    # the node alone: the vehicle it builds is never saved with the scene
    undo_redo.add_do_property(vehicle, "owner", scene_root)
    undo_redo.add_undo_method(scene_root, "remove_child", vehicle)
    undo_redo.add_do_reference(vehicle)
    undo_redo.commit_action()

    EditorInterface.get_selection().clear()
    EditorInterface.get_selection().add_node(vehicle)
    return vehicle


## "Spawn": where the centre of the 3D view looks, unless a track is named
func _on_spawn_button_pressed() -> void:
    var viewport:SubViewport = EditorInterface.get_editor_viewport_3d(0)
    var centre:Vector3 = EditorViewportPoint.world_point(
            viewport.get_camera_3d(), viewport.get_visible_rect().get_center())
    var track_name:String = %TrackName.text.strip_edges()
    var vehicle:MaszynaRailVehicle3D = spawn_vehicle(centre, track_name, %TrackOffset.value)
    # on a track, the view goes to it once it stands there - when it is built and placed
    if vehicle and track_name:
        vehicle.vehicle_built.connect(_on_spawned_vehicle_built.bind(vehicle), CONNECT_ONE_SHOT)


## A tile pressed selects its vehicle and starts dragging it, its exterior under the mouse in the
## 3D view: dropped there, the vehicle stands where it is dropped
func _on_tile_gui_input(event:InputEvent, vehicle:VehicleInfo) -> void:
    var button:InputEventMouseButton = event as InputEventMouseButton
    if button and button.button_index == MOUSE_BUTTON_LEFT and button.pressed:
        select_vehicle(vehicle)
        var preview:MaszynaE3DModelInstance = MaszynaRailVehicle3DInstancer.build_exterior(
                vehicle.vehicle.get_base_dir(), vehicle.vehicle.get_file(), _skins[vehicle.vehicle][0], "")
        preview.transform = MaszynaRailVehicle3DInstancer.MASZYNA_VEHICLE_FRAME
        %TrackDropHint.set_vehicle_category(vehicle.category)
        %ViewportDrop.start(preview, _on_vehicle_dropped)


## Dropped over a track drawn green, the vehicle goes on it at the offset under the cursor;
## anywhere else, where it is dropped
func _on_vehicle_dropped(world_position:Vector3) -> void:
    spawn_vehicle(world_position, %TrackDropHint.get_hovered_track_name(), %TrackDropHint.get_hovered_offset())


## The vehicle spawned on a track stands there: the 3D view goes to it
func _on_spawned_vehicle_built(vehicle:MaszynaRailVehicle3D) -> void:
    EditorInterface.get_selection().clear()
    EditorInterface.get_selection().add_node(vehicle)
    EditorViewFocus.focus_selection()


func _on_track_search_text_changed(_text:String) -> void:
    list_tracks()


## The track highlighted in the list is the track the vehicle is spawned on
func select_track() -> void:
    %TrackName.text = %Tracks.get_item_text(%Tracks.get_selected_items()[0])
    %TrackPicker.hide()


func _on_tracks_item_selected(_index:int) -> void:
    %SelectTrackButton.disabled = false


func _on_page_prev_pressed() -> void:
    show_page(_page - 1)


func _on_page_next_pressed() -> void:
    show_page(_page + 1)


func _on_search_text_changed(_text:String) -> void:
    search_vehicles()


## The vehicles are taken the first time the panel is shown
func _on_visibility_changed() -> void:
    if visible and not _vehicles:
        load_vehicles()


func _on_data_reload_requested() -> void:
    if visible:
        load_vehicles()
