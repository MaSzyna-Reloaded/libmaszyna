#include "MaszynaPantographGeometryReader.hpp"
#include "maszyna/e3d/MaszynaE3DModelReader.hpp"

#include <godot_cpp/core/math.hpp>

namespace godot {
    namespace {
        /* TAnimPant's dimensions of a pantograph type: lower and upper arm, the slider's horizontal
         * offset and its height over its pivot - AKP_4E, and the DSAx, EC160_200 and WBL85 alike
         * (DynObj.cpp:90-160) */
        struct PantographDimensions {
                double lower_length;
                double upper_length;
                double horizontal;
                double slider_height;
        };
        constexpr PantographDimensions PANTOGRAPH_AKP_4E{1.22, 1.755, 0.535, 0.07};
        constexpr PantographDimensions PANTOGRAPH_DSA{1.98374, 2.14199, 0.142, 0.09353};
        /* common_pantograph_settings(): the lower arm's angle lowered (DynObj.cpp:74) */
        constexpr double LOWER_REST_ANGLE_DEGREES = 2.8547285515689267247882521833308;
        /* The slider's scale the model's own height is trusted at (DynObj.cpp:5459) */
        constexpr double SLIDER_SCALE_TOLERANCE = 0.001;
        /* pantfactors: - two places along, then two slider heights (DynObj.cpp:5580-5588) */
        constexpr int FACTOR_COUNT = 4;
        constexpr int FACTOR_HEIGHTS = 2;
        constexpr int FACTOR_PAIR = 2;

        struct Part {
                bool found = false;
                Transform3D rest;
        };
    } // namespace

    /* The submodel of the model by its lowered name, and where it rests in the model's frame - the
     * chain of its parents' transforms; null when the model has none of that name */
    static Ref<MaszynaE3DSubModel> find_submodel(
            const TypedArray<MaszynaE3DSubModel> &p_submodels, const String &p_name, const Transform3D &p_parent,
            Transform3D &p_r_transform) {
        for (int64_t index = 0; index < p_submodels.size(); ++index) {
            Ref<MaszynaE3DSubModel> submodel = p_submodels[index];
            if (submodel.is_null()) {
                continue;
            }
            const Transform3D transform = p_parent * submodel->get_transform();
            if (submodel->get_name().to_lower() == p_name) {
                p_r_transform = transform;
                return submodel;
            }
            if (Ref<MaszynaE3DSubModel> found =
                        find_submodel(submodel->get_submodels(), p_name, transform, p_r_transform);
                found.is_valid()) {
                return found;
            }
        }
        return {};
    }

    void MaszynaPantographGeometryReader::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaPantographGeometryReader",
                D_METHOD(
                        "read", "model", "model_path", "arms", "model_transform", "type", "factors", "index",
                        "vehicle_height", "control"),
                &MaszynaPantographGeometryReader::read);
    }

    Ref<RailVehiclePantograph> MaszynaPantographGeometryReader::read(
            const Ref<MaszynaE3DModel> &p_model, const String &p_model_path, const PackedStringArray &p_arms,
            const Transform3D &p_model_transform, const RailVehicleEnginePowerSource::PantographType p_type,
            const PackedFloat64Array &p_factors, const int p_index, const double p_vehicle_height,
            const RailVehicleEnginePowerSource::PantographSelector p_control) {
        Ref<RailVehiclePantograph> pantograph;
        pantograph.instantiate();
        pantograph->set_control(p_control);
        if (p_model.is_null() || p_arms.size() != ELEMENTS) {
            return pantograph;
        }
        // each arm where it rests in the vehicle's own frame, and the slider's own submodel
        Part arms[ELEMENTS];
        Ref<MaszynaE3DSubModel> slider_model;
        for (int element = 0; element < ELEMENTS; ++element) {
            Transform3D rest;
            const String name = p_arms[element].to_lower();
            const Ref<MaszynaE3DSubModel> submodel =
                    name.is_empty() ? Ref<MaszynaE3DSubModel>()
                                    : find_submodel(p_model->get_submodels(), name, Transform3D(), rest);
            if (submodel.is_valid()) {
                arms[element] = Part{true, p_model_transform * rest};
            }
            if (element == SLIDER) {
                slider_model = submodel;
            }
        }
        if (slider_model.is_null()) {
            return pantograph;
        }
        const PantographDimensions &dimensions =
                p_type == RailVehicleEnginePowerSource::PANTOGRAPH_TYPE_NONE ||
                                p_type == RailVehicleEnginePowerSource::PANTOGRAPH_TYPE_AKP_4E
                        ? PANTOGRAPH_AKP_4E
                        : PANTOGRAPH_DSA;
        double lower_length = dimensions.lower_length;
        double upper_length = dimensions.upper_length;
        double horizontal = dimensions.horizontal;
        double slider_height = dimensions.slider_height;
        double lower_rest_angle = Math::deg_to_rad(LOWER_REST_ANGLE_DEGREES);
        double upper_rest_angle =
                Math::acos(((lower_length * Math::cos(lower_rest_angle)) + horizontal) / upper_length);
        // the FIZ's type comes first; the model is measured only for what it does not give
        const bool measured = p_type == RailVehicleEnginePowerSource::PANTOGRAPH_TYPE_NONE;
        const Part &lower = arms[LOWER_ARM];
        const Part &upper = arms[UPPER_ARM];
        const Part &slider = arms[SLIDER];
        Vector3 position;
        if (!lower.found) {
            slider_height = 0.0;
        } else {
            position.x = lower.rest.origin.x;
            position.y = lower.rest.origin.y;
            if (measured && upper.found) {
                const Vector3 lower_to_upper = lower.rest.basis.inverse().xform(upper.rest.origin - lower.rest.origin);
                const Vector3 upper_to_slider =
                        upper.rest.basis.inverse().xform(slider.rest.origin - upper.rest.origin);
                lower_length = Vector2(lower_to_upper.y, lower_to_upper.z).length();
                upper_length = Vector2(upper_to_slider.y, upper_to_slider.z).length();
                horizontal = Math::abs(upper_to_slider.y) - Math::abs(lower_to_upper.y);
                lower_rest_angle = Math::atan2(Math::abs(lower_to_upper.z), Math::abs(lower_to_upper.y));
                upper_rest_angle = Math::atan2(Math::abs(upper_to_slider.z), Math::abs(upper_to_slider.y));
                // the slider's own top over its pivot and its place along, while its scale holds
                // (DynObj.cpp:5543-5549): its vertices' highest, from 0, as the vehicle carries it
                // (TSubModel::MaxY(), Model3d.cpp:1481-1497); a slider with no triangles has none
                if (Math::abs(slider.rest.basis.determinant() - 1.0) < SLIDER_SCALE_TOLERANCE) {
                    double highest = 0.0;
                    if (slider_model->get_geometry_index() >= 0 && !p_model_path.is_empty()) {
                        const PackedVector3Array vertices = MaszynaE3DModelReader::read_submodel_positions(
                                p_model_path, slider_model->get_geometry_index());
                        for (const Vector3 &vertex: vertices) {
                            highest = MAX(highest, static_cast<double>(slider.rest.xform(vertex).y));
                        }
                        slider_height = highest - slider.rest.origin.y;
                    } else {
                        slider_height = 0.0;
                    }
                    position.z = slider.rest.origin.z;
                } else {
                    slider_height = 0.0;
                }
            }
        }
        if (p_factors.size() == FACTOR_COUNT) {
            // the front and the rear factors, every pantograph after them by its channel
            const int factor = p_index % FACTOR_PAIR;
            if (slider_height == 0.0) {
                // the MMD's place along is the model's own, the vehicle frame turns it (DynObj.cpp:5605-5614)
                position.z = p_model_transform.basis.xform(Vector3(0.0, 0.0, static_cast<real_t>(p_factors[factor]))).z;
                slider_height = p_factors[FACTOR_HEIGHTS + factor];
            }
            if (position.y == 0.0) {
                const double raised = (lower_length * Math::sin(lower_rest_angle)) +
                                      (upper_length * Math::sin(upper_rest_angle)) + slider_height;
                position.y = static_cast<real_t>(p_vehicle_height - slider_height - raised);
            }
        }
        pantograph->set_has_geometry(true);
        pantograph->set_position(position);
        pantograph->set_lower_length(lower_length);
        pantograph->set_upper_length(upper_length);
        pantograph->set_horizontal(horizontal);
        pantograph->set_lower_rest_angle(lower_rest_angle);
        pantograph->set_upper_rest_angle(upper_rest_angle);
        pantograph->set_slider_height(slider_height);
        return pantograph;
    }
} // namespace godot
