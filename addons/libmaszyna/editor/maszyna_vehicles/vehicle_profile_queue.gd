@tool
extends RefCounted

## Side views of vehicles rendered one after another for an editor panel (a render takes a frame),
## each handed to the method that shows it - a method of a node freed meanwhile takes nothing


## A vehicle's side view to render, and the method it goes to
class ProfileLoad:
    var data_path:String
    var file_name:String
    var skin:String
    var vehicle_name:String
    var show:Callable

    func _init(p_data_path:String, p_file_name:String, p_skin:String, p_vehicle_name:String,
            p_show:Callable) -> void:
        data_path = p_data_path
        file_name = p_file_name
        skin = p_skin
        vehicle_name = p_vehicle_name
        show = p_show


var _profile_loads:Array[ProfileLoad] = []
var _loading_profiles:bool = false


## The vehicle's side view goes to show(texture) once it is rendered
func enqueue(data_path:String, file_name:String, skin:String, vehicle_name:String, show:Callable) -> void:
    _profile_loads.append(ProfileLoad.new(data_path, file_name, skin, vehicle_name, show))
    if _loading_profiles:
        return
    _loading_profiles = true
    while _profile_loads:
        var profile_load:ProfileLoad = _profile_loads.pop_front()
        var texture:Texture2D = await MaszynaVehicleProfileManager.get_profile(
                profile_load.data_path, profile_load.file_name, profile_load.skin, profile_load.vehicle_name)
        if texture and profile_load.show.is_valid():
            profile_load.show.call(texture)
    _loading_profiles = false


## The side views not rendered yet are not wanted any more
func clear() -> void:
    _profile_loads.clear()
