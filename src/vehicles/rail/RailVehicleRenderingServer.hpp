#pragma once
#include "legacy/e3d/E3DModel.hpp"
#include "vehicles/rail/RailVehicleAppearance.hpp"
#include "vehicles/rail/RailVehicleCabinKind.hpp"
#include "vehicles/rail/RailVehicleEnginePowerSource.hpp"

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_dictionary.hpp>

#include <array>

namespace godot {
    /* What a rail vehicle looks like, drawn: its models, the submodels that move and how they
     * move, its lights, smoke, couplers, head display, low-poly interior, the detail it is drawn in
     * and where the player finds it. Keyed by the vehicle's RailVehicleServer handle.
     *
     * The vehicle is drawn at a node (vehicle_attach()) - the one its cab, sounds and anything
     * else of the vehicle ride on; this server moves that node wherever RailVehicleServer places
     * the vehicle, and draws the models there. What moves is animated from the vehicle's state
     * natively, once a frame for the vehicles near the camera and a few times a second for the
     * rest - no script runs per vehicle. */
    class RailVehicleRenderingServer : public Object {
            GDCLASS(RailVehicleRenderingServer, Object)

        public:
            /* The vehicle's exterior model was built - as nodes near the camera, as
             * RenderingServer instances far from it; what dresses its nodes does it again */
            static const char *vehicle_model_built_signal;

            /* maszyna/vehicles/detail_distance [m]: nearer than this a vehicle is drawn as nodes and
             * animated, further it is drawn as RenderingServer instances and holds still */
            static constexpr const char *DETAIL_DISTANCE_SETTING = "maszyna/vehicles/detail_distance";
            static constexpr float DEFAULT_DETAIL_DISTANCE = 350.0;
            /* A vehicle drawn in detail keeps it until this much nearer than the detail distance,
             * so one on the boundary is not rebuilt over and over - proportional, as a fixed margin
             * is nothing at a long detail distance and more than the distance at a short one */
            static constexpr float DETAIL_HYSTERESIS = 0.25;
            static constexpr float DETAIL_HYSTERESIS_MIN = 25.0;
            /* How often every vehicle's detail, lights and smoke are looked at [s] - a plume and a
             * lamp change slowly - and how many vehicles a frame may look at doing it */
            static constexpr double SLOW_UPDATE_PERIOD = 0.25;
            static constexpr int MAX_SLOW_UPDATES_PER_FRAME = 16;
            /* A door's submodels: itself and the two below it a folding door turns (DynObj.cpp:592-622) */
            static constexpr int DOOR_ELEMENTS = 3;
            /* How many vehicles drawn in detail a frame animates (pantographs, wipers, mirrors) */
            static constexpr int MAX_DETAILED_UPDATES_PER_FRAME = 32;
            /* Time the models of the vehicles coming within the draw distance may take to build a
             * frame [ms]; a build that started is finished, so a frame builds at least one - the
             * streaming's own budget (SceneryStreamingServer::BUDGET_MSEC) */
            static constexpr uint64_t BUILD_BUDGET_MSEC = 4;

        private:
            static RailVehicleRenderingServer *singleton;

            /* The low-poly interior's cabs, cab0 for a vehicle's single one (DynObj.cpp:2383-2391) */
            static constexpr std::array<const char *, 3> LOW_POLY_CABS = {"cab0", "cab1", "cab2"};

            /* A submodel of the exterior model that moves, and where it rests in the vehicle's own
             * frame - what its pose turns it from */
            struct Part {
                    String submodel;
                    Transform3D rest;
            };

            struct Visual {
                    /* The scene node whose world the vehicle is drawn in - its space, and the parent
                     * of the holder; this server never moves it */
                    ObjectID node;
                    /* The world it is drawn in, none while its scene node is out of one */
                    RID scenario;
                    /* Where the vehicle stands, once it has a place: RailVehicleServer placed it on
                     * a track, or it was told (vehicle_set_transform()). Until then its models are
                     * in no world - a scenery's vehicles are built before their trainset stands
                     * them on its track, and were drawn at the origin meanwhile */
                    Transform3D transform;
                    bool placed = false;
                    /* The node the models of a vehicle drawn in detail are built under - this
                     * server's own, there only while the vehicle is detailed */
                    ObjectID holder;
                    /* Nodes of other layers riding on the vehicle (vehicle_mount_node()) */
                    Vector<ObjectID> mounts;
                    Ref<RailVehicleAppearance> appearance;
                    /* The exterior and the low-poly interior - this server's own when it built them
                     * from the appearance, else whoever handed them over owns them */
                    RID model;
                    RID low_poly;
                    RID passengers;
                    Vector<RID> attachments;
                    /* The coupler adapter fitted to each end, drawn as the exterior is */
                    RID coupler_adapters[2];
                    RID load;
                    /* The cargo's model file, read again with the game's data */
                    String load_data_path;
                    String load_model_filename;
                    bool own_models = false;
                    /* The appearance's model files registered with ResourceLazyLoader, by file
                     * name - loaded at once without lazy loading - and what the models built from
                     * them hold, one per model */
                    HashMap<String, RID> model_resources;
                    Vector<RID> held_models;
                    /* Waiting in pending_builds for its models to be built */
                    bool build_pending = false;
                    /* The appearance's model could not be loaded - not built again until another
                     * appearance is set */
                    bool model_missing = false;
                    Transform3D model_transform;
                    /* How far the cargo sinks into an empty vehicle [m] (DynObj.cpp:3070-3080) */
                    double load_height = 0.0;
                    Part bogies[2];
                    /* front rolling, powered, rear rolling */
                    Vector<Part> wheels[3];
                    /* RailVehicleEnginePowerSource::PantographSelector: lower arm, its pair, upper arm,
                     * its pair, slider */
                    Vector<Part> pantograph_arms[2];
                    Vector<Part> wiper_arms;
                    Vector<Part> mirrors;
                    /* RailVehicleAppearance::get_doors(): DOOR_ELEMENTS a door */
                    Vector<Part> doors;
                    Vector<Part> door_steps;
                    Vector<Part> pendulums;
                    double pendulum_amplitude = 0.0;
                    /* The coupler and hose submodels the model has (AirCoupler::Init(),
                     * DynObj.cpp:2170-2181) */
                    HashSet<String> coupler_submodels;
                    int64_t coupler_state = -1;
                    /* Every pose of the exterior, sent to it in one call */
                    Dictionary poses;
                    TypedDictionary<String, bool> lights;
                    bool headlights_dimmed = false;
                    Ref<Material> head_display_material;
                    /* Born optimized: a scenery's vehicles built as node hierarchies (cars,
                     * interiors) filled the load; _update_detail() details the near ones */
                    bool detailed = false;
                    /* vehicle_set_editable(): drawn in detail wherever the camera is, its nodes
                     * shown in the editor's Scene dock */
                    bool editable = false;
                    RID pickable;
                    RID detection_area;
                    RID detection_shape;
                    /* vehicle_set_visible_low_poly_cabins() */
                    bool low_poly_cabs_visible = true;
                    /* The level of each low-poly cab's light (cabin_set_light_level()), and the
                     * self-illumination the cab is drawn with, following it */
                    double cab_light_levels[LOW_POLY_CABS.size()] = {};
                    double cab_light_energies[LOW_POLY_CABS.size()] = {};
                    PackedFloat64Array wiper_positions;
                    double mirror_left = -1.0;
                    double mirror_right = -1.0;
                    RailVehicleCabinKind::Kind mirror_cabin = RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE;
                    /* The door and step positions last posed, left and right */
                    double door_positions[2] = {-1.0, -1.0};
                    double door_step_positions[2] = {-1.0, -1.0};
            };

            enum PneumaticLine {
                PNEUMATIC_LINE_BRAKE,
                PNEUMATIC_LINE_MAIN,
            };

            enum PneumaticLayout {
                PNEUMATIC_LAYOUT_NONE,
                PNEUMATIC_LAYOUT_LEFT,
                PNEUMATIC_LAYOUT_RIGHT,
                PNEUMATIC_LAYOUT_BOTH,
            };

            /* A mechanical coupler uses OFF, ON and XON. An air hose additionally selects the right
             * submodel, slanted or straight (TDynamicObject::SetPneumatic(), DynObj.cpp:497). */
            enum CouplerVariant {
                COUPLER_VARIANT_OFF,
                COUPLER_VARIANT_ON,
                COUPLER_VARIANT_XON,
                COUPLER_VARIANT_RIGHT_XON,
                COUPLER_VARIANT_RIGHT_ON,
                COUPLER_VARIANT_COUNT,
            };

            HashMap<RID, Visual> vehicles;
            /* The vehicle each exterior model draws */
            HashMap<RID, RID> model_vehicles;
            /* The vehicle each detection area finds */
            HashMap<RID, RID> area_vehicles;
            /* The vehicles in the order the frame visits them, and where each visit left off */
            Vector<RID> visit_order;
            int slow_cursor = 0;
            int detailed_cursor = 0;
            double slow_elapsed = 0.0;
            /* The vehicles whose low-poly cabs are still following their cab lights */
            Vector<RID> fading;
            /* A vehicle within the draw distance whose models wait for their build, and how far
             * from the camera it was when it came there - the nearest are built first */
            struct PendingBuild {
                    RID vehicle;
                    double distance = 0.0;
            };
            Vector<PendingBuild> pending_builds;
            bool processing = false;
            /* DETAIL_DISTANCE_SETTING, read when the settings change - not per vehicle visited */
            float detail_distance = DEFAULT_DETAIL_DISTANCE;

            static Node3D *_node(const Visual &p_visual);
            void _set_processing(bool p_processing);
            void _process_frame();
            void _on_data_reload_requested();
            void _on_streaming_camera_changed();
            void _on_project_settings_changed();
            void _free_models(Visual &p_visual);
            static void _free_model_resources(Visual &p_visual);
            void _create_models(const RID &p_vehicle, Visual &p_visual);
            void _bind_parts(const RID &p_vehicle, Visual &p_visual);
            Part _part(const Visual &p_visual, const String &p_submodel) const;
            Vector<Part> _parts(const Visual &p_visual, const PackedStringArray &p_submodels) const;
            void _publish_pantographs(const RID &p_vehicle, const Visual &p_visual) const;
            void _publish_pantograph_geometry(
                    const RID &p_vehicle, const Visual &p_visual, const Ref<E3DModel> &p_model,
                    RailVehicleEnginePowerSource::PantographSelector p_pantograph) const;
            void _place(const RID &p_vehicle, Visual &p_visual);
            void _move(const RID &p_vehicle, Visual &p_visual);
            void _show_models(const Visual &p_visual, const RID &p_scenario) const;
            void _pose(Visual &p_visual, const Part &p_part, const Basis &p_pose);
            void _pose(Visual &p_visual, const Part &p_part, const Transform3D &p_pose);
            void _pose_doors(const RID &p_vehicle, Visual &p_visual);
            void _pose_pendulums(const RID &p_vehicle, Visual &p_visual);
            void _pose_running_gear(const RID &p_vehicle, Visual &p_visual);
            void _pose_pantographs(const RID &p_vehicle, Visual &p_visual);
            void _pose_wipers(const RID &p_vehicle, Visual &p_visual);
            void _pose_mirrors(const RID &p_vehicle, Visual &p_visual);
            void _send_poses(Visual &p_visual);
            void _update_couplers(const RID &p_vehicle, Visual &p_visual);
            PneumaticLayout _pneumatic_layout(
                    const Visual &p_visual, RailVehicleController::CouplerEnd p_end, PneumaticLine p_line) const;
            CouplerVariant _pneumatic_variant(
                    const RID &p_vehicle, const Visual &p_visual, RailVehicleController::CouplerEnd p_end,
                    PneumaticLine p_line) const;
            void _show_air_coupler(const Visual &p_visual, const String &p_name, bool p_on, bool p_xon);
            void _update_lights(const RID &p_vehicle, Visual &p_visual);
            void _update_smoke(const RID &p_vehicle, const Visual &p_visual) const;
            void _update_detail(const RID &p_vehicle, Visual &p_visual);
            void _set_detailed(const RID &p_vehicle, Visual &p_visual, bool p_detailed);
            /* The models built from the appearance - created, the parts found in them, the cargo */
            void _build_models(const RID &p_vehicle, Visual &p_visual);
            void _cancel_build(const RID &p_vehicle, Visual &p_visual);
            void _build_load(const RID &p_vehicle, Visual &p_visual);
            void _update_low_poly_cabs(const RID &p_vehicle, const Visual &p_visual) const;
            void _on_vehicle_driver_cabin_changed(const RID &p_vehicle, const RID &p_cabin);
            void _update_load(const RID &p_vehicle, Visual &p_visual);
            void _update_detection_area(const RID &p_vehicle, Visual &p_visual);
            void _register_pickable(const RID &p_vehicle, Visual &p_visual);
            void _on_vehicle_placed(const RID &p_vehicle);
            void _on_vehicle_trainset_changed(const RID &p_vehicle);
            void _on_vehicle_coupler_changed(const RID &p_vehicle, int64_t p_flag);
            void _on_vehicle_coupler_adapter_changed(const RID &p_vehicle, int64_t p_end);
            /* The adapters fitted to the vehicle's ends, built again as its controller has them */
            void _update_coupler_adapters(const RID &p_vehicle, Visual &p_visual);
            /* Where an adapter fitted to an end is drawn (Render_coupler_adapter(), opengl33renderer.cpp:1356-1376) */
            Transform3D _coupler_adapter_transform(const RID &p_vehicle, const Visual &p_visual, int p_end) const;
            void _on_vehicle_config_changed(const RID &p_vehicle);
            void _on_vehicle_freed(const RID &p_vehicle);
            void _on_instance_built(const RID &p_instance);

        protected:
            static void _bind_methods();

        public:
            static RailVehicleRenderingServer *get_instance();

            RailVehicleRenderingServer();
            ~RailVehicleRenderingServer() override;

            /* The vehicle is drawn in the world of the scene node p_node_id, once it has a place:
             * where RailVehicleServer places it on a track, or where vehicle_set_transform() says.
             * The node itself is never moved (a node that is to follow the vehicle is mounted,
             * vehicle_mount_node()). Freed with the vehicle (VehicleServer.vehicle_freed). */
            void vehicle_attach(const RID &p_vehicle, uint64_t p_node_id);
            void vehicle_detach(const RID &p_vehicle);
            bool vehicle_is_attached(const RID &p_vehicle) const;
            /* Where a vehicle that stands on no track is drawn - one assembled by hand, where its
             * node stands. RailVehicleServer's placement takes over once the vehicle is on a track. */
            void vehicle_set_transform(const RID &p_vehicle, const Transform3D &p_transform);
            /* Where the vehicle is drawn; the identity for a vehicle not drawn, or without a place */
            Transform3D vehicle_get_transform(const RID &p_vehicle) const;
            /* A Node3D of another layer rides on the vehicle - a cab, a sound emitter, the node of
             * a vehicle assembled by hand: it is put where the vehicle stands, now and wherever
             * the vehicle is placed, until it is unmounted or the vehicle is freed */
            void vehicle_mount_node(const RID &p_vehicle, uint64_t p_node_id);
            void vehicle_unmount_node(const RID &p_vehicle, uint64_t p_node_id);
            /* The vehicle a detection area (PhysicsServer3D) finds - what the player's shape cast
             * hits; an empty RID for an area that is no vehicle's */
            RID detection_area_get_vehicle(const RID &p_area) const;
            /* The world the scene node is in, an empty RID out of it: the models this server built
             * are drawn there - the node's NOTIFICATION_ENTER_WORLD/EXIT_WORLD, as
             * VisualInstance3D's */
            void vehicle_set_scenario(const RID &p_vehicle, const RID &p_scenario);
            /* What the vehicle looks like. With model files, this server builds the models itself -
             * once the vehicle stands within the draw distance of the streaming's camera
             * (SceneryStreamingServer), and frees them beyond it; without, it draws the ones
             * vehicle_set_models() hands it. The pantographs' geometry is read off the exterior
             * model at once, for a vehicle with a current collector. */
            void vehicle_set_appearance(const RID &p_vehicle, const Ref<RailVehicleAppearance> &p_appearance);
            Ref<RailVehicleAppearance> vehicle_get_appearance(const RID &p_vehicle) const;
            /* The exterior and the low-poly interior, as E3DRenderingServer instances somebody else
             * owns - a vehicle assembled by hand */
            void vehicle_set_models(const RID &p_vehicle, const RID &p_model, const RID &p_low_poly);
            /* The exterior model as E3DRenderingServer draws it */
            RID vehicle_get_model(const RID &p_vehicle) const;
            /* The cargo, drawn at the floor of the vehicle (DynObj.cpp:866) while its models are -
             * none for an empty file name */
            void
            vehicle_set_load_model(const RID &p_vehicle, const String &p_data_path, const String &p_model_filename);
            void vehicle_set_head_display_material(const RID &p_vehicle, const Ref<Material> &p_material);
            /* Whether the low-poly interior shows every cab. Not visible - the interior of the
             * occupied cab is drawn in its place by whoever shows it - the low-poly cab of the
             * occupied cab is hidden, or all of them with jointcabs: (DynObj.cpp:1389-1397) */
            void vehicle_set_visible_low_poly_cabins(const RID &p_vehicle, bool p_visible);
            /* The level (0..1) of the light of a VehicleServer cabin that its low-poly cab - of
             * the cabin's kind (RailVehicleServer) - is lit at (TDynamicObject::set_cab_lights(),
             * DynObj.cpp:841-853); with jointcabs: every cab at the brightest */
            void cabin_set_light_level(const RID &p_cabin, double p_level);
            /* Whether the vehicle is drawn in detail - as nodes, animated */
            bool vehicle_is_detailed(const RID &p_vehicle) const;
            /* Whether the vehicle's models stand as nodes the editor shows and edits ("Edit FIZ"):
             * in detail wherever the camera is, under a holder of the scene, owned by the scene's
             * owner - so whoever saves the scene takes them off first */
            void vehicle_set_editable(const RID &p_vehicle, bool p_editable);
            bool vehicle_is_editable(const RID &p_vehicle) const;
            /* The vehicles within the draw distance whose models still wait for their build */
            int builds_get_pending_count() const;
    };
} // namespace godot
