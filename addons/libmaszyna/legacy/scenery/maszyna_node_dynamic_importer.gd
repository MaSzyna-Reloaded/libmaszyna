@tool
extends RefCounted

## The furthest a vehicle may stand from the previous one and still be coupled to it [m]
## (simulationstateserializer.cpp:1028)
const MAX_COUPLING_OFFSET:float = 0.5
## The coupling of a `dynamic` outside a `trainset`, which has no couplingdata
const LONE_COUPLING:String = "3"

## Reads a `dynamic` (deserialize_dynamic(), simulationstateserializer.cpp) into the trainset it
## is a vehicle of. Field order: datafolder, skinfile, mmdfile, [pathname - only when not inside a
## trainset], offset, drivertype, [couplingdata - only inside a trainset], [velocity - only when
## not inside a trainset], loadcount, [loadtype if loadcount != 0], optional trailing destination,
## "enddynamic".
##
## Inside a trainset the vehicle stands where the trainset puts it: the original computes
## `offset == -1.0 ? trainset.offset : trainset.offset - offset`, then decrements trainset.offset
## by the vehicle's own length for the next vehicle (RailVehicleServer.trainset_place()), with the
## vehicle's `offset` as its gap. Outside one it is a trainset of its own on `pathname`, its front
## at `-offset`: the same sum from an offset of 0.
##
## offset == -1.0 is also the original's sentinel for "place this vehicle reversed"
## (simulationstateserializer.cpp:983, `vehicle->Init(..., ( offset == -1.0 ), params)`, and
## DynObj.cpp:1807, `iDirection = (Reversed ? 0 : 1)`).
func import(p:MaszynaParser, context: MaszynaImporterContext) -> MaszynaDynamicData:
    var data_folder:String = _resolve_data_path(p.next_token().replace("\\", "/"))
    data_folder = MaszynaDataPath.resolve(UserSettings.get_maszyna_game_dir(), data_folder)
    var skin_file:String = p.next_token()
    var mmd_file:String = p.next_token()
    var trainset:MaszynaTrainsetData = context.trainset
    if not trainset:
        trainset = MaszynaTrainsetData.new()
        trainset.track_name = p.next_token()
        context.trainsets.append(trainset)
    var offset:float = float(p.next_token())
    var driver_type:String = p.next_token()
    var coupling_data:String = p.next_token() if context.trainset else LONE_COUPLING
    var velocity:float = trainset.velocity if context.trainset else float(p.next_token())
    var load_count:int = int(p.next_token())
    # a load with no type named is not a load (simulationstateserializer.cpp:1031)
    var load_type:String = p.next_token() if load_count != 0 else ""
    if load_type == "enddynamic":
        load_count = 0
        load_type = ""

    var reversed:bool = is_equal_approx(offset, -1.0)

    var dynamic:MaszynaDynamicData = MaszynaDynamicData.new()
    dynamic.data_path = data_folder
    dynamic.file_name = mmd_file
    dynamic.skin = skin_file
    dynamic.direction = TrackServer.DIRECTION_REVERSED if reversed else TrackServer.DIRECTION_NORMAL
    dynamic.gap = 0.0 if reversed else offset
    dynamic.coupling = _parse_coupling(coupling_data, offset, reversed)
    dynamic.velocity = velocity
    # DynObj.cpp:1812-1825 - headdriver occupies cab 1, reardriver cab 2 (-1), anything else none.
    dynamic.driver_type = (
        MaszynaDynamicData.DriverType.DRIVER_HEAD if driver_type == "headdriver"
        else MaszynaDynamicData.DriverType.DRIVER_REAR if driver_type == "reardriver"
        else MaszynaDynamicData.DriverType.DRIVER_NOBODY
    )
    dynamic.load_name = load_type
    dynamic.load_amount = float(load_count)
    trainset.dynamics.append(dynamic)

    var next_token:String = p.next_token()
    if not next_token == "enddynamic":
        # optional trailing destination parameter, not used yet
        p.get_tokens_until("enddynamic")

    return dynamic


## Coupling type with the next vehicle of the trainset (simulationstateserializer.cpp:921-934):
## the number before an optional "." parameter list, negative means a permanent coupling, and a
## vehicle placed further than 0.5 m from the previous one isn't coupled at all.
func _parse_coupling(coupling_data:String, offset:float, reversed:bool) -> int:
    var coupling:int = int(coupling_data.get_slice(".", 0))
    if coupling < 0:
        coupling = -coupling | RailVehicleController.COUPLING_FLAG_PERMANENT
    if not reversed and absf(offset) > MAX_COUPLING_OFFSET:
        coupling = 0
    return coupling


## Same convention as maszyna_node_model_importer.gd's data_path handling: the .scn token gives
## a path relative to the "dynamic" data root (e.g. "pkp/303e_v1"), not a full path - prepend
## "dynamic" when it isn't already there.
func _resolve_data_path(data_folder:String) -> String:
    var data_path_array:Array = data_folder.split("/")
    if not data_path_array or not String(data_path_array[0]).to_lower() == "dynamic":
        data_path_array.insert(0, "dynamic")
    return "/".join(data_path_array)
