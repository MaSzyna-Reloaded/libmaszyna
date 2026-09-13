@tool
@abstract
extends RefCounted
class_name MaszynaSkyEnvironment

var environment_node: Node


func _init(node: Node) -> void:
    environment_node = node


@abstract func create_sky() -> Sky


@abstract func create_nodes(
    world_environment: WorldEnvironment, environment: Environment
) -> void


@abstract func bind_nodes(world_environment: WorldEnvironment) -> void


@abstract func apply_visual_configuration() -> void


@abstract func set_date(year: int, month: int, day: int) -> Vector3i


@abstract func apply_time_configuration() -> void


@abstract func get_date() -> Vector3i


@abstract func get_current_time() -> float
