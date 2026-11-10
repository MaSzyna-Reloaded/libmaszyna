#pragma once
#include "maszyna/e3d/MaszynaE3DModel.hpp"
#include "vehicles/rail/RailVehicleEnginePowerSource.hpp"
#include "vehicles/rail/RailVehiclePantograph.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
    /* A pantograph of a MaSzyna vehicle as TAnimPant builds it (DynObj.cpp:5404-5480, 5577-5633), out of
     * the data the original gives it: the arms' dimensions of the type the FIZ names (PantType=,
     * DynObj.cpp:90-194) or - with no type named, every FIZ of the game data - measured from the model:
     * the lower arm's place, the arms' lengths and rest angles from the lower arm to the upper one and
     * the slider, the slider's height over its pivot and its place along the vehicle. A slider the
     * model cannot be measured by takes its place and height from the MMD's pantfactors:, and with it
     * a pantograph with no lower arm stands on top of the vehicle's box. Read while the vehicle's data
     * is, from the model's description alone (MaszynaE3DModel without its geometry) and the slider's
     * own vertices (MaszynaE3DModelReader::read_submodel_positions()). A pure helper. */
    class MaszynaPantographGeometryReader : public Object {
            GDCLASS(MaszynaPantographGeometryReader, Object)

        protected:
            static void _bind_methods();

        public:
            /* Pantograph elements, as the MMD names them: lower arm, its pair, upper arm, its pair,
             * slider (DynObj.cpp:5414) */
            static constexpr int ELEMENTS = 5;
            static constexpr int LOWER_ARM = 0;
            static constexpr int UPPER_ARM = 2;
            static constexpr int SLIDER = 4;

            /* The pantograph p_index of the vehicle: p_model the description of the model file at
             * p_model_path, p_arms its ELEMENTS submodel names (none - no slider - leaves it without
             * geometry), p_model_transform where the model sits in the vehicle, p_type the FIZ's
             * PantType=, p_factors the MMD's pantfactors:, p_vehicle_height the FIZ's height of the
             * vehicle [m] and p_control the channel it is raised by */
            static Ref<RailVehiclePantograph>
            read(const Ref<MaszynaE3DModel> &p_model, const String &p_model_path, const PackedStringArray &p_arms,
                 const Transform3D &p_model_transform, RailVehicleEnginePowerSource::PantographType p_type,
                 const PackedFloat64Array &p_factors, int p_index, double p_vehicle_height,
                 RailVehicleEnginePowerSource::PantographSelector p_control);
    };
} // namespace godot
