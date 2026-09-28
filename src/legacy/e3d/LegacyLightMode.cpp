#include "LegacyLightMode.hpp"
#include <godot_cpp/core/math.hpp>

namespace godot {
    LegacyLightMode LegacyLightMode::parse(const float p_value) {
        LegacyLightMode result;
        const float value = Math::abs(p_value);
        const float integral = Math::floor(value);
        const float fraction = value - integral;
        result.mode = static_cast<Mode>(static_cast<int>(integral));
        if (result.mode == MODE_DARK || result.mode == MODE_HOME) {
            result.threshold = fraction < FRACTION_EPSILON ? 0.0f : fraction;
            return result;
        }
        if (result.mode > MODE_HOME) {
            result.mode = MODE_OFF; // ls_winter and above are not handled by RaPrepare()
            return result;
        }
        result.phase = p_value > 0.0f ? 0.0f : NEGATIVE_PHASE;
        if (fraction < FRACTION_EPSILON) {
            return result; // plain off/on, or ls_Blink with the default times
        }
        switch (result.mode) {
            case MODE_OFF:
                result.on_time = fraction * FRACTION_TIME_SCALE;
                result.off_time = fraction * FRACTION_TIME_SCALE;
                break;
            case MODE_ON:
                result.on_time = fraction * (DEFAULT_ON_TIME + DEFAULT_OFF_TIME);
                result.off_time = (1.0f - fraction) * (DEFAULT_ON_TIME + DEFAULT_OFF_TIME);
                break;
            default:
                result.on_time = FRACTION_TIME_SCALE / fraction;
                result.off_time = FRACTION_TIME_SCALE / fraction;
                break;
        }
        result.mode = MODE_BLINK;
        return result;
    }
} // namespace godot
