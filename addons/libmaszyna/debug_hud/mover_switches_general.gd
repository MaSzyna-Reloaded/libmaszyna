extends "mover_switches_section.gd"

## TMoverParameters::DamageFlag bits (MOVER.h:125-136); a bit shared by a locomotive's and a wagon's
## meaning carries both names
const DAMAGE_NAMES:Dictionary[int, String] = {
    1: "thin wheel / load shift",
    2: "wheel wear",
    4: "bearing",
    8: "coupling",
    16: "ventilator / load damage",
    32: "engine / load destroyed",
    64: "axle",
    128: "derailed",
    256: "pantograph",
}
## N to kN for the couplers' force
const NEWTONS_PER_KILONEWTON:float = 1000.0
## The radio's volume 0..1 shown in percent
const PERCENT:float = 100.0


func _on_refresh_timer_timeout() -> void:
    if not target_vehicle.is_valid():
        return
    %Speed.value = VehicleServer.vehicle_get_speed(target_vehicle)
    var state:Dictionary = VehicleServer.vehicle_dump_state(target_vehicle)
    %Channel.text = tr("Channel: %d") % state.get("radio_channel", 0)
    %Volume.text = tr("Volume: %d%%") % roundi(PERCENT * float(state.get("radio_volume", 0.0)))
    var damage:int = state.get("train_damage", 0)
    var names:PackedStringArray = []
    for bit:int in DAMAGE_NAMES:
        if damage & bit:
            names.append(DAMAGE_NAMES[bit])
    %Damage.text = tr("Damage: %s") % (", ".join(names) if names else tr("none"))
    %Couplers.text = tr("Couplers: front %.3f m %.0f kN, rear %.3f m %.0f kN") % [
        state.get("coupler_front_distance", 0.0), state.get("coupler_front_force", 0.0) / NEWTONS_PER_KILONEWTON,
        state.get("coupler_rear_distance", 0.0), state.get("coupler_rear_force", 0.0) / NEWTONS_PER_KILONEWTON,
    ]
