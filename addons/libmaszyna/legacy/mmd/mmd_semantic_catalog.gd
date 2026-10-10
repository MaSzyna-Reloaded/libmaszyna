extends RefCounted
class_name MmdSemanticCatalog

## A lamp whose "_on" mesh is several separate pieces - three alerter lamps in one submodel (SM42
## "czuwak_on"), one backlight overlay per gauge - gets a light at each piece, following the
## widget's lit_changed (MmdCabinInstancer._submodel_islands()):
## GLOW - an omni light in the colour of the piece's own texture (maszyna/cabin/instrument_glow_*);
## WIDGET_LIGHT - a copy of the widget's own light, aimed at the driver; the widget itself then
## only switches the meshes, blinks and sounds.
enum IslandLights { GLOW, WIDGET_LIGHT }

## Etap A+B's supported MMD label -> cabin widget mapping. command/state_property/action strings
## are copied from demo/vehicles/sm42/sm_42_cabin.tscn's already-shipped, already-working
## hand-authored wiring - those are wrapper-API choices (command names, state keys), not visual
## data, so they're the same for every vehicle regardless of cabin appearance.
##
## Animation SHAPE (how far a lever rotates/slides) is deliberately NOT stored here - it is
## computed per vehicle from that vehicle's own MMD `rot`/`mov` line by
## MmdCabinInstancer._apply_animation_shape(), because cabin geometry differs between vehicles
## (confirmed: ST44 and SM42 use different scale values for the same label) and a single fixed
## constant can only ever be right for the one vehicle it was copied from. The original engine's
## own TGauge formula (`scaled = value*scale + offset`, then `rot` applies `scaled*360°`) is
## evaluated at value=1 for a fixed "pushed"/"per-unit" target - confirmed against real data
## (section 4.3 of the feasibility doc: "instrumentlight_sw ... rot -0.2" means -72° at value 1,
## i.e. -0.2*360). This holds for any state property whose domain matches what MMD assumes
## (raw switch positions, physical pressures/speeds) - the one confirmed exception is
## brakectrl, bound to our wrapper's own normalized (0..1) brake position, a different numeric
## domain than MaSzyna's raw brake-handle units MMD's scale is calibrated against, so its
## MMD-derived shape may not be visually exact until the wrapper exposes a raw equivalent.
##
## Any MMD label not listed here gets no widget and no animation (see
## MmdImportContext.warn_unsupported_label) - there is no correct state source to guess from,
## so nothing is built rather than something wrong.
##
## `requires_gauge` marks a control whose original handler refuses the command when the cab has no
## gauge for it (TTrain::OnCommand_*: `ggX.SubModel == nullptr`) - such a cab takes no key for it
## (LegacyCabinUnmodelledControls).

static var _catalog:Dictionary = {}
static var _built:bool = false


static func _ensure_built() -> void:
    if _built:
        return
    _built = true

    _catalog = {
        "mainctrl": {
            # the original's handler acts on mvControlled
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": 0,
                "switch_max_position": 10,
                "command_increase": "main_controller_increase",
                "command_decrease": "main_controller_decrease",
                "state_property": "master_controller_position",
                "action_increase": "main_controller_increase",
                "action_decrease": "main_controller_decrease",
                # OnCommand_mastercontroller* act on key repeat too (Train.cpp:1096)
                "repeat_on_hold": true,
            },
            # with a coupled controller the shunt steps follow the main positions (Train.cpp:985)
            "config_max_property": "master_controller_position_max",
            "mesh_path_field": "mesh_path",
        },
        # shunt (field weakening) controller: Train.cpp:10023 "scndctrl:" -> ggScndCtrl,
        # OnCommand_secondcontrollerincrease/decrease (Train.cpp:1188, 1349), Num / and Num *
        "scndctrl": {
            # the original's handler acts on mvControlled
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": 0,
                "switch_max_position": 10,
                "command_increase": "second_controller_increase",
                "command_decrease": "second_controller_decrease",
                "state_property": "controller_second_position",
                "action_increase": "second_controller_increase",
                "action_decrease": "second_controller_decrease",
                # toggle type acts on key repeat too (Train.cpp:1210)
                "repeat_on_hold": true,
            },
            "config_max_property": "second_controller_position_max",
            "mesh_path_field": "mesh_path",
        },
        # jointctrl (combined main controller + local brake handle, e.g. SM42's nastawnik): the
        # negative range is the local brake (Train.cpp:7699-7714, shown by controller_joint_position).
        # Increase/decrease are handled by LegacyCabinJointController, not forwarded directly.
        "jointctrl": {
            "widget_class": CabinSwitch,
            "fixed_fields": {
                # -LocalBrakePosNo (hamulce.h:45)
                "switch_min_position": -10,
                "switch_max_position": 10,
                "command_increase": "main_controller_increase",
                "command_decrease": "main_controller_decrease",
                "state_property": "controller_joint_position",
                "action_increase": "main_controller_increase",
                "action_decrease": "main_controller_decrease",
                # OnCommand_mastercontroller* act on key repeat too (Train.cpp:1096)
                "repeat_on_hold": true,
            },
            "config_max_property": "main_controller_position_max",
            "mesh_path_field": "mesh_path",
        },
        "dirkey": {
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": -1,
                "switch_max_position": 1,
                "command_increase": "direction_increase",
                "command_decrease": "direction_decrease",
                "state_property": "direction",
                "position_names": {-1: "backward", 0: "neutral", 1: "forward", 2: "forward_high_start"},
                "action_increase": "direction_increase",
                "action_decrease": "direction_decrease",
            },
            "config_max_property": "direction_position_max",
            "mesh_path_field": "mesh_path",
        },
        "brakectrl": {
            "widget_class": CabinKnob,
            "fixed_fields": {
                "value_min": 0.0,
                "value_max": 1.0,
                "command": "brake_level_set",
                "state_property": "brake_controller_position_normalized",
                "action_increase": "brake_level_increase",
                "action_decrease": "brake_level_decrease",
                # a handle that steps takes a position per key press (Train.cpp:1964-1965)
                "command_increase": "brake_level_increase",
                "command_decrease": "brake_level_decrease",
                # Quirk: the grip heuristic of CabinHUDMouseSystem got SM42's valve backwards - its
                # handle swings first down, then to the right towards full braking. Down and right
                # brake, as the original's slider does for the train brake (mouse_slider,
                # drivermouseinput.cpp:98-103: up releases).
                "mouse_drag_signs": Vector2(1.0, 1.0),
            },
            # the valve's named positions, where its handle type puts them (hamulce.h:157-171);
            # the later one wins where two share a position (FV4a: cutoff is also its minimum)
            "position_names_config": {
                "brakes_controller_position_filling": "filling",
                "brakes_controller_position_drive": "drive",
                "brakes_controller_position_cutoff": "cutoff",
                "brakes_controller_position_first_step": "first braking step",
                "brakes_controller_position_full": "full braking",
                "brakes_controller_position_emergency": "emergency braking",
            },
            "config_max_property": "",
            # the vehicle's handle steps a position per key press, or moves while the key is held
            "key_stepped_from_config": "brake_handle_movement",
            "mesh_path_field": "mesh_path",
            # brake_level_set expects a normalized 0..1 level (RailVehicleBrake.cpp converts it back to
            # raw internally), so the widget's own value/command domain has to stay normalized -
            # but MMD's scale is calibrated against the raw handle range (RailVehicleBrake.cpp's
            # fBrakeCtrlPos), so the animation shape needs rescaling by that same raw range or the
            # lever visibly over/under-rotates. See _apply_animation_shape()'s doc comment.
            "animation_range_config_properties": ["brakes_controller_position_min", "brakes_controller_position_max"],
        },
        # "localbrake:" - independent/loco brake handle (Train.cpp:10026, ggLocalBrake), a
        # draggable gauge like mainctrl/brakectrl, not a passive display. Real input is
        # OnCommand_independentbrakeincrease/decrease (Train.cpp:1447-1524), bound by default to
        # num_1/num_7 - there was previously no equivalent command anywhere in this wrapper, so
        # the handle could never move. Unlike brakectrl, local_brake_set takes an
        # already-normalized 0..1 level directly (LocalBrakePosA is normalized in the mover
        # itself, no raw Handle-position range to rescale against).
        "localbrake": {
            "widget_class": CabinKnob,
            "fixed_fields": {
                "value_min": 0.0,
                "value_max": 1.0,
                "command": "local_brake_set",
                "state_property": "brake_local_position_normalized",
                "action_increase": "local_brake_increase",
                "action_decrease": "local_brake_decrease",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
            # The original feeds the gauge LocalBrakePosA * LocalBrakePosNo (Train.cpp:7850,
            # LocalBrakePosNo = 10 in hamulce.h:39), so MMD scale is calibrated for 0..10.
            "mmd_scale_multiplier": 10.0,
        },
        # "manualbrake:" gauge shows ManualBrakePos (Train.cpp:10255); mouse drives
        # manualbrakeincrease/decrease (drivermouseinput.cpp:547), keys Ctrl+Num1/Ctrl+Num7
        # (driverkeyboardinput.cpp:64-65), acting on key repeat too (Train.cpp:1811).
        "manualbrake": {
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": 0,
                "switch_max_position": 20,
                "command_increase": "manual_brake_increase",
                "command_decrease": "manual_brake_decrease",
                "state_property": "brake_manual_position",
                "action_increase": "manual_brake_increase",
                "action_decrease": "manual_brake_decrease",
                "repeat_on_hold": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "security_reset_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "security_acknowledge",
                "action": "security_acknowledge",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Confirmed against the original engine (Train.cpp:4061-4080,
        # OnCommand_motoroverloadrelayreset -> MoverParameters->FuseOn(), "zbij nadmiarowy") and
        # Train.cpp:10052 ("fuse_bt:" -> ggFuseButton). No controller_mode override, no
        # state_property, same shape as security_reset_bt/releaser_bt above - fuse_reset() takes
        # no arguments and isn't a persistent toggle.
        "fuse_bt": {
            # the original's handler acts on mvControlled
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "fuse_reset",
                "action": "fuse_reset",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Converter-specific counterpart to fuse_bt above - confirmed against the original engine
        # (Train.cpp:3567-3585, OnCommand_converteroverloadrelayreset ->
        # RelayReset(relay_t::primaryconverteroverload), and Train.cpp:10053
        # "converterfuse_bt:" -> ggConverterFuseButton). state_property reuses the existing
        # converter_overload reading (RailVehicleEngine.cpp - MoverParameters->ConvOvldFlag).
        # Confirmed against Train.cpp:1917-1939 (OnCommand_sandboxactivate) and Train.cpp:10044
        # ("sand_bt:" -> ggSandButton) - momentary, matching the original's press/release shape
        # (sand only while held), same as fuse_bt/converterfuse_bt above.
        "sand_bt": {
            # OnCommand_sandboxactivate refuses it without the gauge (Train.cpp:2284)
            "requires_gauge": true,
            # the original's handler acts on mvControlled
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "sand",
                "state_property": "sand_active",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Confirmed against Train.cpp:5256-5296 (OnCommand_heatingtoggle/enable/disable) and
        # Train.cpp:10116 ("trainheating_sw:" -> ggTrainHeatingButton) - "_sw" (switch), persistent
        # toggle like compressor_sw/converter_sw.
        "trainheating_sw": {
            # OnCommand_heatingtoggle refuses it without the gauge (Train.cpp:6661)
            "requires_gauge": true,
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggTrainHeatingButton.type(), Train.cpp:6687)
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "command": "heating",
                "state_property": "heating_enabled",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Confirmed against Train.cpp:1839-1872 (OnCommand_alarmchaintoggle/enable/disable) and
        # Train.cpp:10027 ("alarmchain:" -> ggAlarmChain) - manual emergency brake pull cord,
        # persistent pulled/released state (no "_sw"/"_bt" suffix - a cord, not a rotary switch).
        "alarmchain": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "alarm_chain",
                "state_property": "alarm_chain_pulled",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The open motor connectors button (Train.cpp:10054 "stlinoff_bt:" -> ggStLinOffButton):
        # held down it opens them, released it closes them again, unless the vehicle's button is a
        # toggle (Switches: MotorConnectors=toggle, OnCommand_motorconnectorsopen, Train.cpp:5025-5055)
        "stlinoff_bt": {
            # OnCommand_motorconnectorsopen/close refuse it without the gauge (Train.cpp:5015, 5062)
            "requires_gauge": true,
            # the original's handler acts on mvControlled
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinButton,
            "monostable_from_config": "motor_connectors_switch_impulse",
            "fixed_fields": {
                "monostable": true,
                "command": "motor_connectors_open",
                "state_property": "motor_connectors_open",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "converterfuse_bt": {
            # the original's handler acts on mvControlled
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "converter_fuse_reset",
                "state_property": "converter_overload",
                "action": "converter_fuse_reset",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "releaser_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "brake_releaser",
                "action": "brake_release",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Confirmed against Train.cpp's own cabin gauge dispatch table (horn_bt:/hornlow_bt:/
        # hornhigh_bt:/whistle_bt: -> ggHornButton/ggHornLowButton/ggHornHighButton/
        # ggWhistleButton) and RailVehicleHorns' own low/high/whistle command+state model (see
        # RailVehicleHorns.hpp) - state_property is the RAW commanded press (unaffected by the
        # emergency-brake override), matching the original's UpdateValue() calls firing straight
        # from the command handler, not the combined "_active" (sound-triggering) state.
        # hornlow_bt:/hornhigh_bt: are the dedicated per-slot buttons (present together on ~80
        # real vehicles). action points at dedicated "horn_low"/"horn_high"/"whistle" InputMap
        # actions (demo/project.godot) - matching RailVehicleHorns' own command naming, not sm42_v1's
        # older horn1/horn2 shim naming (that scene's own action_increase/action_decrease were
        # updated to match).
        #
        # horn_bt: is the single SHARED button used instead on the far more common (~220 real
        # vehicles) case where a vehicle has only one physical horn control. Confirmed real:
        # OnCommand_hornlowactivate's AND OnCommand_hornhighactivate's own null-checks
        # (`ggHornButton == nullptr && ggHornLow/HighButton == nullptr`) both pass as soon as
        # ggHornButton alone exists - so in the original engine ONE horn_bt: button already
        # responds to BOTH low and high (swinging the same gauge to -1.0/+1.0 depending on which
        # was pressed), not low-only. Modeled as a CabinSwitch exactly like SM42's own
        # hand-authored "Horn" node (demo/vehicles/sm42/sm_42_cabin.tscn) - a single bidirectional
        # lever whose two positions hold horn_low and horn_high, the original's two commands -
        # rather than CabinButton, which can only carry one command and would leave one of the
        # two keys permanently dead whenever only horn_bt: exists.
        "horn_bt": {
            # OnCommand_hornlow/hornhighactivate refuse it without horn_bt or their own button (Train.cpp:7934, 7978)
            "requires_gauge": true,
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": -1,
                "switch_max_position": 1,
                "automatic_reset": true,
                "position_commands": {1: "horn_low", -1: "horn_high"},
                "state_property": "horn",
                "action_increase": "horn_low",
                "action_decrease": "horn_high",
                "position_names": {-1: "high tone", 0: "off", 1: "low tone"},
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "hornlow_bt": {
            # OnCommand_hornlowactivate refuses it without horn_bt or hornlow_bt (Train.cpp:7934)
            "requires_gauge": true,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "horn_low",
                "state_property": "horn_low_pressed",
                "action": "horn_low",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "hornhigh_bt": {
            # OnCommand_hornhighactivate refuses it without horn_bt or hornhigh_bt (Train.cpp:7978)
            "requires_gauge": true,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "horn_high",
                "state_property": "horn_high_pressed",
                "action": "horn_high",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "whistle_bt": {
            # OnCommand_whistleactivate refuses it without the gauge (Train.cpp:8022)
            "requires_gauge": true,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "whistle",
                "state_property": "whistle_pressed",
                "action": "whistle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "main_on_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "main_switch",
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "main_off_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "main_switch",
                "controller_mode": CabinButton.ControllerMode.Off,
                # Ctrl+Shift+M - the original binds no key to opening alone (driverkeyboardinput.cpp:109)
                "action": "main_switch_off",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # main_sw: one line breaker switch instead of the main_on_bt/main_off_bt pair
        # (drivermouseinput.cpp:774 -> linebreakertoggle, Train.cpp:3714). Its behaviour is
        # LegacyCabinMainSwitch; its key M is the behaviour's own keyboard control
        # (LegacyCabinMainSwitch.KEY), as the original's linebreakertoggle acts in every cab
        # (driverkeyboardinput.cpp:108).
        "main_sw": {
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggMainButton.type(), Train.cpp:3733)
            "shape_from_button_type": true,
            # an impulse one rests midway, between its two directions (Train.cpp:11348)
            "push_value_rest": 0.5,
            "fixed_fields": {
                "monostable": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Reverser push buttons (drivermouseinput.cpp:521-529 -> reverserforward/neutral/backward,
        # Train.cpp OnCommand_reverser*). Behaviour: LegacyCabinReverser; the D / R keys reach dirkey.
        "dirforward_bt": {
            "widget_class": CabinButton,
            "fixed_fields": { "monostable": true },
            # its "_on" lamp follows the controlled vehicle's DirActive (m_dir*, Train.cpp:8520, 12037)
            "state_light": {"state_property": "direction", "lit_condition": CabinIndicator3D.LitCondition.POSITIVE},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "dirneutral_bt": {
            "widget_class": CabinButton,
            "fixed_fields": { "monostable": true },
            # its "_on" lamp follows the controlled vehicle's DirActive (m_dir*, Train.cpp:8520, 12037)
            "state_light": {"state_property": "direction", "lit_condition": CabinIndicator3D.LitCondition.ZERO},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "dirbackward_bt": {
            "widget_class": CabinButton,
            "fixed_fields": { "monostable": true },
            # its "_on" lamp follows the controlled vehicle's DirActive (m_dir*, Train.cpp:8520, 12037)
            "state_light": {"state_property": "direction", "lit_condition": CabinIndicator3D.LitCondition.NEGATIVE},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # shp_reset_bt: cab signalling reset of a vehicle with a separate acknowledge button
        # (drivermouseinput.cpp:603 -> cabsignalacknowledge, Train.cpp:2876, Shift+Space).
        "shp_reset_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "security_cabsignal_acknowledge",
                "controller_mode": CabinButton.ControllerMode.On,
                "action": "security_cabsignal_acknowledge",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Spring brake buttons (drivermouseinput.cpp:642-649 -> springbraketoggle/enable/disable).
        # The toggle flips SpringBrake.Activate (Train.cpp:6780); its key is the game's
        # eu07_input-keyboard.ini binding, the original has no default one.
        "springbraketoggle_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "set_spring_brake_active",
                "state_property": "spring_brake/active",
                "action": "spring_brake_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "springbrakeon_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "set_spring_brake_active",
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "springbrakeoff_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "set_spring_brake_active",
                "controller_mode": CabinButton.ControllerMode.Off,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # pantalloff_sw: drops every pantograph (drivermouseinput.cpp:834 -> pantographlowerall,
        # Train.cpp:3336, Ctrl+P).
        "pantalloff_sw": {
            # OnCommand_pantographlowerall refuses it without the gauge (Train.cpp:3342)
            "requires_gauge": true,
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggPantAllDownButton.type(), Train.cpp:3352)
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "command": "pantographs_drop_all",
                "state_property": "current_collector/pantographs_dropped",
                "action": "pantographs_drop_all_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "fuelpump_sw": {
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggFuelPumpButton.type(), Train.cpp:3879)
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": true,
                "command": "fuel_pump",
                "state_property": "fuel_pump_active",
                "action": "fuel_pump_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "oilpump_sw": {
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggOilPumpButton.type(), Train.cpp:3978)
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": true,
                "command": "oil_pump",
                "state_property": "oil_pump_active",
                "action": "oil_pump_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # A diesel's cooling water pump (LegacyCabinPump), W (OnCommand_waterpumptoggle, Train.cpp:4228)
        "waterpump_sw": {
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggWaterPumpButton.type(), Train.cpp:4236)
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": true,
                "state_property": "water_pump_active",
                "action": "water_pump_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The water pump's breaker, Ctrl+W (OnCommand_waterpumpbreakertoggle, Train.cpp:4175)
        "waterpumpbreaker_sw": {
            # the original's handler acts on mvControlled (Train.cpp:4175-4226)
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "water_pump_breaker",
                "state_property": "water_pump_breaker",
                "action": "water_pump_breaker_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The cooling water's heater, Shift+W (OnCommand_waterheatertoggle, Train.cpp:4122)
        "waterheater_sw": {
            # the original's handler acts on mvControlled (Train.cpp:4122-4173)
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "water_heater",
                "state_property": "water_heater_enabled",
                "action": "water_heater_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The water heater's breaker, Ctrl+Shift+W (OnCommand_waterheaterbreakertoggle, Train.cpp:4069)
        "waterheaterbreaker_sw": {
            # the original's handler acts on mvControlled (Train.cpp:4069-4120)
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "water_heater_breaker",
                "state_property": "water_heater_breaker",
                "action": "water_heater_breaker_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The link of the two water circuits, Shift+H (OnCommand_watercircuitslinktoggle, Train.cpp:4327)
        "watercircuitslink_sw": {
            # the original's handler acts on mvControlled (Train.cpp:4327-4378)
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "water_circuits_link",
                "state_property": "water_circuits_link",
                "action": "water_circuits_link_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The traction motors' blowers A (LegacyCabinPump), Shift+N (OnCommand_motorblowerstogglefront, Train.cpp:4750)
        "motorblowersfront_sw": {
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggMotorBlowersFrontButton.type(), Train.cpp:4758)
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": true,
                "state_property": "motor_blowers_front_active",
                "action": "motor_blowers_front_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The traction motors' blowers B (LegacyCabinPump); the original's Shift+M is the line breaker's off here (OnCommand_motorblowerstogglerear, Train.cpp:4848)
        "motorblowersrear_sw": {
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggMotorBlowersRearButton.type(), Train.cpp:4856)
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": true,
                "state_property": "motor_blowers_rear_active",
                "action": "motor_blowers_rear_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # All the traction motors' blowers held off (LegacyCabinMotorBlowersAllOff), Ctrl+M (OnCommand_motorblowersdisableall, Train.cpp:4948)
        "motorblowersalloff_sw": {
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggMotorBlowersAllOffButton.type(), Train.cpp:4956)
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": true,
                "state_property": "motor_blowers_front_disabled",
                "action": "motor_blowers_all_off",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The passengers' compartment lights (LegacyCabinCompartmentLights): one switch, or an on and
        # an off button (OnCommand_compartmentlightstoggle/enable/disable, Train.cpp:6357-6460); the
        # original binds no key
        "compartmentlights_sw": {
            "widget_class": CabinButton,
            # its original handlers branch on the kind of switch (ggCompartmentLightsButton.type(), Train.cpp:6386)
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": true,
                "state_property": "compartment_lights_enabled",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "compartmentlightson_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "compartmentlightsoff_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The cab's own lights (LegacyCabinCabLights): each cab keeps its switches, and the
        # button shows what its cab holds - no vehicle command, no vehicle state
        "instrumentlight_sw": {
            # OnCommand_instrumentlightenable/disable refuse it without the gauge (Train.cpp:6491, 6515)
            "requires_gauge": true,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "action": "devices_light_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # the dashboard and timetable light switches (Train.cpp:11903-11904) - LegacyCabinCabLights
        "dashboardlight_sw": {
            # OnCommand_dashboardlightenable/disable refuse it without the gauge (Train.cpp:6553, 6577)
            "requires_gauge": true,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "timetablelight_sw": {
            # OnCommand_timetablelightenable/disable refuse it without the gauge (Train.cpp:6619, 6643)
            "requires_gauge": true,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "cablight_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "action": "cabin_light_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11923 ggCabLightDimButton -> OnCommand_interiorlightdimtoggle (Train.cpp:6274):
        # the cab light at 0.4 of its level (Train.cpp:9745), Ctrl+'
        "cablightdim_sw": {
            # OnCommand_interiorlightdimenable/disable refuse it without the gauge (Train.cpp:6301, 6332)
            "requires_gauge": true,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "action": "cabin_light_dim_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "radio_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "radio",
                "state_property": "radio_enabled",
                "action": "radio_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Confirmed against the original engine's own source (vehicle/Gauge.cpp:182,
        # `scale *= mul`, called from vehicle/Train.cpp's per-label gauge.Load(...) sites):
        # pressure-family gauge labels (brakepress/brakepressb, pipepress/pipepressb, scndpress,
        # limpipepress, cntrlpress, springbrakepress, epctrlvalue, compressor/compressorb,
        # pantpress, brakes) are loaded with mul=0.1, so MMD's own declared scale must be
        # multiplied by 0.1 before use for these specific labels - every other gauge label
        # (confirmed: tachometer, oilpress) uses the default mul=1.0, i.e. no correction. This is
        # NOT a per-vehicle hardcoded guess - mmd_scale_multiplier is the same fixed correction
        # factor the original engine itself applies for this label, on every vehicle.
        # Train.cpp:10274-10278: tachometer: is the jumpy Hasler needle (AssignFloat(&fTachoVelocityJump)).
        # Train.cpp:12101-12134 - "tachometer:"/"tachometerb:" jerk (fTachoVelocityJump),
        # "tachometern:" moves smoothly and "tachometerd:" is digital (both fTachoVelocity)
        "tachometer": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "tachometer_speed_jump",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        "tachometerb": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "tachometer_speed_jump",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        "tachometern": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "tachometer_speed",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        "tachometerd": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "tachometer_speed",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        # Train.cpp:12334-12347: gauge.AssignFloat(fEngine + 1) / (fEngine + 2), where
        # fEngine[i] = mvControlled->ShowEngineRotation(i) = std::abs(enrot) (Mover.cpp:1998) -
        # the RAW rotations-PER-SECOND value, unmultiplied. Our wrapper's "engine_rpm" is already
        # the human-readable RPM (enrot*60 - confirmed from real state: engine_rpm_count=8.2667,
        # engine_rpm=496.0=8.2667*60), a different domain than what MMD's scale assumes, same
        # class of problem as brakectrl's normalized-vs-raw mismatch. "engine_rpm_count" is the
        # correct binding - it IS enrot, matching the original 1:1. mul=1.0 (default, confirmed -
        # no explicit third Load() argument). The hand-authored sm_42_cabin.tscn wires
        # enrot2m's submodel ("obrot01") to "engine_rpm" (not "_count") with its own hand-picked
        # mesh_rotation, which only works because that scene's rotation value was tuned by hand
        # against the ×60 value, not derived from MMD's scale like this catalog is.
        "enrot1m": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "engine_rpm_count",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        "enrot2m": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "engine_rpm_count",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        # a pressure of a car of the train, named by the numbers before its shape, on the scale of
        # the 0.1 multiplier (Train.cpp:12158-12166) - LegacyCabinTrainsetPressures
        "brakes": {
            "widget_class": CabinGauge,
            "fixed_fields": {},
            "state_property_of_leading_numbers": true,
            "mmd_scale_multiplier": 0.1,
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        "brakepress": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "brake_air_pressure",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
            "mmd_scale_multiplier": 0.1,
        },
        # Train.cpp:12185 - brakepressb: is the same brake cylinder gauge as brakepress:
        "brakepressb": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "brake_air_pressure",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
            "mmd_scale_multiplier": 0.1,
        },
        # Train.cpp:12207 - limpipepress: the control reservoir of the driver's brake valve
        # (m_brakehandlecp = Handle->GetCP(), Train.cpp:8908)
        "limpipepress": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "brake_handle_control_pressure",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
            "mmd_scale_multiplier": 0.1,
        },
        "pipepress": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "pipe_pressure",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
            "mmd_scale_multiplier": 0.1,
        },
        # Confirmed against the original engine's own source (Train.cpp:10487-10492, "hvoltage:"
        # loads a gauge and binds it to fHVoltage, itself computed as
        # max(PantographVoltage, GetTrainsetHighVoltage()) - Train.cpp:6944-6946, see
        # RailVehicleElectricEngine.cpp's own comment on current_collector/voltage). Not in the
        # pressure-family mmd_scale_multiplier list above, so default mul=1.0 like tachometer/
        # enrot/oilpress.
        "hvoltage": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "current_collector/voltage",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        "oilpress": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "oil_pump_pressure",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        # Train.cpp:10407-10412: gauge.Load(Parser, DynamicObject, 0.1); gauge.AssignDouble(
        # &mvOccupied->Compressor) - main reservoir pressure gauge, part of the pressure-family
        # mmd_scale_multiplier=0.1 group named in this section's own header comment above.
        "compressor": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "compressor_pressure",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
            "mmd_scale_multiplier": 0.1,
        },
        # Train.cpp:10438-10443: pantograph tank pressure gauge, AssignDouble(&PantPress), scale 0.1.
        "pantpress": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "current_collector/pantograph_tank_pressure",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
            "mmd_scale_multiplier": 0.1,
        },
        # The original engine's own approach for "i-*:" indicator lights (Train.cpp's TButton) is
        # to show/hide a matching "<submodel>_on"/"<submodel>_off" mesh pair - not reproduced here.
        # Instead this reuses CabinSpotLight3D (already a generic, reusable addon widget - not
        # SM42-specific), positioned at the "czuwak" submodel MMD actually names (see
        # _position_at_submodel()) instead of SM42's own 3 hand-placed "CzuwakOmni" lights (their
        # exact 3D offsets are that specific cab's own hand-tuned art, not derivable from MMD - one
        # light at the submodel's own transform is the closest generic equivalent). Numeric light
        # parameters (color/energy/range/angle/specular/volumetric fog) are copied from
        # CzuwakOmni1 - light_projector (a demo-specific texture asset,
        # res://vehicles/sm42/czuwak_projector.png) is deliberately NOT copied: an
        # addons/libmaszyna/ catalog can't depend on demo/ content.
        #
        # SM42's OWN CzuwakOmni1/2/3 have no state_property at all - the actual flashing there
        # comes from a separate CabinBlinker node ("Czuwak", cabin_blinker.gd) with its own
        # internal Timer, driving a `blink` signal a cabin-script handler uses to toggle those
        # lights externally. blink_time (below) ports that same Timer-based flash directly into
        # CabinSpotLight3D itself instead, so this stays one widget per MMD label.
        # Confirmed against VehicleController.cpp:46/265,429 - exact command+state pair already
        # proven in production via SM42's own hand-authored Battery node.
        # Train.cpp:11928-11929 ggBatteryOnButton/ggBatteryOffButton -> OnCommand_batteryenable/disable
        # (Train.cpp:2939-3070): pressed they switch the battery, then spring back - LegacyCabinBattery
        "batteryon_sw": {
            "widget_class": CabinButton,
            "fixed_fields": { "monostable": true },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "batteryoff_sw": {
            "widget_class": CabinButton,
            "fixed_fields": { "monostable": true },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11829 ggBrakeOperationModeCtrl -> OnCommand_trainbrakeoperationmodeincrease/decrease
        # (Train.cpp:2447-2477), showing log2 of BrakeOpModeFlag; Ctrl+Num2 / Ctrl+Num8
        "brakeopmode_sw": {
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": 0,
                "switch_max_position": 3,
                "command_increase": "brake_operation_mode_increase",
                "command_decrease": "brake_operation_mode_decrease",
                "state_property": "brake_operation_mode_position",
                "action_increase": "brake_operation_mode_increase",
                "action_decrease": "brake_operation_mode_decrease",
            },
            "config_max_property": "brake_operation_mode_position_max",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:12019-12031 - the speed control's buttons, lit while it is active
        # (SpeedCtrlUnit.IsActive); OnCommand_speedcontrol* act on the press (Train.cpp:6887-6975)
        # and have no default key
        "speedinc_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_increase",
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speeddec_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_decrease",
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedctrlpowerinc_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_power_increase",
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedctrlpowerdec_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_power_decrease",
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11914-11916 ggRelayResetButtons -> OnCommand_universalrelayreset (Train.cpp:5175),
        # impulse-only, no key (driverkeyboardinput.cpp:127-129)
        "relayreset1_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "universal_relay_reset",
                "command_param": RailVehicleSwitches.RELAY_RESET_BUTTON_1,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11914-11916 ggRelayResetButtons -> OnCommand_universalrelayreset (Train.cpp:5175),
        # impulse-only, no key (driverkeyboardinput.cpp:127-129)
        "relayreset2_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "universal_relay_reset",
                "command_param": RailVehicleSwitches.RELAY_RESET_BUTTON_2,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11914-11916 ggRelayResetButtons -> OnCommand_universalrelayreset (Train.cpp:5175),
        # impulse-only, no key (driverkeyboardinput.cpp:127-129)
        "relayreset3_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "universal_relay_reset",
                "command_param": RailVehicleSwitches.RELAY_RESET_BUTTON_3,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedbutton0": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_button",
                "command_param": 0,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedbutton1": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_button",
                "command_param": 1,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedbutton2": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_button",
                "command_param": 2,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedbutton3": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_button",
                "command_param": 3,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedbutton4": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_button",
                "command_param": 4,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedbutton5": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_button",
                "command_param": 5,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedbutton6": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_button",
                "command_param": 6,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedbutton7": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_button",
                "command_param": 7,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedbutton8": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_button",
                "command_param": 8,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "speedbutton9": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "speed_control_button",
                "command_param": 9,
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "battery_sw": {
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggBatteryButton.type(), Train.cpp:2929)
            "shape_from_button_type": true,
            # an impulse one rests midway, between its two directions (Train.cpp:11342)
            "push_value_rest": 0.5,
            "fixed_fields": {
                "monostable": false,
                "command": "battery",
                "state_property": "battery_enabled",
                "action": "battery_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:10130 "cabactivation_sw:" (ggCabActivationButton) - OnCommand_cabactivationenable/
        # disable (Train.cpp:2430-2472), default key Ctrl+J (cabactivationtoggle); the switch shows
        # IsCabMaster() (Train.cpp:8020), exposed as cabin_controleable.
        "cabactivation_sw": {
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggCabActivationButton.type(), Train.cpp:3100)
            "shape_from_button_type": true,
            # an impulse one rests midway, between its two directions (Train.cpp:3115)
            "push_value_rest": 0.5,
            "fixed_fields": {
                "monostable": false,
                "command": "cab_activation",
                "state_property": "cabin_controleable",
                "action": "cab_activation_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Confirmed against RailVehicleElectricEngine.cpp:160,172,322 - converter()/converter_enabled.
        # the converter switch and its off switch: an impulse one springs back (Switches:
        # Converter=impulse, OnCommand_convertertoggle/disable, Train.cpp:4382-4458) - LegacyCabinConverter
        "converter_sw": {
            "widget_class": CabinButton,
            "monostable_from_config": "converter_switch_impulse",
            "fixed_fields": {
                "monostable": false,
                "action": "converter_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "converteroff_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The motor overload relay's threshold, high start (Train.cpp:11830 "maxcurrent_sw:" ->
        # ggMaxCurrentCtrl, OnCommand_motoroverloadrelaythresholdsetlow/sethigh -> CurrentSwitch(),
        # Train.cpp:5123-5145). The original's key (Ctrl+F) has no input action yet.
        "maxcurrent_sw": {
            # the original's handler acts on mvControlled
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "motor_overload_relay_threshold",
                "state_property": "motor_overload_relay_high_threshold",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Confirmed against RailVehicleElectricEngine.cpp:159,170,323 - compressor()/compressor_enabled -
        # the switch label (vehicle/Train.cpp:11875, "compressor_sw:" -> ggCompressorButton), not
        # to be confused with "compressor:"/"compressorb:" (the pressure GAUGE, still genuinely
        # missing - no raw pressure value exists in the wrapper, only the enabled/allowed booleans).
        "compressor_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "compressor",
                "state_property": "compressor_enabled",
                "action": "compressor_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Auxiliary pantograph compressor, runs while held (Train.cpp:10114 "pantcompressor_sw:" ->
        # ggPantCompressorButton, OnCommand_pantographcompressoractivate, Shift+V).
        "pantcompressor_sw": {
            # the original's handler acts on mvPantographUnit
            "target": CabinState.Target.PANTOGRAPH_UNIT,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "pantograph_compressor",
                "action": "pantograph_compressor_activate",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Three-way valve feeding the pantographs from the auxiliary compressor instead of the main
        # tank (Train.cpp:10115 "pantcompressorvalve_sw:" -> ggPantCompressorValve,
        # OnCommand_pantographcompressorvalvetoggle, Ctrl+V).
        "pantcompressorvalve_sw": {
            # the original's handler acts on mvControlled
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "pantograph_compressor_valve",
                "state_property": "current_collector/pantograph_compressor_valve",
                "action": "pantograph_compressor_valve_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # wipers_sw: the wiper switch, one position per row of the FIZ WiperList:
        # (Train.cpp:11985 ggWiperSw, drivermouseinput.cpp:1110 wiperswitchincrease/decrease,
        # Train.cpp:2638-2661). The original has no default key for it. The switch and the
        # wipers live in RailVehicleWipers - the vendored Mover has neither.
        "wipers_sw": {
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": 0,
                "command_increase": "wipers_switch_increase",
                "command_decrease": "wipers_switch_decrease",
                "state_property": "wipers_switch_position",
                "action_increase": "wipers_switch_increase",
                "action_decrease": "wipers_switch_decrease",
            },
            "config_max_property": "wipers_switch_position_max",
            "mesh_path_field": "mesh_path",
        },
        # radiochannel_sw: real vehicles carry different physical radio hardware - some (e.g.
        # "Koliber" units) only have separate next/prev channel buttons
        # (radiochannelnext_sw:/radiochannelprev_sw: below), others (e.g. "Radmor" units) have an
        # actual turnable multi-position selector knob under this label - a real interactive
        # control, not just a passive readout, so it needs the same CabinSwitch shape as mainctrl
        # (mouse-turnable + command_increase/decrease), not CabinGauge (display-only, no input).
        # switch_min/max_position match radio_channel_min/max's own real default range
        # (VehicleController.hpp - 1..10, the same range the original engine hardcodes universally
        # in OnCommand_radiochannelset). value_offset=1: confirmed real in-game - channel 1
        # (switch_position=1) was rendering the knob one full step past its physical rest
        # position, and decrease could never visually return to rest, because the knob's own
        # first notch corresponds to switch_position=1, not 0 (channel 0 isn't a valid radio
        # channel at all) - the same "state domain doesn't start where the mesh's rest position
        # is" mismatch as enrot/brakectrl, just an integer shift instead of a scale/unit one.
        "radiochannel_sw": {
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": 1,
                "switch_max_position": 10,
                "value_offset": 1,
                "command_increase": "radio_channel_increase",
                "command_decrease": "radio_channel_decrease",
                "state_property": "radio_channel",
                "action_increase": "radio_channel_increase",
                "action_decrease": "radio_channel_decrease",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Momentary buttons (vehicle/Train.cpp:8103-8135: ggRadioChannelNext/Previous.UpdateValue
        # on press/release, exactly like ggHornButton) - controller_mode=On (not the CabinButton
        # default OnOff) so the command fires exactly ONCE per press, not once on press AND once on
        # release: radio_channel_increase/decrease take an int step, not a persistent on/off state,
        # so a second call on release would double-step the channel. Sending `true` as p1 relies
        # on the same `p_step > 0 ? p_step : 1` guard as CabinSwitch's zero-arg call (both convert
        # to step=1) - confirmed, not a guess, since main_on_bt/main_off_bt already prove
        # ControllerMode.On/Off's single-shot-on-press behavior. No state_property: neither button
        # has a wrapper-tracked "is pressed" readback, same as releaser_bt/security_reset_bt.
        "radiochannelnext_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "radio_channel_increase",
                "controller_mode": CabinButton.ControllerMode.On,
                "action": "radio_channel_increase",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "radiochannelprev_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "radio_channel_decrease",
                "controller_mode": CabinButton.ControllerMode.On,
                "action": "radio_channel_decrease",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Front / rear pantograph switches: Train.cpp:11905-11906 -> OnCommand_pantographtogglefront/
        # rear (drivermouseinput.cpp:855-860), keys P / O (driverkeyboardinput.cpp:201-202).
        # LegacyCabinPantographs owns them - how the valve is operated depends on the vehicle's
        # switch type. state_property: Pantographs[].is_active, index 0 the front one
        # (MOVER.h:154 end { front = 0, rear = 1 }).
        "pantfront_sw": {
            "widget_class": CabinButton,
            "monostable_from_config": "pantograph_switch_impulse",
            "fixed_fields": {
                "monostable": false,
                "state_property": "current_collector/pantograph_first_active",
                "action": "pantograph_front_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "pantrear_sw": {
            "widget_class": CabinButton,
            "monostable_from_config": "pantograph_switch_impulse",
            "fixed_fields": {
                "monostable": false,
                "state_property": "current_collector/pantograph_second_active",
                "action": "pantograph_rear_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The lowering buttons: Train.cpp:11907-11908 -> OnCommand_pantographlowerfront/rear
        # (drivermouseinput.cpp:861-866), owned by LegacyCabinPantographs. With an impulse switch
        # type their presence alone is what lets a pantograph be lowered, which is why a cab may
        # declare them with no submodel (dynamic/pkp/e186_v2/base.mmd.inc:187-188).
        "pantfrontoff_sw": {
            "widget_class": CabinButton,
            "fixed_fields": { "monostable": true },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "pantrearoff_sw": {
            "widget_class": CabinButton,
            "fixed_fields": { "monostable": true },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Confirmed against VehicleController.cpp:422 - internal_state["total_distance"] =
        # p_mover->DistCounter, the exact same field the original engine's own distcounter: gauge
        # binds (vehicle/Train.cpp:12370-12374, gauge.AssignDouble(&mvControlled->DistCounter)) -
        # loaded with no explicit mul argument there, so max_value=1.0 (the same "no correction"
        # convention as tachometer/enrot) is correct, not a guess.
        "distcounter": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "total_distance",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        # The engine's current1 (RailVehicleEngine, ShowCurrent(1)) of the controlled vehicle,
        # matching the original engine's own hvcurrent1: gauge
        # (vehicle/Train.cpp:12137-12142, gauge.AssignFloat(fHCurrent + 1)) in its default path
        # (vehicle/Train.cpp:8638-8641, fHCurrent[1] = mvControlled->ShowCurrent(1) - a plain,
        # unmultiplied passthrough). The one case NOT reproduced: when the vehicle is a
        # multi-unit EZT with ShowNextCurrent toggled on, the original engine instead shows
        # mvSecond's (the other physical unit's) ShowCurrent(1)*1.05 - a driver-facing "peek at
        # next unit's ammeter" feature this wrapper has no command/state for at all, out of scope
        # here.
        "hvcurrent1": {
            # mvControlled's ammeter - a control car's cab shows its motor car's
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "current1",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        # Same source/shape as hvcurrent1 above, one motor circuit over (Train.cpp:10317
        # "hvcurrent2:"/"hvcurrent2b:", ShowCurrent(2)).
        "hvcurrent2": {
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "current2",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        # Train.cpp:10494-10498 "lvoltage:" ("woltomierz niskiego napiecia" - low voltage
        # voltmeter) loads a plain gauge with no AssignFloat visible at the load site itself;
        # ggLVoltage.GetValue() is read back elsewhere as a plain gauge value, not a computed
        # one - power24_voltage (VehicleController.cpp, p_mover->Power24vVoltage) is this wrapper's
        # own low-voltage-circuit reading, same domain (24V control/battery circuit).
        "lvoltage": {
            "widget_class": CabinGauge,
            "fixed_fields": {
                "state_property": "power24_voltage",
                "max_value": 1.0,
            },
            "config_max_property": "",
            "mesh_path_field": "target_mesh_path",
        },
        # Instrument panel fault/status lamps - original engine's own mechanism (confirmed by the
        # comment above battery_sw/i-cablight in this file, and mmd_cabin_instancer.gd:801-814) is
        # an "<submodel>_on"/"<submodel>_off" mesh swap, fully automatic once state_property is
        # set - no light_widget_class needed here (that's only for i-cablight/i-instrumentlight,
        # which are also real light sources). Plain passthroughs of already-exposed state:
        # the radio's lamps of a message heard and of the Radio-Stop (btLampkaRadioMessage,
        # btLampkaRadioStop, Train.cpp:9129-9130, 11618-11619) - the cab radio's own state (CabinRadio3D)
        # m_doors: a door of the trainset open (autolights, Train.cpp:11753)
        "i-doors": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": LegacyCabinDoors.DOORS_OPEN_LAMP },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # btLampkaDoorLeft: on the cab's left (Train.cpp:9167)
        "i-door_left": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": LegacyCabinDoors.SIDE_OPEN_LAMPS[RailVehicleDoors.SIDE_LEFT] },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # btLampkaDoorRight (Train.cpp:9168)
        "i-door_right": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": LegacyCabinDoors.SIDE_OPEN_LAMPS[RailVehicleDoors.SIDE_RIGHT] },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # m_doorspermitleft (autolights, Train.cpp:11754)
        "i-doorpermit_left": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": LegacyCabinDoorPermits.LAMP_KEYS[RailVehicleDoors.SIDE_LEFT] },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # m_doorspermitright (Train.cpp:11755)
        "i-doorpermit_right": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": LegacyCabinDoorPermits.LAMP_KEYS[RailVehicleDoors.SIDE_RIGHT] },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # m_doorpermits: a door permit of the trainset given (Train.cpp:11756)
        "i-doorpermit_any": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": LegacyCabinDoors.PERMITS_LAMP },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # Doors.step_enabled (Train.cpp:11757)
        "i-doorstep": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "doors_step_enabled" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # btLampkaBlokadaDrzwi: Doors.is_locked (Train.cpp:9170)
        "i-door_blocked": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "doors_locked" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # btLampkaDoorLockOff: the lock switched off (Train.cpp:9171)
        "i-door_blockedoff": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "doors_lock_enabled", "invert_value": true },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # btLampkaDepartureSignal: the signal of the controlled vehicle (Train.cpp:9172)
        "i-departure_signal": {
            "target": CabinState.Target.CONTROLLED,
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "doors_departure_signal" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-radiomessage": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": CabinRadio3D.MESSAGE_PLAYED_KEY },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-radiostop": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": CabinRadio3D.RADIO_STOP_LAMP_KEY },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-slippery": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "slipping_wheels" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-motor_ovld": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "fuse_active" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-conv_ovld": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "converter_overload" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # Confirmed against the original engine: "i-comp_ovld:" -> btLampkaNadmSpr
        # (Train.cpp:9882/Train.h:693), but nothing in the original ever assigns that button a
        # value - Train.h marks it "// TODO: implement" and it stays permanently unlit there too.
        # Mapped here to a state key that's never populated (always reads false via
        # CabinIndicator3D's state.get(..., false) fallback) purely to silence the
        # MMD_BINDING_UNSUPPORTED diagnostic - matching the original's own dead widget, not adding
        # new Mover/engine state for a fault the original never actually tracks.
        "i-comp_ovld": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "indicators/compressor_overload_unimplemented" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-trainheating": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "heating_enabled" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # These mirror the original's own combined conditions (not single-flag passthroughs) -
        # see RailVehicleElectricEngine.cpp's own comment on "indicators/*" for the exact Train.cpp
        # line references each one is confirmed against.
        "i-contactors": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "indicators/contactors_active" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-diff_relay": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "indicators/diff_relay_active" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-resistors": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "indicators/resistors_active" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-vent_ovld": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "indicators/vent_overload_active" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-highcurrent": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "indicators/highcurrent_active" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-battery": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "battery_enabled" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-springbrakeactive": {
            "widget_class": CabinIndicator3D,
            # Train.cpp:9195 - lit by SpringBrake.IsActive, the spring braking, not by the switch
            "fixed_fields": { "state_property": "spring_brake/braking" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # Train.cpp:9196 - lit while the spring is not braking
        "i-springbrakeinactive": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "spring_brake/braking", "invert_value": true },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-mainbreaker": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "indicators/mainbreaker_active" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # The E3D indicator follows the plain Radio flag, matching vehicle/Train.cpp:9160.
        # The separate OmniLight follows radio_powered and copies SM42's hand-authored
        # RadioPowerLed parameters; unlike the indicator mesh, its glow requires supply power.
        # Its colour is the lamp submodel's diffuse, which tints the greyscale lamp texture
        # (Model3d.cpp:1918, openglrenderer.cpp:2779).
        "i-radio": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {
                "state_property": "radio_enabled",
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
            "light_widget_class": CabinOmniLight3D,
            "light_color_from_submodel": true,
            "light_fixed_fields": {
                "state_property": "radio_powered",
                "light_energy": 0.007,
                "light_energy_on": 0.05,
                "light_energy_off": 0.0,
                "omni_range": 0.1,
            },
        },
        # i-* labels are indicator meshes in MaSzyna: the widget only switches the matching
        # <submodel>_on/<submodel>_off pair. The optional light widget below is a separate Godot
        # lighting effect anchored at the same submodel when that position exists.
        # Numeric parameters come from SM42's hand-authored reference cabin.
        "i-cablight": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": {
                "cab_light": CabinState.Light.CAB,
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
            "light_widget_class": CabinSpotLight3D,
            "flip_upward_spotlight": true,
            "spread_light_along_submodel": true,
            "light_fixed_fields": {
                # the level, so that the dimmed and the 24 V-only light is dimmer (Train.cpp:9745)
                "cab_light": CabinState.Light.CAB,
                "light_enabled": true,
                "light_color": Color(0.960938, 0.881759, 0.75824, 1.0),
                "light_energy_on": 0.411,
                "light_energy_off": 0.0,
                "light_volumetric_fog_energy": 16.235,
                "light_size": 1.0,
                "light_specular": 5.297,
                "spot_range": 2.785,
                "spot_attenuation": 1.44,
                "spot_angle": 63.62,
                "spot_angle_attenuation": 1.27456,
                "shadow_enabled": true,
            },
        },
        "i-instrumentlight": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": {
                "cab_light": CabinState.Light.INSTRUMENT,
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
            # Instrument backlight overlays are painted as a soft glow, not a hard-edged cutout -
            # unlike most other on/off indicator meshes (e.g. i-cablight), they need real alpha
            # blending (E3DModelInstance.force_alpha_submodel_paths) rather than the default
            # alpha-scissor, or the glow renders as a crisp, wrong-looking silhouette.
            "force_alpha": true,
            # one glow per gauge's overlay, in that overlay's colour
            "island_lights": IslandLights.GLOW,
        },
        # the instrument light's other kinds (Train.cpp:11755-11779: what powers and switches each is the
        # cab logic's, LegacyCabinCabLights.InstrumentLightType) and the dashboard and timetable lights
        # (btDashboardLight, btTimetableLight, Train.cpp:11708-11709, 9573-9574)
        "i-instrumentlight_m": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": {
                "cab_light": CabinState.Light.INSTRUMENT,
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
            "force_alpha": true,
            "island_lights": IslandLights.GLOW,
        },
        "i-instrumentlight_c": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": {
                "cab_light": CabinState.Light.INSTRUMENT,
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
            "force_alpha": true,
            "island_lights": IslandLights.GLOW,
        },
        "i-instrumentlight_a": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": {
                "cab_light": CabinState.Light.INSTRUMENT,
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
            "force_alpha": true,
            "island_lights": IslandLights.GLOW,
        },
        "i-instrumentlight_l": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": {
                "cab_light": CabinState.Light.INSTRUMENT,
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
            "force_alpha": true,
            "island_lights": IslandLights.GLOW,
        },
        "i-dashboardlight": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": {
                "cab_light": CabinState.Light.DASHBOARD,
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
            "force_alpha": true,
            "island_lights": IslandLights.GLOW,
        },
        "i-timetablelight": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": {
                "cab_light": CabinState.Light.TIMETABLE,
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
            "force_alpha": true,
            "island_lights": IslandLights.GLOW,
        },
        # Confirmed against Mover.cpp:183-188 (is_cabsignal_blinking(): `return power &&
        # cabsignal_active` - same static "alert active" shape as is_blinking(), not a real-time
        # oscillating value) and RailVehicleSecuritySystem.cpp:64 (p_state["cabsignal_blinking"]).
        # Same reasoning as i-radio above: SM42's own hand-authored cabin has no dedicated SHP
        # indicator light to copy real numeric params from (only a "czuw_shp" SecurityAcknowledge
        # BUTTON mesh, not a light), so light_enabled stays at its default (false); on_target/
        # off_target submodel toggling and the click sound still work.
        "i-security_cabsignal": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {
                "state_property": "cabsignal_blinking",
                "blink_time": 0.2,
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-security_aware": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {
                "state_property": "blinking",
                # "blinking" (RailVehicleSecuritySystem::is_blinking(), Mover.cpp) is a STATIC "alert
                # active" flag, not a real-time oscillating value - blink_time (matching
                # CabinBlinker's own default, cabin_blinker.gd) is what actually makes the light
                # flash instead of just turning steadily on.
                "blink_time": 0.2,
                # the ONE catalog entry with real reference light data to copy (SM42's own
                # CzuwakOmni1) - every other indicator label defaults to light_enabled=false.
                "light_enabled": true,
                "light_color": Color(0.960938, 0.506832, 0.349091, 1.0),
                "light_energy_on": 0.2,
                "light_energy_off": 0.0,
                "light_size": 0.696,
                "light_specular": 2.014,
                "light_volumetric_fog_energy": 16.0,
                "shadow_enabled": true,
                "spot_range": 2.129,
                "spot_attenuation": 1.98,
                "spot_angle": 69.32,
            },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
            # the alerter lamp has to light the driver, whatever the lamp submodel's own axes are
            "aim_at_driver": true,
            # SM42 has three alerter lamps in its one "czuwak_on" - each gets this light
            "island_lights": IslandLights.WIDGET_LIGHT,
        },
        # Confirmed against vehicle/Train.cpp:5267-5316 (OnCommand_headlighttoggleleft/enableleft
        # etc.) and RailVehicleLighting::light_switch()'s own doc comment (RailVehicleLighting.hpp) - these ten
        # MMD switch labels are cab-relative: upperlight_sw:/leftlight_sw:/rightlight_sw:/
        # leftend_sw:/rightend_sw: (no "rear" prefix) toggle whichever physical end is the
        # CURRENTLY ACTIVE cab's own front, so their own switch position mirrors RailVehicleLighting's
        # "active_..." state; rearupperlight_sw:/etc. toggle the opposite end, mirroring
        # "opposite_...". command_param is light_switch()'s own p_light argument - the MMD label's
        # suffix after stripping "light"/"_sw" (confirmed real examples from that doc comment:
        # "upper", "left", "leftend", "rearupper", "rearleftend"), NOT the full MMD label. action
        # reuses the headlight_*_toggle/redmarker_*_toggle InputMap actions (demo/project.godot) -
        # same one action per physical switch as headlight_left_toggle etc. already use for the
        # keyboard-only fallback.
        "upperlight_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "light_switch",
                "command_param": "upper",
                "state_property": "lights/active_headlight_upper_enabled",
                "action": "headlight_upper_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "leftlight_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "light_switch",
                "command_param": "left",
                "state_property": "lights/active_headlight_left_enabled",
                "action": "headlight_left_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "rightlight_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "light_switch",
                "command_param": "right",
                "state_property": "lights/active_headlight_right_enabled",
                "action": "headlight_right_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "leftend_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "light_switch",
                "command_param": "leftend",
                "state_property": "lights/active_redmarker_left_enabled",
                "action": "redmarker_left_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "rightend_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "light_switch",
                "command_param": "rightend",
                "state_property": "lights/active_redmarker_right_enabled",
                "action": "redmarker_right_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "rearupperlight_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "light_switch",
                "command_param": "rearupper",
                "state_property": "lights/opposite_headlight_upper_enabled",
                "action": "headlight_rear_upper_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "rearleftlight_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "light_switch",
                "command_param": "rearleft",
                "state_property": "lights/opposite_headlight_left_enabled",
                "action": "headlight_rear_left_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "rearrightlight_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "light_switch",
                "command_param": "rearright",
                "state_property": "lights/opposite_headlight_right_enabled",
                "action": "headlight_rear_right_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "rearleftend_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "light_switch",
                "command_param": "rearleftend",
                "state_property": "lights/opposite_redmarker_left_enabled",
                "action": "redmarker_rear_left_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "rearrightend_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "light_switch",
                "command_param": "rearrightend",
                "state_property": "lights/opposite_redmarker_right_enabled",
                "action": "redmarker_rear_right_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Confirmed against vehicle/Train.cpp:9199-9208 - these read the mover's own already-
        # resolved per-end light bitmask (MOVER.h's iLights[front]/iLights[rear], tested against
        # the `light::` bit flags) directly, not the raw selector position - iLights is the
        # final, live "which bulbs are actually lit" result (already accounts for
        # light_power/selector position/wiring), added to RailVehicleLighting's own state
        # (lights/front_headlight_upper_enabled etc., RailVehicleLighting.cpp) specifically for these
        # labels.
        # "upper"=headlight_upper, "left/right light"=headlight_left/right (white),
        # "left/right end"=redmarker_left/right (red tail/end-of-train markers) - confirmed via
        # the same bit flags MOVER.h itself defines. No blink_time - these are steady on/off,
        # unlike the alerter/SHP indicators. light_enabled stays at its default (false) - same
        # "no real per-vehicle lamp reference data" reasoning as i-radio/i-security_cabsignal.
        "i-upperlight": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {"state_property": "lights/front_headlight_upper_enabled"},
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-leftlight": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {"state_property": "lights/front_headlight_left_enabled"},
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-rightlight": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {"state_property": "lights/front_headlight_right_enabled"},
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-leftend": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {"state_property": "lights/front_redmarker_left_enabled"},
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-rightend": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {"state_property": "lights/front_redmarker_right_enabled"},
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-rearupperlight": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {"state_property": "lights/rear_headlight_upper_enabled"},
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-rearleftlight": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {"state_property": "lights/rear_headlight_left_enabled"},
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-rearrightlight": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {"state_property": "lights/rear_headlight_right_enabled"},
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-rearleftend": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {"state_property": "lights/rear_redmarker_left_enabled"},
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        "i-rearrightend": {
            "widget_class": CabinSpotLight3D,
            "fixed_fields": {"state_property": "lights/rear_redmarker_right_enabled"},
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # Train.cpp:11911 ggPantSelectedButton -> OnCommand_pantographtoggleselected (Train.cpp:3403),
        # Ctrl+Shift+O. Its handler branches on the kind of switch (ggPantSelectedButton.type(),
        # Train.cpp:3434) - LegacyCabinPantographSelected owns it
        "pantselected_sw": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            # an impulse one rests midway, between its two directions (Train.cpp:3455)
            "push_value_rest": 0.5,
            "fixed_fields": {
                "monostable": true,
                "state_property": "current_collector/valve_enabled",
                "action": "pantograph_toggle_selected",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11912 ggPantSelectedDownButton -> OnCommand_pantographlowerselected (Train.cpp:3483),
        # branching on ggPantSelectedDownButton.type() - LegacyCabinPantographSelected owns it
        "pantselectedoff_sw": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The pantograph selector, showing the cab's PantsPreset selection (Train.cpp:12063) ->
        # OnCommand_pantographselectnext/previous (Train.cpp:3375-3402), Shift+P / Shift+O.
        # LegacyCabinPantographPresets owns it
        "pantselect_sw": {
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": 0,
                "action_increase": "pantograph_select_next",
                "action_decrease": "pantograph_select_previous",
            },
            "config_max_property": "pantograph_preset_max",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11895 ggPantValvesButton -> OnCommand_pantographvalvesupdate/off (Train.cpp:3551-3620),
        # no key (driverkeyboardinput.cpp:213). A lever resting midway: up sets the pantographs'
        # valves to the selection, down closes them. Its gauge shows 1 / 0.5 / 0, so its three
        # positions are half the MMD scale apart. LegacyCabinPantographPresets owns it
        "pantvalves_sw": {
            "widget_class": CabinSwitch,
            "mmd_scale_multiplier": 0.5,
            "fixed_fields": {
                "switch_min_position": 0,
                "switch_max_position": 2,
                "switch_reset_position": 1,
                "switch_position": 1,
                "automatic_reset": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The same two operations as separate buttons (Train.cpp:11983-11984), owned by
        # LegacyCabinPantographPresets
        "pantvalvesupdate_bt": {
            "widget_class": CabinButton,
            "fixed_fields": { "monostable": true },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "pantvalvesoff_bt": {
            "widget_class": CabinButton,
            "fixed_fields": { "monostable": true },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11868 ggLightsButton -> OnCommand_lightspresetactivatenext/previous (Train.cpp:5193-5265),
        # Shift+T / T; it shows LightsPos - 1
        "lights_sw": {
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": 0,
                "command_increase": "increase_light_selector_position",
                "command_decrease": "decrease_light_selector_position",
                "state_property": "light_selector_position",
                "action_increase": "lights_preset_next",
                "action_decrease": "lights_preset_previous",
            },
            "config_max_property": "light_position_max",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11853 ggDoorPermitPresetButton -> OnCommand_doorpermitpresetactivatenext/previous
        # (Train.cpp:7296-7317), Ctrl+Shift+. / Ctrl+Shift+,
        "doorpermitpreset_sw": {
            "widget_class": CabinSwitch,
            "fixed_fields": {
                "switch_min_position": 0,
                "command_increase": "doors_next_permit_preset",
                "command_decrease": "doors_previous_permit_preset",
                "state_property": "doors_permit_preset",
                "action_increase": "doors_permit_preset_next",
                "action_decrease": "doors_permit_preset_previous",
            },
            "config_max_property": "doors_permit_preset_max",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:12033-12034 ggDoorLeft/RightPermitButton -> OnCommand_doorpermitleft/right
        # (Train.cpp:7196-7294), Shift+, / Shift+. - LegacyCabinDoorPermits
        # the cruise control switch and its off switch, lit while the speed control is active
        # (OnCommand_tempomattoggle, Train.cpp:1486-1549; stategauges, Train.cpp:11999-12000) - LegacyCabinTempomat
        "tempomat_sw": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "tempomatoff_sw": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
            },
            "state_light": {"state_property": "speed_control/active"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # the door step switch, lit by the step permit (OnCommand_doorsteptoggle, Train.cpp:7692-7720;
        # stategauges, Train.cpp:12018) - LegacyCabinDoorStep
        "doorstep_sw": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
            },
            "state_light": {"state_property": "doors_step_enabled"},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "doorleftpermit_sw": {
            "widget_class": CabinButton,
            # its original handler branches on the kind of switch (ggDoorLeftPermitButton.is_push(), Train.cpp:7213)
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "doors_left_permit",
            },
            # lit as the original lights it (stategauges, Train.cpp:12015; LegacyCabinDoorPermits)
            "state_light": {"state_property": LegacyCabinDoorPermits.LAMP_KEYS[RailVehicleDoors.SIDE_LEFT]},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        "doorrightpermit_sw": {
            "widget_class": CabinButton,
            # ggDoorRightPermitButton.is_push(), Train.cpp:7263
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "doors_right_permit",
            },
            "state_light": {"state_property": LegacyCabinDoorPermits.LAMP_KEYS[RailVehicleDoors.SIDE_RIGHT]},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # The door controls - LegacyCabinDoors (Train.cpp:7087-7724, 7899-7929): each takes a press and
        # a release, a cab without the gauge does nothing; the gauges show what the handlers set
        # OnCommand_doortoggleleft (Train.cpp:7121), Comma
        "door_left_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "action": "doors_left_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # OnCommand_doortoggleright (Train.cpp:7420), Period
        "door_right_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "action": "doors_right_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # OnCommand_dooropenleft (Train.cpp:7320) - no key in the original
        "doorlefton_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # OnCommand_dooropenright (Train.cpp:7495)
        "doorrighton_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # OnCommand_doorcloseleft (Train.cpp:7362)
        "doorleftoff_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # OnCommand_doorcloseright (Train.cpp:7538)
        "doorrightoff_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # OnCommand_dooropenall (Train.cpp:7595), Shift+/
        "doorallon_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "action": "doors_open_all",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # OnCommand_doorcloseall (Train.cpp:7630), Ctrl+/ - lit while a door of the trainset
        # is open (stategauges, Train.cpp:12035)
        "dooralloff_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "action": "doors_close_all",
            },
            "state_light": {"state_property": LegacyCabinDoors.DOORS_OPEN_LAMP},
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # OnCommand_doormodetoggle (Train.cpp:7717), Ctrl+Shift+/ - shows Doors.remote_only
        # (autoboolgauges, Train.cpp:12054)
        "doormode_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "state_property": "doors_remote_only",
                "action": "doors_remote_mode_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # OnCommand_doorlocktoggle (Train.cpp:7087), Ctrl+S
        "door_signalling_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "state_property": "doors_lock_enabled",
                "action": "doors_lock_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # OnCommand_departureannounce (Train.cpp:7899), /
        "departure_signal_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "action": "departure_announce",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:12065 "mirrors_sw:" shows MirrorForbidden, a press flips it
        # (OnCommand_mirrorstoggle, Train.cpp:7726; drivermouseinput.cpp:744) - no key in the original
        "mirrors_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "mirrors_forbid",
                "state_property": "mirrors_forbidden",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11865 ggDimHeadlightsButton -> OnCommand_headlightsdimtoggle (Train.cpp:6125), Ctrl+L
        "dimheadlights_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": false,
                "command": "headlights_dim",
                "state_property": "headlights_dimmed",
                "action": "headlights_dim_toggle",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11897 ggRadioStop -> OnCommand_radiostopsend (Train.cpp:8149) - sends on the press,
        # Ctrl+Shift+Pause
        "radiostop_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "radio_stop",
                "action": "radio_stop_send",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11899 ggRadioCall1 -> OnCommand_radiocall1send (Train.cpp:8209) - sends on the press
        # to the scenery's radio launchers in range; no key (driverkeyboardinput.cpp)
        "radiocall1_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "radio_call1",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11900 ggRadioCall3 -> OnCommand_radiocall3send (Train.cpp:8227), Backspace
        # (driverkeyboardinput.cpp:158)
        "radiocall3_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "radio_call3",
                "action": "radio_call3_send",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11903 ggRadioVolumeNext -> OnCommand_radiovolumeincrease (Train.cpp:8246)
        "radiovolumenext_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "radio_volume_increase",
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11902 ggRadioVolumePrevious -> OnCommand_radiovolumedecrease (Train.cpp:8263)
        "radiovolumeprev_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "radio_volume_decrease",
                "controller_mode": CabinButton.ControllerMode.On,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11931 ggDistanceCounterButton -> OnCommand_distancecounteractivate (Train.cpp:1552),
        # an impulse button starting the count on the press
        "distancecounter_sw": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "distance_counter_activate",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11838 ggUniveralBrakeButton1 -> OnCommand_universalbrakebutton1
        # (Train.cpp:1897) - UniversalBrakeButton(0, held)
        "universalbrake1_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "universal_brake_button",
                "command_param": 0,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11839 ggUniveralBrakeButton2 -> OnCommand_universalbrakebutton2
        # (Train.cpp:1897) - UniversalBrakeButton(1, held)
        "universalbrake2_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "universal_brake_button",
                "command_param": 1,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11840 ggUniveralBrakeButton3 -> OnCommand_universalbrakebutton3
        # (Train.cpp:1897) - UniversalBrakeButton(2, held)
        "universalbrake3_bt": {
            "widget_class": CabinButton,
            "fixed_fields": {
                "monostable": true,
                "command": "universal_brake_button",
                "command_param": 2,
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11758 - LockPipe, the main pipe cut off from the brake valve
        "i-mainpipelock": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "main_pipe_locked" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # Train.cpp:9218 btLampkaTempomat - SpeedCtrlUnit.IsActive
        "i-tempomat": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "speed_control/active" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # Train.cpp:9210 btLampkaMalfunction - the controlled vehicle's dizel_heat.PA
        "i-malfunction": {
            "widget_class": CabinIndicator3D,
            "fixed_fields": { "state_property": "diesel_heat_malfunction" },
            "config_max_property": "",
            "mesh_path_field": "",
            "position_at_submodel": true,
        },
        # Train.cpp:11935 ggUniversals[0] -> OnCommand_generictoggle0 (Train.cpp:6720), key 0: a
        # control of the cab alone, no vehicle behind it; pushes or toggles by its type (Train.cpp:6733)
        "universal0": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "generic_toggle_0",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11936 ggUniversals[1] -> OnCommand_generictoggle1 (Train.cpp:6720), key 1: a
        # control of the cab alone, no vehicle behind it; pushes or toggles by its type (Train.cpp:6733)
        "universal1": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "generic_toggle_1",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11937 ggUniversals[2] -> OnCommand_generictoggle2 (Train.cpp:6720), key 2: a
        # control of the cab alone, no vehicle behind it; pushes or toggles by its type (Train.cpp:6733)
        "universal2": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "generic_toggle_2",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11938 ggUniversals[3] -> OnCommand_generictoggle3 (Train.cpp:6720), key 3: a
        # control of the cab alone, no vehicle behind it; pushes or toggles by its type (Train.cpp:6733)
        "universal3": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "generic_toggle_3",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11939 ggUniversals[4] -> OnCommand_generictoggle4 (Train.cpp:6720), key 4: a
        # control of the cab alone, no vehicle behind it; pushes or toggles by its type (Train.cpp:6733)
        "universal4": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "generic_toggle_4",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11940 ggUniversals[5] -> OnCommand_generictoggle5 (Train.cpp:6720), key 5: a
        # control of the cab alone, no vehicle behind it; pushes or toggles by its type (Train.cpp:6733)
        "universal5": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "generic_toggle_5",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11941 ggUniversals[6] -> OnCommand_generictoggle6 (Train.cpp:6720), key 6: a
        # control of the cab alone, no vehicle behind it; pushes or toggles by its type (Train.cpp:6733)
        "universal6": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "generic_toggle_6",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11942 ggUniversals[7] -> OnCommand_generictoggle7 (Train.cpp:6720), key 7: a
        # control of the cab alone, no vehicle behind it; pushes or toggles by its type (Train.cpp:6733)
        "universal7": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "generic_toggle_7",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11943 ggUniversals[8] -> OnCommand_generictoggle8 (Train.cpp:6720), key 8: a
        # control of the cab alone, no vehicle behind it; pushes or toggles by its type (Train.cpp:6733)
        "universal8": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "generic_toggle_8",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
        # Train.cpp:11944 ggUniversals[9] -> OnCommand_generictoggle9 (Train.cpp:6720), key 9: a
        # control of the cab alone, no vehicle behind it; pushes or toggles by its type (Train.cpp:6733)
        "universal9": {
            "widget_class": CabinButton,
            "shape_from_button_type": true,
            "fixed_fields": {
                "monostable": false,
                "action": "generic_toggle_9",
            },
            "config_max_property": "",
            "mesh_path_field": "mesh_path",
        },
    }


static func get_labels() -> Array:
    _ensure_built()
    return _catalog.keys()


static func has_label(label:String) -> bool:
    _ensure_built()
    return _catalog.has(label)


static func get_entry(label:String) -> Dictionary:
    _ensure_built()
    return _catalog.get(label, {})


## The fields of a control as a cab has it: its entry's fixed fields shaped by the MMD `type:` and by
## the vehicle's configuration - what the widget built of it shows (MmdCabinInstancer._build_widget)
## and what the cab logic's keys do (LegacyCabinLogic), with or without the widget
static func resolve_fields(label:String, button_type:CabinButton.ButtonType, vehicle_config:Dictionary) -> Dictionary:
    var entry:Dictionary = get_entry(label)
    var fields:Dictionary = entry.get("fixed_fields", {}).duplicate()
    if entry.get("widget_class") == CabinButton:
        # A control whose original handler branches on its type (the entry says which line) is
        # shaped by it: a push springs back, and shows no state while at rest - the original
        # returns it to neutral on release rather than to the vehicle's state (Train.cpp:2929,
        # 11342). Every other control keeps the fixed shape of its entry.
        if entry.get("shape_from_button_type", false):
            var push:bool = bool(button_type & CabinButton.ButtonType.PUSH)
            fields["monostable"] = push
            if push:
                fields["state_property"] = ""
                fields["value_rest"] = entry.get("push_value_rest", 0.0)
        # A switch whose kind is the vehicle's rather than the gauge's: the pantograph switches
        # spring back when the vehicle's pantograph switches are impulse ones (PantSwitchType,
        # Train.cpp:3170)
        var monostable_property:String = entry.get("monostable_from_config", "")
        if monostable_property:
            fields["monostable"] = bool(vehicle_config.get(monostable_property, fields.get("monostable", false)))
    # A knob whose key steps or holds as the vehicle's handle does (the train brake handle,
    # Train.cpp:1960-1966)
    var key_stepped_property:String = entry.get("key_stepped_from_config", "")
    if key_stepped_property:
        fields["key_stepped"] = int(vehicle_config.get(key_stepped_property,
                RailVehicleBrake.BRAKE_HANDLE_MOVEMENT_STEPPED)) == RailVehicleBrake.BRAKE_HANDLE_MOVEMENT_STEPPED
    var config_max_property:String = entry.get("config_max_property", "")
    if config_max_property and entry.get("widget_class") == CabinSwitch and vehicle_config.has(config_max_property):
        fields["switch_max_position"] = int(vehicle_config[config_max_property])
    return fields
