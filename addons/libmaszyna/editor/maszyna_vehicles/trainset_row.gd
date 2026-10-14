@tool
extends HBoxContainer

## One trainset of the edited scene: its name, its vehicles' types in words, "Show" and the side
## views of its vehicles in the order they stand

## "Show" was pressed: the 3D view is to go to the trainset's vehicles (RailVehicleServer's)
signal show_requested(vehicles:Array[RID])
## "Set as start vehicle" was pressed: the scene's player is to start in one of these vehicles
signal start_vehicle_requested(vehicles:Array[RID])

## Height of a vehicle's side view [px]
const PROFILE_HEIGHT:float = 32.0

## The trainset's vehicles, in the order they stand
var _vehicles:Array[RID] = []


func set_trainset(trainset_name:String, vehicles:Array[RID]) -> void:
    %Name.text = trainset_name
    _vehicles = vehicles


## The vehicles' types (their .mmd/.fiz file names) in the order they stand, a run of one type as
## "N× type": "4e-tv + 3× 111a"
func set_vehicle_types(file_names:Array[String]) -> void:
    var parts:Array[String] = []
    var index:int = 0
    while index < file_names.size():
        var run_end:int = index
        while run_end + 1 < file_names.size() and file_names[run_end + 1] == file_names[index]:
            run_end += 1
        var count:int = run_end - index + 1
        parts.append(file_names[index] if count == 1 else "%d× %s" % [count, file_names[index]])
        index = run_end + 1
    %VehicleTypes.text = " + ".join(parts)
    %VehicleTypes.tooltip_text = %VehicleTypes.text


## The place of the next vehicle's side view, empty until the view is rendered
func add_vehicle_profile(tooltip:String) -> TextureRect:
    var place:Control = Control.new()
    place.custom_minimum_size = Vector2(PROFILE_HEIGHT, PROFILE_HEIGHT)
    place.tooltip_text = tooltip
    var profile:TextureRect = TextureRect.new()
    profile.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
    profile.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
    profile.set_anchors_preset(Control.PRESET_FULL_RECT)
    profile.mouse_filter = Control.MOUSE_FILTER_IGNORE
    place.add_child(profile)
    %Vehicles.add_child(place)
    return profile


## The side view in its place, which is as wide as the vehicle is long over the buffers
## (MaszynaVehicleProfileManager.get_profile_coupling_width()) - the view overhangs it onto the
## neighbours when it is longer, as the shared bogies of an articulated unit do
func show_profile(texture:Texture2D, profile:TextureRect, coupling_width:float) -> void:
    profile.texture = texture
    var display_scale:float = PROFILE_HEIGHT / float(texture.get_height())
    var width:float = texture.get_width() * display_scale
    var place_width:float = coupling_width * display_scale if coupling_width > 0.0 else width
    profile.offset_left = -(width - place_width) * 0.5
    profile.offset_right = (width - place_width) * 0.5
    (profile.get_parent() as Control).custom_minimum_size = Vector2(place_width, PROFILE_HEIGHT)


func _on_show_pressed() -> void:
    show_requested.emit(_vehicles)


## "Set as start vehicle" can be pressed only while the scene has a player to start
func set_start_vehicle_enabled(enabled:bool) -> void:
    %StartVehicle.disabled = not enabled


func _on_start_vehicle_pressed() -> void:
    start_vehicle_requested.emit(_vehicles)
