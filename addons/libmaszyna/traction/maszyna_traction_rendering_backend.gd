@tool
extends TractionRenderingBackend
class_name MaszynaTractionRenderingBackend

## The traction drawn as the original draws it (Traction.cpp): the contact wires, the support
## wires sagging between the masts and the droppers, of copper or aluminium, with or without
## patina. Registered with TractionRenderingServer as "maszyna" (MaszynaSceneryProvider).

const TRACTION_AL_MATERIAL: Material = preload("../materials/traction_al.tres")
const TRACTION_CU_MATERIAL: Material = preload("../materials/traction_cu.tres")
const TRACTION_AL_PATINA_MATERIAL: Material = preload("../materials/traction_al_patina.tres")
const TRACTION_CU_PATINA_MATERIAL: Material = preload("../materials/traction_cu_patina.tres")


class MaszynaTractionState extends TractionState:
    var primary_mesh_instance: RID = RID()
    var secondary_mesh_instance: RID = RID()
    var primary_mesh: Mesh
    var secondary_mesh: Mesh


func make_state() -> TractionState:
    var state: MaszynaTractionState = MaszynaTractionState.new()
    state.primary_mesh_instance = RenderingServer.instance_create()
    state.secondary_mesh_instance = RenderingServer.instance_create()
    return state


func free_state(base_state: TractionState) -> void:
    var state: MaszynaTractionState = base_state as MaszynaTractionState
    RenderingServer.free_rid(state.primary_mesh_instance)
    RenderingServer.free_rid(state.secondary_mesh_instance)
    state.primary_mesh = null
    state.secondary_mesh = null


func apply_scenario(base_state: TractionState) -> void:
    var state: MaszynaTractionState = base_state as MaszynaTractionState
    RenderingServer.instance_set_scenario(state.primary_mesh_instance, state.scenario)
    RenderingServer.instance_set_scenario(state.secondary_mesh_instance, state.scenario)


func apply_transform(base_state: TractionState) -> void:
    var state: MaszynaTractionState = base_state as MaszynaTractionState
    RenderingServer.instance_set_transform(state.primary_mesh_instance, state.transform)
    RenderingServer.instance_set_transform(state.secondary_mesh_instance, state.transform)


func apply_visible(base_state: TractionState) -> void:
    var state: MaszynaTractionState = base_state as MaszynaTractionState
    RenderingServer.instance_set_visible(state.primary_mesh_instance, state.visible)
    RenderingServer.instance_set_visible(state.secondary_mesh_instance, state.visible)


## The span's wires, and their material: a .scn's span of its own (MaszynaTractionData), any other
## of copper; dulled by patina when its damage says so
func build(base_state: TractionState) -> void:
    var state: MaszynaTractionState = base_state as MaszynaTractionState
    var span: TractionData = state.description
    if not span:
        return
    _update_traction_meshes(
        state, span.contact_p1, span.contact_p2, span.support_p1, span.support_p2, span.wire_thickness,
        span.wires, span.wire_offset, span.min_height, span.segment_length
    )
    var scn_span: MaszynaTractionData = span as MaszynaTractionData
    var copper: bool = not scn_span or scn_span.material == MaszynaTractionData.WireMaterial.COPPER
    var material: Material = TRACTION_CU_MATERIAL if copper else TRACTION_AL_MATERIAL
    if span.damage_flag & MaszynaTractionData.DAMAGE_PATINA:
        material = TRACTION_CU_PATINA_MATERIAL if copper else TRACTION_AL_PATINA_MATERIAL
    for instance: RID in [state.primary_mesh_instance, state.secondary_mesh_instance]:
        RenderingServer.instance_geometry_set_material_override(instance, material.get_rid())
    RenderingServer.instance_set_base(state.primary_mesh_instance, state.primary_mesh.get_rid() if state.primary_mesh else RID())
    RenderingServer.instance_set_base(state.secondary_mesh_instance, state.secondary_mesh.get_rid() if state.secondary_mesh else RID())


func clear(base_state: TractionState) -> void:
    var state: MaszynaTractionState = base_state as MaszynaTractionState
    state.primary_mesh = null
    state.secondary_mesh = null
    RenderingServer.instance_set_base(state.primary_mesh_instance, RID())
    RenderingServer.instance_set_base(state.secondary_mesh_instance, RID())


func _update_traction_meshes(
    state: MaszynaTractionState,
    contact_p1: Vector3,
    contact_p2: Vector3,
    support_p1: Vector3,
    support_p2: Vector3,
    wire_thickness: float,
    wires: TractionRenderingServer.Wires,
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
    var contact_offset: Vector3 = lateral_axis * (0.0 if wires == TractionRenderingServer.Wires.CONTACT_SUPPORT else wire_offset)
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

    if not wires == TractionRenderingServer.Wires.CONTACT:
        var support_points: Array[Vector3] = [support_p1]
        for index: int in range(num_sections - 1):
            var support_point: Vector3 = support_p1 + support_span * f
            var t: float = max(0.0, 1.0 - abs(f - mid) * 2.0)
            if not wires == TractionRenderingServer.Wires.TWO_CONTACT_TWO_SUPPORT or (not index == 0 and not index == num_sections - 2):
                support_point.y -= sqrt(t) * sag_height
                support_points.append(support_point)
            f += step
        support_points.append(support_p2)
        for index: int in range(support_points.size() - 1):
            primary_segments.append(SegmentData.new(support_points[index], support_points[index + 1], support_hint))

    if wires == TractionRenderingServer.Wires.TWO_CONTACT_ONE_SUPPORT or wires == TractionRenderingServer.Wires.TWO_CONTACT_TWO_SUPPORT:
        secondary_segments.append(SegmentData.new(right_contact_p1, right_contact_p2, right_contact_hint))

    f = step
    if wires == TractionRenderingServer.Wires.TWO_CONTACT_TWO_SUPPORT:
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
    if not wires == TractionRenderingServer.Wires.CONTACT:
        var end_support_offset: float = 0.25 * sag_height if wires == TractionRenderingServer.Wires.TWO_CONTACT_TWO_SUPPORT else 0.0
        var mid_support_offset: float = 0.05 if wires == TractionRenderingServer.Wires.TWO_CONTACT_TWO_SUPPORT else 0.0
        for index: int in range(num_sections - 1):
            var support_anchor: Vector3 = support_p1 + support_span * f
            var contact_anchor: Vector3 = contact_p1 + contact_span * f
            var t: float = max(0.0, 1.0 - abs(f - mid) * 2.0)
            var hanger_offset: float = end_support_offset if index == 0 or index == num_sections - 2 else mid_support_offset
            support_anchor.y -= sqrt(t) * sag_height + hanger_offset

            if wires == TractionRenderingServer.Wires.CONTACT_SUPPORT:
                primary_segments.append(SegmentData.new(support_anchor, contact_anchor, lateral_axis))
            elif index % 2 == 0:
                var left_contact_anchor: Vector3 = contact_anchor - contact_offset
                primary_segments.append(SegmentData.new(support_anchor, left_contact_anchor, lateral_axis))
            else:
                var right_contact_anchor: Vector3 = contact_anchor + contact_offset
                secondary_segments.append(SegmentData.new(support_anchor, right_contact_anchor, lateral_axis))
            if wires == TractionRenderingServer.Wires.TWO_CONTACT_TWO_SUPPORT and (index == 1 or index == num_sections - 3):
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


