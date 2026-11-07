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
func import(p:MaszynaParser, context: MaszynaImporterContext, vehicle_name:String) -> MaszynaVehicleData:
    var data_folder:String = _resolve_data_path(p.next_token().replace("\\", "/"))
    data_folder = MaszynaDataPath.resolve(MaszynaGameDirectory.get_game_dir(), data_folder)
    var skin_file:String = p.next_token()
    var mmd_file:String = p.next_token()
    var trainset:TrainsetData = context.trainset
    if not trainset:
        trainset = TrainsetData.new()
        # in lower case, as the track's name (simulationstateserializer.cpp:993-995)
        trainset.track_name = p.next_token().to_lower()
        context.trainsets.append(trainset)
    var offset:float = float(p.next_token())
    var driver_type:String = p.next_token()
    var coupling_data:String = p.next_token() if context.trainset else LONE_COUPLING
    # outside a trainset the vehicle's own velocity is its trainset's
    if not context.trainset:
        trainset.velocity = float(p.next_token())
    var load_count:int = int(p.next_token())
    # a load with no type named is not a load (simulationstateserializer.cpp:1031)
    var load_type:String = p.next_token() if load_count != 0 else ""
    if load_type == "enddynamic":
        load_count = 0
        load_type = ""

    var reversed:bool = is_equal_approx(offset, -1.0)

    var dynamic:MaszynaVehicleData = MaszynaVehicleData.new()
    dynamic.data_path = data_folder
    dynamic.file_name = mmd_file
    dynamic.skin = skin_file
    # DynObj.cpp:1812-1825 - headdriver occupies cab 1, reardriver cab 2 (-1), anything else none.
    dynamic.driver_type = (
        MaszynaVehicleData.DriverType.DRIVER_HEAD if driver_type == "headdriver"
        else MaszynaVehicleData.DriverType.DRIVER_REAR if driver_type == "reardriver"
        else MaszynaVehicleData.DriverType.DRIVER_NOBODY
    )
    dynamic.load_name = load_type
    dynamic.load_amount = float(load_count)
    var member:TrainsetMemberData = TrainsetMemberData.new()
    member.vehicle = dynamic
    member.direction = TrackServer.DIRECTION_REVERSED if reversed else TrackServer.DIRECTION_NORMAL
    member.gap = 0.0 if reversed else offset
    member.coupling_mode = coupling_mode_of(coupling_data, offset, reversed)
    # what the modes cannot carry is reported, with the file and the vehicle
    var coupling_flags:RailVehicleController.CouplingFlags = _coupling_flags_of(coupling_data)
    if MaszynaMoverCoupling.has_unknown_bits(absi(int(coupling_data.get_slice(".", 0)))):
        push_warning("%s: %s: couplingdata %s has bits with no coupling flag (%s), left out"
                % [context.get_current_file(), vehicle_name, coupling_data, _flag_names(coupling_flags)])
    if coupling_flags and not coupling_flags & RailVehicleController.COUPLING_FLAG_COUPLER:
        push_warning("%s: %s: couplingdata %s (%s) has no coupler, not coupled"
                % [context.get_current_file(), vehicle_name, coupling_data, _flag_names(coupling_flags)])
    trainset.members.append(member)

    var next_token:String = p.next_token()
    if not next_token == "enddynamic":
        # optional trailing destination parameter, not used yet
        p.get_tokens_until("enddynamic")

    return dynamic


## How the vehicle couples to the next one of the trainset, of its `couplingdata`
## (simulationstateserializer.cpp:921-934): the number before an optional "." parameter list is the
## Mover's coupling:: mask (MaszynaMoverCoupling), a negative one permanent, and a vehicle placed
## further than 0.5 m from the previous one isn't coupled at all. The mask is mapped onto the few
## ways a trainset couples (MASZYNA_ORIGINAL_QUIRKS.md, "A trainset's coupling is the Mover's raw
## bits"); a mask without the coupler couples nothing. The one reading of it - the importer's and the
## scenery selector's list (MaszynaSceneryInfo).
static func coupling_mode_of(coupling_data:String, offset:float, reversed:bool) -> RailVehicleController.CouplingMode:
    if not reversed and absf(offset) > MAX_COUPLING_OFFSET:
        return RailVehicleController.COUPLING_MODE_NONE
    var flags:RailVehicleController.CouplingFlags = _coupling_flags_of(coupling_data)
    if not flags & RailVehicleController.COUPLING_FLAG_COUPLER:
        return RailVehicleController.COUPLING_MODE_NONE
    if flags & RailVehicleController.COUPLING_FLAG_PERMANENT:
        return RailVehicleController.COUPLING_MODE_PERMANENT
    if flags == RailVehicleController.COUPLING_FLAG_COUPLER:
        return RailVehicleController.COUPLING_MODE_COUPLER
    if flags == RailVehicleController.COUPLING_FLAG_COUPLER | RailVehicleController.COUPLING_FLAG_BRAKEHOSE:
        return RailVehicleController.COUPLING_MODE_BRAKE
    return RailVehicleController.COUPLING_MODE_FULL


## The core's flags of a couplingdata, a negative one permanent
static func _coupling_flags_of(coupling_data:String) -> RailVehicleController.CouplingFlags:
    var number:int = int(coupling_data.get_slice(".", 0))
    var flags:int = MaszynaMoverCoupling.flags_from_mover(absi(number))
    if number < 0:
        flags |= RailVehicleController.COUPLING_FLAG_PERMANENT
    return flags as RailVehicleController.CouplingFlags


## The flags by their names, as a warning shows them
func _flag_names(flags:RailVehicleController.CouplingFlags) -> String:
    var names:PackedStringArray = []
    for name:String in ClassDB.class_get_enum_constants("RailVehicleController", "CouplingFlags"):
        var flag:int = ClassDB.class_get_integer_constant("RailVehicleController", name)
        if flag and flags & flag:
            names.append(name)
    return " | ".join(names) if names else "COUPLING_FLAG_NONE"


## Same convention as maszyna_node_model_importer.gd's data_path handling: the .scn token gives
## a path relative to the "dynamic" data root (e.g. "pkp/303e_v1"), not a full path - prepend
## "dynamic" when it isn't already there.
func _resolve_data_path(data_folder:String) -> String:
    var data_path_array:Array = data_folder.split("/")
    if not data_path_array or not String(data_path_array[0]).to_lower() == "dynamic":
        data_path_array.insert(0, "dynamic")
    return "/".join(data_path_array)
