@tool
extends VBoxContainer

## The trainsets of the edited scene - those its sceneries placed (RailVehicleServer's, by their
## handles) and those assembled by hand (TrainSet3D) - each with the side views of its vehicles,
## listed again whenever one of the scene's sceneries has loaded. "Show" centres the 3D view on the
## trainset's vehicles. A scenery's `dynamic` outside a `trainset` has no name and is not listed.
## "Spawn..." asks for the browser of vehicles to add one to the scene. "Set as start vehicle", while
## the edited scene has a MaszynaPlayer, makes the trainset's first vehicle with a cab the player's
## start_vehicle_id.

## "Spawn..." was pressed: the browser of vehicles is wanted
signal spawn_requested

const TrainsetRow = preload("./trainset_row.gd")
const VehicleProfileQueue = preload("./vehicle_profile_queue.gd")
const TRAINSET_ROW:PackedScene = preload("./trainset_row.tscn")

## The sceneries of the edited scene whose `loaded` lists the trainsets again, by instance id - a
## closed scene frees them
var _include_ids:Array[int] = []
var _scene_root:Node = null
var _profile_queue:VehicleProfileQueue = VehicleProfileQueue.new()
## The vehicles have no nodes to select, and the 3D view goes only to what is selected: "Show" puts
## a marker node where each vehicle of the trainset stands, selects those, and takes them away at
## the next "Show" or another scene
var _show_markers:Array[Node3D] = []


func _enter_tree() -> void:
    get_tree().node_added.connect(_on_node_added)
    get_tree().node_removed.connect(_on_node_removed)


func _exit_tree() -> void:
    get_tree().node_added.disconnect(_on_node_added)
    get_tree().node_removed.disconnect(_on_node_removed)


## The edited scene: its trainsets are listed now, and again whenever one of its sceneries loads
func set_scene_root(root:Node) -> void:
    for include_id:int in _include_ids:
        var include:MaszynaIncludeNode = instance_from_id(include_id) as MaszynaIncludeNode
        if include:
            include.loaded.disconnect(list_trainsets)
    _include_ids.clear()
    _free_show_markers()
    _scene_root = root
    if root:
        var includes:Array[Node] = root.find_children("", "MaszynaIncludeNode", true, false)
        if root is MaszynaIncludeNode:
            includes.append(root)
        for node:Node in includes:
            if _is_scene_scenery(node):
                (node as MaszynaIncludeNode).loaded.connect(list_trainsets)
                _include_ids.append(node.get_instance_id())
    list_trainsets()


## A scenery coming into the edited scene after it was set - the one a test scene shows (the
## scenery importer's scenery_test.tscn)
func add_scenery(include:MaszynaIncludeNode) -> void:
    if not _is_scene_scenery(include) or _include_ids.has(include.get_instance_id()):
        return
    include.loaded.connect(list_trainsets)
    _include_ids.append(include.get_instance_id())
    list_trainsets()


## A scenery leaving the edited scene - its trainsets go from the list
func remove_scenery(include:MaszynaIncludeNode) -> void:
    if not _include_ids.has(include.get_instance_id()):
        return
    include.loaded.disconnect(list_trainsets)
    _include_ids.erase(include.get_instance_id())
    list_trainsets()


## A scenery of the edited scene: the scene's own, or one of a scene shown in it, made there without
## an owner - but not an include inside another scenery, which is that scenery's content
func _is_scene_scenery(include:Node) -> bool:
    if not _scene_root or not (include == _scene_root or _scene_root.is_ancestor_of(include)):
        return false
    var node:Node = include
    while not node == _scene_root:
        node = node.get_parent()
        if node is MaszynaIncludeNode:
            return false
    return true


func list_trainsets() -> void:
    for row:Node in %Trainsets.get_children():
        row.queue_free()
    _profile_queue.clear()
    if not _scene_root:
        return
    var trainsets:Array[RID] = []
    for node:Node in _scene_root.find_children("", "TrainSet3D", true, false):
        trainsets.append((node as TrainSet3D).get_rid())
    for include_id:int in _include_ids:
        trainsets.append_array((instance_from_id(include_id) as MaszynaIncludeNode).get_trainsets())
    for trainset:RID in trainsets:
        var trainset_name:String = RailVehicleServer.trainset_get_name(trainset)
        if not trainset_name:
            continue
        var vehicles:Array[RID] = []
        vehicles.assign(RailVehicleServer.trainset_get_vehicles(trainset))
        var row:TrainsetRow = TRAINSET_ROW.instantiate()
        %Trainsets.add_child(row)
        row.set_trainset(trainset_name, vehicles)
        row.show_requested.connect(show_trainset)
        row.start_vehicle_requested.connect(set_start_vehicle)
        row.set_start_vehicle_enabled(not _find_player(null) == null)
        var file_names:Array[String] = []
        for vehicle:RID in vehicles:
            if MaszynaLegacyVehicleSystem.vehicle_exists(vehicle):
                file_names.append(MaszynaLegacyVehicleSystem.vehicle_get_dynamic(vehicle).file_name)
        row.set_vehicle_types(file_names)
        for vehicle:RID in vehicles:
            if MaszynaLegacyVehicleSystem.vehicle_exists(vehicle):
                var dynamic:MaszynaDynamicData = MaszynaLegacyVehicleSystem.vehicle_get_dynamic(vehicle)
                var tooltip:String = "%s (%s)" % [VehicleServer.vehicle_get_name(vehicle), dynamic.file_name]
                _profile_queue.enqueue(dynamic.data_path, dynamic.file_name, dynamic.skin, dynamic.vehicle_id,
                        row.show_profile.bind(row.add_vehicle_profile(tooltip),
                                MaszynaVehicleProfileManager.get_profile_coupling_width(
                                        dynamic.data_path, dynamic.file_name)))


func _on_spawn_button_pressed() -> void:
    spawn_requested.emit()


func show_trainset(vehicles:Array[RID]) -> void:
    _free_show_markers()
    var selection:EditorSelection = EditorInterface.get_selection()
    selection.clear()
    for vehicle:RID in vehicles:
        var marker:Node3D = Node3D.new()
        _scene_root.add_child(marker, false, Node.INTERNAL_MODE_BACK)
        marker.global_transform = RailVehicleRenderingServer.vehicle_get_transform(vehicle)
        _show_markers.append(marker)
        selection.add_node(marker)
    MaszynaEditorViewFocus.focus_selection()


## The scene's player starts in the trainset's first vehicle with a cab (the first vehicle when
## none has one) - as one action to undo
func set_start_vehicle(vehicles:Array[RID]) -> void:
    var player:MaszynaPlayer = _find_player(null)
    if not player or not vehicles:
        return
    var vehicle:RID = vehicles[0]
    # CabinSystem is not a tool script: in the editor only RailVehicleServer knows the cabins
    for candidate:RID in vehicles:
        if (RailVehicleServer.vehicle_get_front_cabin(candidate).is_valid()
                or RailVehicleServer.vehicle_get_rear_cabin(candidate).is_valid()):
            vehicle = candidate
            break
    var vehicle_id:String = VehicleServer.vehicle_get_id(vehicle)
    if not vehicle_id:
        push_warning("Trainsets: vehicle '%s' has no id to start in" % VehicleServer.vehicle_get_name(vehicle))
        return
    var undo_redo:EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
    undo_redo.create_action("Set start vehicle %s" % vehicle_id)
    undo_redo.add_do_property(player, "start_vehicle_id", vehicle_id)
    undo_redo.add_undo_property(player, "start_vehicle_id", player.start_vehicle_id)
    undo_redo.commit_action()


## The edited scene's player, [param leaving] (a node leaving the tree) left out; null when it has none
func _find_player(leaving:Node) -> MaszynaPlayer:
    if not _scene_root:
        return null
    var nodes:Array[Node] = _scene_root.find_children("*", "", true, false)
    nodes.append(_scene_root)
    for node:Node in nodes:
        if node is MaszynaPlayer and not node == leaving:
            return node
    return null


## A player coming into or leaving the edited scene turns "Set as start vehicle" on or off
func _on_node_added(node:Node) -> void:
    if node is MaszynaPlayer:
        _set_start_vehicle_enabled(not _find_player(null) == null)


func _on_node_removed(node:Node) -> void:
    if node is MaszynaPlayer:
        _set_start_vehicle_enabled(not _find_player(node) == null)


func _set_start_vehicle_enabled(enabled:bool) -> void:
    for row:Node in %Trainsets.get_children():
        (row as TrainsetRow).set_start_vehicle_enabled(enabled)


func _free_show_markers() -> void:
    for marker:Node3D in _show_markers:
        if is_instance_valid(marker):
            marker.queue_free()
    _show_markers.clear()
