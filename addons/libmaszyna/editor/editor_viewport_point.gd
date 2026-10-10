@tool
extends RefCounted
class_name MaszynaEditorViewportPoint

## Where a point of an editor 3D view lands in the world - what is put there from a panel stands
## on what the view shows under that point

## How far the ray from the view looks for something to stand on [m]
const RAY_LENGTH: float = 100000.0
## Where along the ray a point lands when it hits nothing and looks above the horizon [m]
const FALLBACK_DISTANCE: float = 10.0


## The first thing the camera's ray through screen_position hits, else the ground plane, else a
## point FALLBACK_DISTANCE along the ray
static func world_point(viewport_camera: Camera3D, screen_position: Vector2) -> Vector3:
    var ray_origin: Vector3 = viewport_camera.project_ray_origin(screen_position)
    var ray_direction: Vector3 = viewport_camera.project_ray_normal(screen_position)
    var ray_end: Vector3 = ray_origin + ray_direction * RAY_LENGTH
    var world: World3D = viewport_camera.get_world_3d()

    if world:
        var query: PhysicsRayQueryParameters3D = PhysicsRayQueryParameters3D.create(ray_origin, ray_end)
        var hit: Dictionary = world.direct_space_state.intersect_ray(query)
        if hit.has("position"):
            return hit["position"]

    var ground_plane: Plane = Plane(Vector3.UP, 0.0)
    var ground_hit: Variant = ground_plane.intersects_ray(ray_origin, ray_direction)
    if ground_hit is Vector3:
        return ground_hit

    return ray_origin + ray_direction * FALLBACK_DISTANCE
