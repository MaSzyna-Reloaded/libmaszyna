extends MaszynaGutTest

## CabinHUDMouseSystem: a cab control under the cursor is outlined and captioned, a left click
## presses and releases it, a vertical drag steps it.

var _camera:Camera3D
var _mesh:MeshInstance3D
var _control:RID
var _calls:Array[String] = []
var _captions:Array[String] = []


func before_each() -> void:
    _calls.clear()
    _captions.clear()
    _camera = Camera3D.new()
    add_child_autofree(_camera)
    _camera.make_current()
    _mesh = MeshInstance3D.new()
    _mesh.mesh = BoxMesh.new()
    _mesh.position = Vector3(0.0, 0.0, -3.0)
    add_child_autofree(_mesh)
    CabinHUDMouseSystem.mouse_set_camera(_camera.get_instance_id())
    CabinHUDMouseSystem.control_hovered.connect(_on_control_hovered)
    _control = _create_control(Vector3.UP)


## A control whose one increase shifts it by `step_offset`
func _create_control(step_offset:Vector3) -> RID:
    return CabinHUDMouseSystem.control_create(_mesh.get_instance_id(), "battery", "B",
            func() -> void: _calls.append("pressed"),
            func() -> void: _calls.append("released"),
            func() -> void: _calls.append("increase"),
            func() -> void: _calls.append("decrease"),
            Basis.IDENTITY, step_offset, Callable(), Vector2.ZERO)


func after_each() -> void:
    CabinHUDMouseSystem.control_free(_control)
    CabinHUDMouseSystem.control_hovered.disconnect(_on_control_hovered)


func _on_control_hovered(caption:String, hints:String, state:String) -> void:
    _captions.append("%s|%s|%s" % [caption, hints, state])


func _move_to(position:Vector2) -> bool:
    var event:InputEventMouseMotion = InputEventMouseMotion.new()
    event.position = position
    return CabinHUDMouseSystem.mouse_input(event)


func _drag_by(relative:Vector2) -> bool:
    var event:InputEventMouseMotion = InputEventMouseMotion.new()
    event.relative = relative
    return CabinHUDMouseSystem.mouse_input(event)


func _left_button(pressed:bool) -> bool:
    var event:InputEventMouseButton = InputEventMouseButton.new()
    event.button_index = MOUSE_BUTTON_LEFT
    event.pressed = pressed
    return CabinHUDMouseSystem.mouse_input(event)


func _over_control() -> Vector2:
    return _camera.unproject_position(_mesh.global_position)


func test_hover_outlines_and_captions_the_control() -> void:
    _move_to(_over_control())
    assert_eq(CabinHUDMouseSystem.control_get_hovered(), _control)
    assert_not_null(_mesh.material_overlay)
    assert_eq(_captions, ["battery|B|"] as Array[String])

    _move_to(Vector2.ONE)
    assert_false(CabinHUDMouseSystem.control_get_hovered().is_valid())
    assert_null(_mesh.material_overlay)


func test_click_presses_and_releases() -> void:
    _move_to(_over_control())
    assert_true(_left_button(true))
    assert_true(_left_button(false))
    assert_eq(_calls, ["pressed", "released"] as Array[String])


func test_click_off_a_control_is_not_consumed() -> void:
    _move_to(Vector2.ONE)
    assert_false(_left_button(true))
    assert_false(_left_button(false))
    assert_eq(_calls, [] as Array[String])


func test_vertical_drag_steps_the_control() -> void:
    var step:int = ClassDB.class_get_integer_constant(&"CabinHUDMouseSystem", &"DRAG_STEP_PIXELS")
    _move_to(_over_control())
    _left_button(true)
    assert_true(_drag_by(Vector2(0.0, -step)))
    assert_true(_drag_by(Vector2(0.0, -step)))
    assert_true(_drag_by(Vector2(0.0, step)))
    _left_button(false)
    assert_eq(_calls, ["pressed", "increase", "increase", "decrease", "released"] as Array[String])


## The first movement chooses the mouse axis and the drag keeps to it until let go; each axis
## counts the way the grip goes on it - here right and up.
func test_drag_keeps_the_axis_its_first_movement_took() -> void:
    CabinHUDMouseSystem.control_free(_control)
    _control = _create_control(Vector3(1.0, 0.5, 0.0))
    var step:int = ClassDB.class_get_integer_constant(&"CabinHUDMouseSystem", &"DRAG_STEP_PIXELS")
    _move_to(_over_control())
    _left_button(true)
    _drag_by(Vector2(step, 0.0))
    _drag_by(Vector2(0.0, -step))
    _drag_by(Vector2(-step, 0.0))
    _left_button(false)
    _left_button(true)
    _drag_by(Vector2(0.0, -step))
    _left_button(false)
    assert_eq(_calls, ["pressed", "increase", "decrease", "released", "pressed", "increase", "released"]
            as Array[String])


## Signs fixed by the control win over its grip: here down and left increase.
func test_fixed_signs_override_the_grip() -> void:
    CabinHUDMouseSystem.control_free(_control)
    _control = CabinHUDMouseSystem.control_create(_mesh.get_instance_id(), "valve", "",
            func() -> void: _calls.append("pressed"),
            func() -> void: _calls.append("released"),
            func() -> void: _calls.append("increase"),
            func() -> void: _calls.append("decrease"),
            Basis.IDENTITY, Vector3.UP, Callable(), Vector2(-1.0, 1.0))
    var step:int = ClassDB.class_get_integer_constant(&"CabinHUDMouseSystem", &"DRAG_STEP_PIXELS")
    _move_to(_over_control())
    _left_button(true)
    _drag_by(Vector2(0.0, step))
    _left_button(false)
    _left_button(true)
    _drag_by(Vector2(-step, 0.0))
    _left_button(false)
    assert_eq(_calls, ["pressed", "increase", "released", "pressed", "increase", "released"] as Array[String])


func test_occluder_in_front_hides_the_control() -> void:
    var desk:MeshInstance3D = MeshInstance3D.new()
    desk.mesh = BoxMesh.new()
    desk.position = Vector3(0.0, 0.0, -1.5)
    add_child_autofree(desk)
    var occluder:RID = CabinHUDMouseSystem.occluder_create(desk.get_instance_id())
    _move_to(_over_control())
    assert_false(CabinHUDMouseSystem.control_get_hovered().is_valid())
    CabinHUDMouseSystem.occluder_free(occluder)


func test_occluder_behind_or_the_control_itself_does_not_hide_it() -> void:
    var wall:MeshInstance3D = MeshInstance3D.new()
    wall.mesh = BoxMesh.new()
    wall.position = Vector3(0.0, 0.0, -5.0)
    add_child_autofree(wall)
    var occluders:Array[RID] = [
        CabinHUDMouseSystem.occluder_create(wall.get_instance_id()),
        CabinHUDMouseSystem.occluder_create(_mesh.get_instance_id()),
    ]
    _move_to(_over_control())
    assert_eq(CabinHUDMouseSystem.control_get_hovered(), _control)
    for occluder:RID in occluders:
        CabinHUDMouseSystem.occluder_free(occluder)


## A mesh under a control with no other control under it moves with it and is the control too:
## EP07's brake valve handle, a reverser's handle.
func test_child_mesh_is_part_of_the_control() -> void:
    var handle:MeshInstance3D = MeshInstance3D.new()
    handle.mesh = BoxMesh.new()
    handle.position = Vector3(2.0, 0.0, 0.0)
    _mesh.add_child(handle)
    CabinHUDMouseSystem.control_free(_control)
    _control = _create_control(Vector3.UP)
    _move_to(_camera.unproject_position(handle.global_position))
    assert_eq(CabinHUDMouseSystem.control_get_hovered(), _control)
    assert_not_null(handle.material_overlay)
    assert_not_null(_mesh.material_overlay)


## A control with another control under it is a panel, its own mesh only: E186's universal1 holds
## op1 (a control) and op12 (nobody's) - whichever registers first.
func test_control_holding_another_control_is_its_own_mesh_only() -> void:
    for panel_first:bool in [true, false]:
        var control_child:MeshInstance3D = MeshInstance3D.new()
        control_child.mesh = BoxMesh.new()
        control_child.position = Vector3(2.0, 0.0, 0.0)
        _mesh.add_child(control_child)
        var other_child:MeshInstance3D = MeshInstance3D.new()
        other_child.mesh = BoxMesh.new()
        other_child.position = Vector3(-2.0, 0.0, 0.0)
        _mesh.add_child(other_child)
        CabinHUDMouseSystem.control_free(_control)
        var child_control:RID = RID()
        if not panel_first:
            child_control = CabinHUDMouseSystem.control_create(control_child.get_instance_id(), "pantograph", "",
                    Callable(), Callable(), Callable(), Callable(), Basis.IDENTITY, Vector3.ZERO, Callable(),
                    Vector2.ZERO)
        _control = _create_control(Vector3.UP)
        if panel_first:
            child_control = CabinHUDMouseSystem.control_create(control_child.get_instance_id(), "pantograph", "",
                    Callable(), Callable(), Callable(), Callable(), Basis.IDENTITY, Vector3.ZERO, Callable(),
                    Vector2.ZERO)

        _move_to(_camera.unproject_position(other_child.global_position))
        assert_false(CabinHUDMouseSystem.control_get_hovered().is_valid(), "op12 is nobody's")
        _move_to(_camera.unproject_position(control_child.global_position))
        assert_eq(CabinHUDMouseSystem.control_get_hovered(), child_control, "op1 is its own")
        _move_to(_over_control())
        assert_eq(CabinHUDMouseSystem.control_get_hovered(), _control, "the panel keeps its own mesh")
        CabinHUDMouseSystem.control_free(child_control)
        control_child.free()
        other_child.free()


func test_cursor_just_beside_a_small_control_takes_it() -> void:
    _mesh.scale = Vector3.ONE * 0.01
    _move_to(_over_control() + Vector2(6.0, 0.0))
    assert_eq(CabinHUDMouseSystem.control_get_hovered(), _control)


## A wheel on a lying axis is turned by its top: grabbed below the hub, dragging the way its top
## moves on an increase still increases.
func test_wheel_is_turned_by_its_top_wherever_grabbed() -> void:
    CabinHUDMouseSystem.control_free(_control)
    # turning about X, the camera's right: an increase takes the top of the wheel away and down on
    # screen, while the point grabbed below the hub, on the near face, goes up
    _control = CabinHUDMouseSystem.control_create(_mesh.get_instance_id(), "wheel", "",
            func() -> void: _calls.append("pressed"),
            func() -> void: _calls.append("released"),
            func() -> void: _calls.append("increase"),
            func() -> void: _calls.append("decrease"),
            Basis(Vector3.RIGHT, deg_to_rad(-10.0)), Vector3.ZERO, Callable(), Vector2.ZERO)
    var step:int = ClassDB.class_get_integer_constant(&"CabinHUDMouseSystem", &"DRAG_STEP_PIXELS")
    _move_to(_camera.unproject_position(_mesh.global_position + Vector3(0.0, -0.4, 0.5)))
    _left_button(true)
    _drag_by(Vector2(0.0, step))
    _left_button(false)
    assert_eq(_calls, ["pressed", "increase", "released"] as Array[String])


## A continuous control takes the drag's travel along its movement instead of steps.
func test_continuous_control_takes_the_travel() -> void:
    CabinHUDMouseSystem.control_free(_control)
    var travels:Array[float] = []
    _control = CabinHUDMouseSystem.control_create(_mesh.get_instance_id(), "valve", "",
            Callable(), Callable(), Callable(), Callable(), Basis.IDENTITY, Vector3.UP,
            func(travel:float) -> void: travels.append(travel), Vector2.ZERO)
    _move_to(_over_control())
    _left_button(true)
    _drag_by(Vector2(0.0, -7.0))
    _left_button(false)
    assert_eq(travels.size(), 1)
    assert_almost_eq(travels[0], 7.0, 0.01)


## A valve is held by its handle - the point farthest from its axis - wherever the cursor took it:
## its core grabbed next to the axis still drags the way the handle moves, and only along the
## mouse axis the handle moves along.
func test_turning_control_is_dragged_by_its_grip() -> void:
    var handle:MeshInstance3D = MeshInstance3D.new()
    handle.mesh = BoxMesh.new()
    handle.scale = Vector3.ONE * 0.2
    handle.position = Vector3(0.0, 0.0, 1.5)
    _mesh.add_child(handle)
    CabinHUDMouseSystem.control_free(_control)
    # turning about the standing Y axis: the handle, toward the viewer, goes right on an increase
    _control = CabinHUDMouseSystem.control_create(
            _mesh.get_instance_id(), "valve", "",
            func() -> void: _calls.append("pressed"),
            func() -> void: _calls.append("released"),
            func() -> void: _calls.append("increase"),
            func() -> void: _calls.append("decrease"),
            Basis(Vector3.UP, deg_to_rad(10.0)), Vector3.ZERO, Callable(), Vector2.ZERO)
    var step:int = ClassDB.class_get_integer_constant(&"CabinHUDMouseSystem", &"DRAG_STEP_PIXELS")
    # on the core's front face, beside the handle
    _move_to(_camera.unproject_position(_mesh.global_position + Vector3(-0.3, 0.2, 0.5)))
    _left_button(true)
    _drag_by(Vector2(step, 0.0))
    _left_button(false)
    assert_eq(_calls, ["pressed", "increase", "released"] as Array[String])


func test_state_goes_with_the_caption_and_follows_changes() -> void:
    var states:Array[String] = []
    var on_state:Callable = func(state:String) -> void: states.append(state)
    CabinHUDMouseSystem.control_state_changed.connect(on_state)
    CabinHUDMouseSystem.control_set_state(_control, "3")
    _move_to(_over_control())
    CabinHUDMouseSystem.control_set_state(_control, "4")
    CabinHUDMouseSystem.control_state_changed.disconnect(on_state)
    assert_eq(_captions, ["battery|B|3"] as Array[String])
    assert_eq(states, ["4"] as Array[String])


## Each position is a notch: a quick pull of two steps and a half makes one, the rest is dropped.
func test_quick_pull_stops_in_the_next_notch() -> void:
    var step:int = ClassDB.class_get_integer_constant(&"CabinHUDMouseSystem", &"DRAG_STEP_PIXELS")
    _move_to(_over_control())
    _left_button(true)
    _drag_by(Vector2(0.0, -2.5 * step))
    _left_button(false)
    assert_eq(_calls, ["pressed", "increase", "released"] as Array[String])


func test_freeing_the_hovered_control_clears_its_outline() -> void:
    _move_to(_over_control())
    CabinHUDMouseSystem.control_free(_control)
    assert_false(CabinHUDMouseSystem.control_get_hovered().is_valid())
    assert_null(_mesh.material_overlay)


## E186's op1 (pantfront_sw) is a submodel under opcje_panto (universal1), whose panel turns it into
## view: a control under another's mesh is its own - the one above hovered and clicked it for it,
## and every button of the panel lit up together.
func test_a_control_under_another_control_is_its_own() -> void:
    var child:MeshInstance3D = MeshInstance3D.new()
    child.mesh = BoxMesh.new()
    child.position = Vector3(2.0, 0.0, 0.0)
    _mesh.add_child(child)
    var child_control:RID = CabinHUDMouseSystem.control_create(
            child.get_instance_id(), "pantograph", "",
            func() -> void: _calls.append("child pressed"), Callable(), Callable(), Callable(),
            Basis.IDENTITY, Vector3.ZERO, Callable(), Vector2.ZERO)

    _move_to(_camera.unproject_position(child.global_position))
    assert_eq(CabinHUDMouseSystem.control_get_hovered(), child_control, "the child takes the cursor")
    assert_null(_mesh.material_overlay, "and the control above it is not lit")
    _left_button(true)
    _left_button(false)
    assert_eq(_calls, ["child pressed"] as Array[String])
    CabinHUDMouseSystem.control_free(child_control)
