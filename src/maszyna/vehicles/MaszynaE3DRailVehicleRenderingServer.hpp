#pragma once
#include "maszyna/e3d/MaszynaE3DModel.hpp"
#include "utils/RIDHash.hpp"
#include "vehicles/rail/RailVehicleCabinKind.hpp"
#include "vehicles/rail/RailVehicleController.hpp"
#include "vehicles/rail/RailVehicleEnginePowerSource.hpp"
#include "vehicles/rail/RailVehicleRenderingServer.hpp"

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/typed_dictionary.hpp>

#include <array>

namespace godot {
    /* A MaSzyna vehicle drawn as the original draws it (TDynamicObject, DynObj.cpp): over the core's
     * drawing of its models and running gear, its pantographs, wipers, mirrors, doors, steps and
     * pendulums animated from its state, its couplers and hoses shown as they are coupled, its lamps
     * and smoke, its cargo, the coupler adapters fitted to its ends, and the low-poly cabs of its
     * interior hidden and lit. What moves is found by the names its MMD gives
     * (MaszynaRailVehicleAppearance). It stands in for RailVehicleRenderingServer under that name. */
    class MaszynaE3DRailVehicleRenderingServer : public RailVehicleRenderingServer {
            GDCLASS(MaszynaE3DRailVehicleRenderingServer, RailVehicleRenderingServer)

        public:
            /* A door's submodels: itself and the two below it a folding door turns (DynObj.cpp:592-622) */
            static constexpr int DOOR_ELEMENTS = 3;

        private:
            static MaszynaE3DRailVehicleRenderingServer *singleton;

            /* The low-poly interior's cabs, cab0 for a vehicle's single one (DynObj.cpp:2383-2391) */
            static constexpr std::array<const char *, 3> LOW_POLY_CABS = {"cab0", "cab1", "cab2"};

            struct MaszynaVisual {
                    /* The coupler adapter fitted to each end, drawn as the exterior is */
                    RID coupler_adapters[2];
                    RID load;
                    /* The cargo's model file, read again with the game's data */
                    String load_data_path;
                    String load_model_filename;
                    /* The cargo's and each end's coupler adapter's model file, registered as the
                     * appearance's are */
                    RID load_resource;
                    String coupler_adapter_models[2];
                    RID coupler_adapter_resources[2];
                    /* How far the cargo sinks into an empty vehicle [m] (DynObj.cpp:3070-3080) */
                    double load_height = 0.0;
                    /* RailVehicleEnginePowerSource::PantographSelector: lower arm, its pair, upper arm,
                     * its pair, slider */
                    Vector<Part> pantograph_arms[2];
                    Vector<Part> wiper_arms;
                    Vector<Part> mirrors;
                    /* MaszynaRailVehicleAppearance::get_doors(): DOOR_ELEMENTS a door */
                    Vector<Part> doors;
                    Vector<Part> door_steps;
                    Vector<Part> pendulums;
                    double pendulum_amplitude = 0.0;
                    /* The coupler and hose submodels the model has (AirCoupler::Init(),
                     * DynObj.cpp:2170-2181) */
                    HashSet<String> coupler_submodels;
                    int64_t coupler_state = -1;
                    TypedDictionary<String, bool> lights;
                    bool headlights_dimmed = false;
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
                    /* The low-poly interior's lights (MaszynaE3DLightFactory::make_interior_lights()) as last
                     * switched: on or off, and how bright, -1 for not yet */
                    float interior_light_intensity = -1.0F;
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

            RIDMap<MaszynaVisual> maszyna_vehicles;
            /* The vehicles whose low-poly cabs are still following their cab lights */
            Vector<RID> fading;

            void _register_coupler_adapters(const RID &p_vehicle, MaszynaVisual &p_maszyna);
            void _publish_pantographs(const RID &p_vehicle, const Visual &p_visual) const;
            void _publish_pantograph_geometry(
                    const RID &p_vehicle, const Visual &p_visual, const Ref<MaszynaE3DModel> &p_model,
                    RailVehicleEnginePowerSource::PantographSelector p_pantograph) const;
            void _pose_doors(const RID &p_vehicle, Visual &p_visual, MaszynaVisual &p_maszyna);
            void _pose_pendulums(const RID &p_vehicle, Visual &p_visual, const MaszynaVisual &p_maszyna);
            void _pose_pantographs(const RID &p_vehicle, Visual &p_visual, const MaszynaVisual &p_maszyna);
            void _pose_wipers(const RID &p_vehicle, Visual &p_visual, MaszynaVisual &p_maszyna);
            void _pose_mirrors(const RID &p_vehicle, Visual &p_visual, MaszynaVisual &p_maszyna);
            void _update_couplers(const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna);
            PneumaticLayout _pneumatic_layout(
                    const MaszynaVisual &p_maszyna, RailVehicleController::CouplerEnd p_end,
                    PneumaticLine p_line) const;
            CouplerVariant _pneumatic_variant(
                    const RID &p_vehicle, const MaszynaVisual &p_maszyna, RailVehicleController::CouplerEnd p_end,
                    PneumaticLine p_line) const;
            static void _show_air_coupler(
                    const Visual &p_visual, const MaszynaVisual &p_maszyna, const String &p_name, bool p_on,
                    bool p_xon);
            void _update_lights(const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna);
            void _update_smoke(const RID &p_vehicle, const Visual &p_visual) const;
            void _build_load(const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna);
            void _update_load(const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna);
            /* The snow drawn over the cargo, as over the exterior - but for a multiple unit's, whose
             * load is its passengers */
            void
            _send_load_snow(const RID &p_vehicle, const MaszynaVisual &p_maszyna, const Ref<Material> &p_overlay) const;
            void _update_low_poly_cabs(const RID &p_vehicle, const Visual &p_visual) const;
            /* Lit with the compartment lights - of a vehicle with a driver in its trainset, as the original
             * lights them only with an owner or a driver (DynObj.cpp:4238-4245) - as bright as they shine */
            void _update_interior_lights(const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna) const;
            /* The adapters fitted to the vehicle's ends, built again as its controller has them */
            void _update_coupler_adapters(const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna);
            /* Where an adapter fitted to an end is drawn (Render_coupler_adapter(), opengl33renderer.cpp:1356-1376) */
            Transform3D _coupler_adapter_transform(const RID &p_vehicle, const Visual &p_visual, int p_end) const;
            void _on_vehicle_trainset_changed(const RID &p_vehicle);
            void _on_vehicle_coupler_changed(const RID &p_vehicle, int64_t p_flag);
            void _on_vehicle_coupler_adapter_changed(const RID &p_vehicle, int64_t p_end);
            void _on_vehicle_config_changed(const RID &p_vehicle);
            void _on_data_reload_requested();

        protected:
            static void _bind_methods();

            void _vehicle_attached(const RID &p_vehicle) override;
            void _vehicle_detaching(const RID &p_vehicle) override;
            void _vehicle_appearance_changed(const RID &p_vehicle) override;
            void _vehicle_add_preload_requests(const RID &p_vehicle, Array &p_r_requests) const override;
            void _vehicle_preloaded(const RID &p_vehicle) override;
            void _vehicle_models_created(const RID &p_vehicle) override;
            void _vehicle_models_freed(const RID &p_vehicle) override;
            void _vehicle_models_changed(const RID &p_vehicle) override;
            void _vehicle_parts_bound(const RID &p_vehicle) override;
            void _vehicle_moved(const RID &p_vehicle) override;
            void _vehicle_scenario_changed(const RID &p_vehicle, const RID &p_scenario) override;
            void _vehicle_update(const RID &p_vehicle) override;
            void _vehicle_pose(const RID &p_vehicle) override;
            void _vehicle_shown_cabin_changed(const RID &p_vehicle) override;
            void _vehicle_snow_changed(const RID &p_vehicle, const Ref<Material> &p_overlay) override;
            /* The wiper arms the MMD names (animwiperprefix), at rest, swept by the FIZ's wiper angle */
            PackedVector4Array _vehicle_get_wiper_sweeps(const RID &p_vehicle) const override;
            void _frame_processed(double p_delta) override;

        public:
            static MaszynaE3DRailVehicleRenderingServer *get_instance();

            MaszynaE3DRailVehicleRenderingServer();
            ~MaszynaE3DRailVehicleRenderingServer() override;

            /* The cargo, drawn at the floor of the vehicle (DynObj.cpp:866) while its models are -
             * none for an empty file name */
            void
            vehicle_set_load_model(const RID &p_vehicle, const String &p_data_path, const String &p_model_filename);
            /* The level (0..1) of the light of a VehicleServer cabin that its low-poly cab - of
             * the cabin's kind (RailVehicleServer) - is lit at (TDynamicObject::set_cab_lights(),
             * DynObj.cpp:841-853); with jointcabs: every cab at the brightest */
            void cabin_set_light_level(const RID &p_cabin, double p_level);
    };
} // namespace godot
