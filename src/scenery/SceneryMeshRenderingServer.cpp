#include "SceneryMeshRenderingServer.hpp"

#include "scenery/SceneryStreamingServer.hpp"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void SceneryMeshRenderingServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("mesh_create", "mesh", "transform", "range_end", "scenario"),
                &SceneryMeshRenderingServer::mesh_create);
        ClassDB::bind_method(
                D_METHOD("mesh_set_scenario", "mesh", "scenario"), &SceneryMeshRenderingServer::mesh_set_scenario);
        ClassDB::bind_method(D_METHOD("mesh_free", "mesh"), &SceneryMeshRenderingServer::mesh_free);
    }

    SceneryMeshRenderingServer::SceneryMeshRenderingServer() {
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL(streaming);
        // the owner lives as long as the streaming does
        stream_owner = streaming->owner_create(
                "meshes", Callable(), callable_mp(this, &SceneryMeshRenderingServer::_stream_build),
                callable_mp(this, &SceneryMeshRenderingServer::_stream_clear));
    }

    SceneryMeshRenderingServer::~SceneryMeshRenderingServer() {
        RenderingServer *rendering = RenderingServer::get_singleton();
        for (const KeyValue<RID, MeshPlacement> &item: placements) {
            if (item.value.instance.is_valid() && rendering != nullptr) {
                rendering->free_rid(item.value.instance);
            }
        }
    }

    RID SceneryMeshRenderingServer::mesh_create(
            const Ref<Mesh> &p_mesh, const Transform3D &p_transform, const float p_range_end, const RID &p_scenario) {
        ERR_FAIL_COND_V(p_mesh.is_null(), RID());
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL_V(streaming, RID());
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        MeshPlacement placement;
        placement.mesh = p_mesh;
        placement.transform = p_transform;
        placement.scenario = p_scenario;
        placements.insert(rid, placement);
        placements[rid].stream_rid = streaming->stream_register(
                stream_owner, rid, p_transform.xform(p_mesh->get_aabb().get_center()), p_range_end);
        return rid;
    }

    void SceneryMeshRenderingServer::mesh_set_scenario(const RID &p_mesh, const RID &p_scenario) {
        MeshPlacement *placement = placements.getptr(p_mesh);
        ERR_FAIL_NULL(placement);
        placement->scenario = p_scenario;
        if (placement->instance.is_valid()) {
            RenderingServer::get_singleton()->instance_set_scenario(placement->instance, p_scenario);
        }
    }

    void SceneryMeshRenderingServer::mesh_free(const RID &p_mesh) {
        MeshPlacement *placement = placements.getptr(p_mesh);
        ERR_FAIL_NULL(placement);
        if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
            streaming->stream_free(placement->stream_rid);
        }
        if (placement->instance.is_valid()) {
            RenderingServer::get_singleton()->free_rid(placement->instance);
        }
        placements.erase(p_mesh);
    }

    void SceneryMeshRenderingServer::_stream_build(const RID &p_mesh, const Variant & /*p_preloaded*/) {
        MeshPlacement *placement = placements.getptr(p_mesh);
        if (placement == nullptr || placement->instance.is_valid()) {
            return;
        }
        RenderingServer *rendering = RenderingServer::get_singleton();
        placement->instance = rendering->instance_create2(placement->mesh->get_rid(), placement->scenario);
        rendering->instance_set_transform(placement->instance, placement->transform);
    }

    void SceneryMeshRenderingServer::_stream_clear(const RID &p_mesh) {
        MeshPlacement *placement = placements.getptr(p_mesh);
        if (placement == nullptr || !placement->instance.is_valid()) {
            return;
        }
        RenderingServer::get_singleton()->free_rid(placement->instance);
        placement->instance = RID();
    }
} // namespace godot
