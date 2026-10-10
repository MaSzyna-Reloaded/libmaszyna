extends MaszynaGutTest

## HUDServer - the one owner of what the HUD shows: its panels by name, the HUD at all, and the
## vehicle whose card is open. It knows no panel of its own.

const PANEL:StringName = &"test_hud_server_panel"
const TRACK_NAME:String = "hud_server_test"
const TRACK_LENGTH:float = 200.0
const TRACK_OFFSET:float = 100.0
## Enough frames for the vehicle to take its controller
const SETTLE_FRAMES:int = 4

## What HUDServer announced, in order
var _announced:Array = []


func before_each() -> void:
    _announced.clear()
    HUDServer.panel_visibility_changed.connect(_on_panel_visibility_changed)
    HUDServer.hud_visibility_changed.connect(_on_hud_visibility_changed)
    HUDServer.card_changed.connect(_on_card_changed)


func after_each() -> void:
    HUDServer.panel_visibility_changed.disconnect(_on_panel_visibility_changed)
    HUDServer.hud_visibility_changed.disconnect(_on_hud_visibility_changed)
    HUDServer.card_changed.disconnect(_on_card_changed)
    HUDServer.panel_set_visible(PANEL, false)
    HUDServer.hud_set_visible(true)
    HUDServer.card_close()


func _on_panel_visibility_changed(panel:StringName, shown:bool) -> void:
    _announced.append([panel, shown])


func _on_hud_visibility_changed(shown:bool) -> void:
    _announced.append([&"hud", shown])


func _on_card_changed(vehicle:RID) -> void:
    _announced.append([&"card", vehicle])


func test_a_panel_never_opened_is_closed_and_opens_and_closes_once() -> void:
    assert_false(HUDServer.panel_is_visible(PANEL))

    HUDServer.panel_toggle(PANEL)
    HUDServer.panel_set_visible(PANEL, true)
    assert_true(HUDServer.panel_is_visible(PANEL))
    HUDServer.panel_toggle(PANEL)

    assert_false(HUDServer.panel_is_visible(PANEL))
    assert_eq(_announced, [[PANEL, true], [PANEL, false]], "opening an open panel is no change")


func test_the_hud_is_shown_until_hidden() -> void:
    assert_true(HUDServer.hud_is_visible())

    HUDServer.hud_set_visible(false)
    HUDServer.hud_set_visible(false)

    assert_false(HUDServer.hud_is_visible())
    assert_eq(_announced, [[&"hud", false]])


func test_the_card_shows_one_vehicle_and_closes_when_it_is_freed() -> void:
    var track:RID = build_track(TRACK_NAME, TRACK_LENGTH)
    var vehicle_node:RailVehicle3D = build_rail_vehicle("HudServerTest", TRACK_NAME, TRACK_OFFSET)
    await wait_idle_frames(SETTLE_FRAMES)
    var vehicle:RID = vehicle_node.get_rid()

    HUDServer.card_open(vehicle)
    HUDServer.card_open(vehicle)
    assert_eq(HUDServer.card_get_vehicle(), vehicle)
    free_rail_vehicle(vehicle_node)

    assert_eq(HUDServer.card_get_vehicle(), RID())
    assert_eq(_announced, [[&"card", vehicle], [&"card", RID()]])
    TrackServer.track_free(track)
    TrackServer.topology_rebuild()
