#include "MaszynaMoverElectricEngineConfiguration.hpp"

namespace godot {
    void MaszynaMoverElectricEngineConfiguration::_bind_methods() {
        BIND_PROPERTY(MaszynaMoverElectricEngineConfiguration, Variant::FLOAT, circuit_resistance, "circuit");
        BIND_PROPERTY(MaszynaMoverElectricEngineConfiguration, Variant::FLOAT, circuit_tuhex_sum, "circuit/tuhex");
        BIND_PROPERTY(MaszynaMoverElectricEngineConfiguration, Variant::FLOAT, circuit_tuhex_diff, "circuit/tuhex");
        BIND_PROPERTY(
                MaszynaMoverElectricEngineConfiguration, Variant::FLOAT, circuit_tuhex_min_current, "circuit/tuhex");
        BIND_PROPERTY(
                MaszynaMoverElectricEngineConfiguration, Variant::FLOAT, circuit_tuhex_max_current, "circuit/tuhex");
        BIND_PROPERTY(MaszynaMoverElectricEngineConfiguration, Variant::INT, circuit_tuhex_stages, "circuit/tuhex");
        BIND_PROPERTY(MaszynaMoverElectricEngineConfiguration, Variant::FLOAT, circuit_tuhex_sum_1, "circuit/tuhex");
        BIND_PROPERTY(MaszynaMoverElectricEngineConfiguration, Variant::FLOAT, circuit_tuhex_sum_2, "circuit/tuhex");
        BIND_PROPERTY(MaszynaMoverElectricEngineConfiguration, Variant::FLOAT, circuit_tuhex_sum_3, "circuit/tuhex");
        BIND_PROPERTY(
                MaszynaMoverElectricEngineConfiguration, Variant::BOOL,
                cntrl_converter_overload_relay_off_when_main_is_off, "cntrl");
    }
} // namespace godot
