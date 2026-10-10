@tool
extends VBoxContainer

## The trainsets of the edited scene - those its sceneries placed (RailVehicleServer's, by their
## handles) and those assembled by hand (TrainSet3D) - each with the side views of its vehicles,
## listed again whenever one of the scene's sceneries has loaded. "Show" centres the 3D view on the
## trainset's vehicles. A scenery's `dynamic` outside a `trainset` has no name and is not listed.
## "Spawn..." asks for the browser of vehicles to add one to the scene.

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
        # the sceneries the scene itself holds; the includes of a scenery are its content
        var includes:Array[Node] = root.find_children("", "MaszynaIncludeNode", true, true)
        if root is MaszynaIncludeNode:
            includes.append(root)
        for node:Node in includes:
            (node as MaszynaIncludeNode).loaded.connect(list_trainsets)
            _include_ids.append(node.get_instance_id())
    list_trainsets()


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
        for vehicle:RID in vehicles:
            if MaszynaLegacyVehicleSystem.vehicle_exists(vehicle):
                var dynamic:MaszynaDynamicData = MaszynaLegacyVehicleSystem.vehicle_get_dynamic(vehicle)
                var tooltip:String = "%s (%s)" % [dynamic.name, dynamic.file_name]
                _profile_queue.enqueue(dynamic.data_path, dynamic.file_name, dynamic.skin, dynamic.name,
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


func _free_show_markers() -> void:
    for marker:Node3D in _show_markers:
        if is_instance_valid(marker):
            marker.queue_free()
    _show_markers.clear()
