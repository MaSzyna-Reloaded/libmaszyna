@tool
extends Node

enum Wires {
    CONTACT,
    CONTACT_SUPPORT,
    TWO_CONTACT_ONE_SUPPORT,
    TWO_CONTACT_TWO_SUPPORT
}


class TractionState:
    var primary_mesh_instance: RID = RID()
    var secondary_mesh_instance: RID = RID()
    var primary_mesh: Mesh
    var secondary_mesh: Mesh
    ## Valid when the span is streamed (SceneryStreamingServer); hand-placed nodes are not
    var stream_rid: RID = RID()
    var streamed: bool = false
    ## Geometry of the span, kept so the meshes can be built when it comes into range
    var contact_p1: Vector3 = Vector3.ZERO
    var contact_p2: Vector3 = Vector3.ZERO
    var support_p1: Vector3 = Vector3.ZERO
    var support_p2: Vector3 = Vector3.ZERO
    var wire_thickness: float = 0.0
    var wires: int = 0
    var wire_offset: float = 0.0
    var min_height: float = 0.0
    var segment_length: float = 0.0


class SegmentData:
    var start: Vector3 = Vector3.ZERO
    var finish: Vector3 = Vector3.ZERO
    var width_axis_hint: Vector3 = Vector3.ZERO

    func _init(p_start: Vector3, p_finish: Vector3, p_width_axis_hint: Vector3) -> void:
        start = p_start
        finish = p_finish
        width_axis_hint = p_width_axis_hint

const MIN_SEGMENT_WIDTH: float = 0.01
const WIRE_THICKNESS_SCALE: float = 0.0015
const EPSILON: float = 0.0001

var _tractions: Dictionary[RID, TractionState] = {}
var _next_traction_id: int = 0
var _stream_owner: int = -1


func create_traction() -> RID:
    var state: TractionState = TractionState.new()
    state.primary_mesh_instance = RenderingServer.instance_create()
    state.secondary_mesh_instance = RenderingServer.instance_create()

    _next_traction_id += 1
    var traction_rid: RID = rid_from_int64(_next_traction_id)
    _tractions[traction_rid] = state

    return traction_rid


func free_traction(traction_rid: RID) -> void:
    var state: TractionState = _tractions.get(traction_rid)
    if not state:
        return

    if state.stream_rid.is_valid():
        SceneryStreamingServer.stream_free(state.stream_rid)
    if state.primary_mesh_instance.is_valid():
        RenderingServer.free_rid(state.primary_mesh_instance)
    if state.secondary_mesh_instance.is_valid():
        RenderingServer.free_rid(state.secondary_mesh_instance)

    state.primary_mesh = null
    state.secondary_mesh = null
    _tractions.erase(traction_rid)


func set_traction_scenario(traction_rid: RID, scenario: RID) -> void:
    var state: TractionState = _tractions.get(traction_rid)
    if not state:
        return

    RenderingServer.instance_set_scenario(state.primary_mesh_instance, scenario)
    RenderingServer.instance_set_scenario(state.secondary_mesh_instance, scenario)


func set_traction_transform(traction_rid: RID, transform: Transform3D) -> void:
    var state: TractionState = _tractions.get(traction_rid)
    if not state:
        return

    RenderingServer.instance_set_transform(state.primary_mesh_instance, transform)
    RenderingServer.instance_set_transform(state.secondary_mesh_instance, transform)


func set_traction_visible(traction_rid: RID, visible: bool) -> void:
    var state: TractionState = _tractions.get(traction_rid)
    if not state:
        return

    RenderingServer.instance_set_visible(state.primary_mesh_instance, visible)
    RenderingServer.instance_set_visible(state.secondary_mesh_instance, visible)


func set_traction_material(traction_rid: RID, material_rid: RID) -> void:
    var state: TractionState = _tractions.get(traction_rid)
    if not state:
        return

    RenderingServer.instance_geometry_set_material_override(state.primary_mesh_instance, material_rid)
    RenderingServer.instance_geometry_set_material_override(state.secondary_mesh_instance, material_rid)


func set_traction_geometry(
    traction_rid: RID,
    contact_p1: Vector3,
    contact_p2: Vector3,
    support_p1: Vector3,
    support_p2: Vector3,
    wire_thickness: float,
    wires: Wires,
    wire_offset: float,
    min_height: float,
    segment_length: float
) -> void:
    var state: TractionState = _tractions.get(traction_rid)
    if not state:
        return

    state.contact_p1 = contact_p1
    state.contact_p2 = contact_p2
    state.support_p1 = support_p1
    state.support_p2 = support_p2
    state.wire_thickness = wire_thickness
    state.wires = wires
    state.wire_offset = wire_offset
    state.min_height = min_height
    state.segment_length = segment_length
    if state.stream_rid.is_valid() and not state.streamed:
        return # out of range: built by _stream_build() when the camera comes into range
    _build_traction_meshes(state)


## Registers the span for streaming instead of building its wires: a scenery has thousands of
## spans and each one is two RenderingServer instances with no visibility range of their own.
## The meshes are built when the camera comes within the streaming draw distance of the chunk
## holding the span (SceneryStreamingServer) and dropped when it leaves.
func stream_traction(traction_rid: RID) -> void:
    var state: TractionState = _tractions.get(traction_rid)
    if not state:
        return
    if _stream_owner < 0:
        _stream_owner = SceneryStreamingServer.owner_create("traction", Callable(), _stream_build, _stream_clear)
    state.stream_rid = SceneryStreamingServer.stream_register(
        _stream_owner, traction_rid, state.contact_p1, 0.0
    )
    _stream_clear(traction_rid)


func _stream_build(traction_rid: RID, _preloaded: Variant) -> void:
    var state: TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.streamed = true
    _build_traction_meshes(state)


func _stream_clear(traction_rid: RID) -> void:
    var state: TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.streamed = false
    state.primary_mesh = null
    state.secondary_mesh = null
    RenderingServer.instance_set_base(state.primary_mesh_instance, RID())
    RenderingServer.instance_set_base(state.secondary_mesh_instance, RID())


func _build_traction_meshes(state: TractionState) -> void:
    _update_traction_meshes(
        state,
        state.contact_p1,
        state.contact_p2,
        state.support_p1,
        state.support_p2,
        state.wire_thickness,
        state.wires,
        state.wire_offset,
        state.min_height,
        state.segment_length
    )
    RenderingServer.instance_set_base(state.primary_mesh_instance, state.primary_mesh.get_rid() if state.primary_mesh else RID())
    RenderingServer.instance_set_base(state.secondary_mesh_instance, state.secondary_mesh.get_rid() if state.secondary_mesh else RID())

func _update_traction_meshes(
    state: TractionState,
    contact_p1: Vector3,
    contact_p2: Vector3,
    support_p1: Vector3,
    support_p2: Vector3,
    wire_thickness: float,
    wires: Wires,
    wire_offset: float,
    min_height: float,
    segment_length: float
) -> void:
    var primary_segments: Array[SegmentData] = []
    var secondary_segments: Array[SegmentData] = []
    if (
        contact_p1.distance_to(contact_p2) <= EPSILON
        or support_p1.distance_to(support_p2) <= EPSILON
    ):
        state.primary_mesh = ArrayMesh.new()
        state.secondary_mesh = null
        return

    var planar_span: Vector3 = Vector3(contact_p2.x - contact_p1.x, 0.0, contact_p2.z - contact_p1.z)
    var span_length: float = planar_span.length()
    if span_length <= EPSILON:
        state.primary_mesh = ArrayMesh.new()
        state.secondary_mesh = null
        return

    var lateral_axis: Vector3 = Vector3(-planar_span.z, 0.0, planar_span.x).normalized()
    var contact_offset: Vector3 = lateral_axis * (0.0 if wires == Wires.CONTACT_SUPPORT else wire_offset)
    var left_contact_p1: Vector3 = contact_p1 - contact_offset
    var left_contact_p2: Vector3 = contact_p2 - contact_offset
    var right_contact_p1: Vector3 = contact_p1 + contact_offset
    var right_contact_p2: Vector3 = contact_p2 + contact_offset
    var left_contact_hint: Vector3 = _get_segment_axis_hint(left_contact_p1, left_contact_p2, lateral_axis)
    var right_contact_hint: Vector3 = _get_segment_axis_hint(right_contact_p1, right_contact_p2, lateral_axis)
    var support_hint: Vector3 = _get_segment_axis_hint(support_p1, support_p2, lateral_axis)

    primary_segments.append(SegmentData.new(left_contact_p1, left_contact_p2, left_contact_hint))

    var support_span: Vector3 = support_p2 - support_p1
    var contact_span: Vector3 = contact_p2 - contact_p1
    var traction_length: float = contact_p1.distance_to(contact_p2)
    var num_sections: int = int(traction_length / segment_length) if segment_length > EPSILON else 0
    var step: float = 1.0 / float(num_sections) if num_sections > 0 else 0.0
    var sag_height: float = ((support_p1.y - contact_p1.y) + (support_p2.y - contact_p2.y)) * 0.5 - min_height
    var mid: float = 0.5
    var f: float = step

    if not wires == Wires.CONTACT:
        var support_points: Array[Vector3] = [support_p1]
        for index: int in range(num_sections - 1):
            var support_point: Vector3 = support_p1 + support_span * f
            var t: float = max(0.0, 1.0 - abs(f - mid) * 2.0)
            if not wires == Wires.TWO_CONTACT_TWO_SUPPORT or (not index == 0 and not index == num_sections - 2):
                support_point.y -= sqrt(t) * sag_height
                support_points.append(support_point)
            f += step
        support_points.append(support_p2)
        for index: int in range(support_points.size() - 1):
            primary_segments.append(SegmentData.new(support_points[index], support_points[index + 1], support_hint))

    if wires == Wires.TWO_CONTACT_ONE_SUPPORT or wires == Wires.TWO_CONTACT_TWO_SUPPORT:
        secondary_segments.append(SegmentData.new(right_contact_p1, right_contact_p2, right_contact_hint))

    f = step
    if wires == Wires.TWO_CONTACT_TWO_SUPPORT:
        var secondary_support_points: Array[Vector3] = [
            Vector3(support_p1.x, support_p1.y - 0.65 * sag_height, support_p1.z)
        ]
        for index: int in range(num_sections - 1):
            var support_point: Vector3 = support_p1 + support_span * f
            var t: float = max(0.0, 1.0 - abs(f - mid) * 2.0)
            var end_offset: float = 0.25 * sag_height if index == 0 or index == num_sections - 2 else 0.05
            support_point.y -= sqrt(t) * sag_height + end_offset
            secondary_support_points.append(support_point)
            f += step
        secondary_support_points.append(Vector3(support_p2.x, support_p2.y - 0.65 * sag_height, support_p2.z))
        for index: int in range(secondary_support_points.size() - 1):
            secondary_segments.append(SegmentData.new(secondary_support_points[index], secondary_support_points[index + 1], support_hint))

    f = step
    if not wires == Wires.CONTACT:
        var end_support_offset: float = 0.25 * sag_height if wires == Wires.TWO_CONTACT_TWO_SUPPORT else 0.0
        var mid_support_offset: float = 0.05 if wires == Wires.TWO_CONTACT_TWO_SUPPORT else 0.0
        for index: int in range(num_sections - 1):
            var support_anchor: Vector3 = support_p1 + support_span * f
            var contact_anchor: Vector3 = contact_p1 + contact_span * f
            var t: float = max(0.0, 1.0 - abs(f - mid) * 2.0)
            var hanger_offset: float = end_support_offset if index == 0 or index == num_sections - 2 else mid_support_offset
            support_anchor.y -= sqrt(t) * sag_height + hanger_offset

            if wires == Wires.CONTACT_SUPPORT:
                primary_segments.append(SegmentData.new(support_anchor, contact_anchor, lateral_axis))
            elif index % 2 == 0:
                var left_contact_anchor: Vector3 = contact_anchor - contact_offset
                primary_segments.append(SegmentData.new(support_anchor, left_contact_anchor, lateral_axis))
            else:
                var right_contact_anchor: Vector3 = contact_anchor + contact_offset
                secondary_segments.append(SegmentData.new(support_anchor, right_contact_anchor, lateral_axis))
            if wires == Wires.TWO_CONTACT_TWO_SUPPORT and (index == 1 or index == num_sections - 3):
                var lower_support: Vector3 = support_anchor
                lower_support.y -= 0.05
                secondary_segments.append(SegmentData.new(lower_support, support_anchor, lateral_axis))
            f += step

    state.primary_mesh = _create_segment_mesh(primary_segments, wire_thickness)
    state.secondary_mesh = _create_segment_mesh(secondary_segments, wire_thickness) if secondary_segments else null

func _get_segment_axis_hint(start_point: Vector3, end_point: Vector3, fallback_axis: Vector3) -> Vector3:
    var segment_direction: Vector3 = (end_point - start_point).normalized()
    var axis_hint: Vector3 = segment_direction.cross(Vector3.UP)
    if axis_hint.length_squared() <= EPSILON:
        axis_hint = fallback_axis
    return axis_hint.normalized() if axis_hint.length_squared() > EPSILON else Vector3.RIGHT


func _create_segment_mesh(segments: Array[SegmentData], wire_thickness: float) -> ArrayMesh:
    var mesh: ArrayMesh = ArrayMesh.new()
    if not segments:
        return mesh

    var width: float = max(wire_thickness * WIRE_THICKNESS_SCALE, MIN_SEGMENT_WIDTH)
    var surface_tool: SurfaceTool = SurfaceTool.new()
    surface_tool.begin(Mesh.PRIMITIVE_TRIANGLES)

    for segment: SegmentData in segments:
        _append_ribbon_segment(surface_tool, segment, width)

    var committed_mesh: ArrayMesh = surface_tool.commit()
    return committed_mesh


func _append_ribbon_segment(surface_tool: SurfaceTool, segment: SegmentData, width: float) -> void:
    var segment_vector: Vector3 = segment.finish - segment.start
    var segment_length: float = segment_vector.length()
    if segment_length <= EPSILON:
        return

    var segment_direction: Vector3 = segment_vector / segment_length
    var width_axis: Vector3 = _get_ribbon_width_axis(segment_direction, segment.width_axis_hint)
    var depth_axis: Vector3 = width_axis.cross(segment_direction).normalized()
    var half_width: Vector3 = width_axis * (width * 0.5)
    var half_depth: Vector3 = depth_axis * (width * 0.5)

    var start_left_bottom: Vector3 = segment.start - half_width - half_depth
    var start_right_bottom: Vector3 = segment.start + half_width - half_depth
    var start_right_top: Vector3 = segment.start + half_width + half_depth
    var start_left_top: Vector3 = segment.start - half_width + half_depth
    var end_left_bottom: Vector3 = segment.finish - half_width - half_depth
    var end_right_bottom: Vector3 = segment.finish + half_width - half_depth
    var end_right_top: Vector3 = segment.finish + half_width + half_depth
    var end_left_top: Vector3 = segment.finish - half_width + half_depth

    _append_quad(
        surface_tool,
        start_left_top,
        end_left_top,
        end_right_top,
        start_right_top,
        depth_axis,
        segment_length
    )
    _append_quad(
        surface_tool,
        start_right_bottom,
        end_right_bottom,
        end_left_bottom,
        start_left_bottom,
        -depth_axis,
        segment_length
    )
    _append_quad(
        surface_tool,
        start_right_top,
        end_right_top,
        end_right_bottom,
        start_right_bottom,
        width_axis,
        segment_length
    )
    _append_quad(
        surface_tool,
        start_left_bottom,
        end_left_bottom,
        end_left_top,
        start_left_top,
        -width_axis,
        segment_length
    )


func _append_triangle(
    surface_tool: SurfaceTool,
    first: Vector3,
    second: Vector3,
    third: Vector3,
    normal: Vector3,
    u_start: float,
    u_end: float
) -> void:
    surface_tool.set_normal(normal)
    surface_tool.set_uv(Vector2(u_start, 0.0))
    surface_tool.add_vertex(first)
    surface_tool.set_normal(normal)
    surface_tool.set_uv(Vector2(u_start, 1.0))
    surface_tool.add_vertex(second)
    surface_tool.set_normal(normal)
    surface_tool.set_uv(Vector2(u_end, 0.0))
    surface_tool.add_vertex(third)


func _append_quad(
    surface_tool: SurfaceTool,
    first: Vector3,
    second: Vector3,
    third: Vector3,
    fourth: Vector3,
    normal: Vector3,
    u_end: float
) -> void:
    _append_triangle(surface_tool, first, second, third, normal, 0.0, u_end)
    _append_triangle(surface_tool, first, third, fourth, normal, 0.0, u_end)


func _get_ribbon_width_axis(segment_direction: Vector3, axis_hint: Vector3) -> Vector3:
    var axis: Vector3 = segment_direction.cross(Vector3.UP)
    if axis.length_squared() <= EPSILON:
        axis = axis_hint - segment_direction * segment_direction.dot(axis_hint)
    if axis.length_squared() <= EPSILON:
        axis = segment_direction.cross(Vector3.RIGHT)
    if axis.length_squared() <= EPSILON:
        axis = segment_direction.cross(Vector3.FORWARD)
    if axis.length_squared() <= EPSILON:
        return Vector3.RIGHT
    return axis.normalized()
