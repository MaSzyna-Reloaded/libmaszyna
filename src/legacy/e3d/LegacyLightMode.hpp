#pragma once

namespace godot {
    /// A value of a scenery `lights` list or of a `lights` event (TAnimModel::lsLights[]): the
    /// integer part is TLightState (AnimModel.h:27-34), the fraction and the sign modify it.
    /// Parsed once into what it means, so nothing past the parser deals with the encoding.
    struct LegacyLightMode {
            enum Mode {
                MODE_OFF = 0,
                MODE_ON = 1,
                MODE_BLINK = 2,
                MODE_DARK = 3,
                MODE_HOME = 4,
            };

            /// fOnTime/fOffTime (AnimModel.h:208-209)
            static constexpr float DEFAULT_ON_TIME = 0.5;
            static constexpr float DEFAULT_OFF_TIME = 0.5;
            /// Scale of the fraction of ls_Off and ls_Blink (AnimModel.cpp:522-530)
            static constexpr float FRACTION_TIME_SCALE = 0.5;
            /// A fraction below this is no fraction (AnimModel.cpp:521)
            static constexpr float FRACTION_EPSILON = 0.01;
            /// A negative value shifts the blinking by this many seconds (AnimModel.cpp:538)
            static constexpr float NEGATIVE_PHASE = 0.5;

            Mode mode = MODE_OFF;
            /// MODE_DARK/MODE_HOME: the light level at or below which the light is on; 0 when the
            /// value carries no fraction, which means the default threshold
            float threshold = 0.0;
            /// MODE_BLINK: seconds on, seconds off, and the shift of the cycle
            float on_time = DEFAULT_ON_TIME;
            float off_time = DEFAULT_OFF_TIME;
            float phase = 0.0;

            /// TAnimModel::RaAnimate(), AnimModel.cpp:506-540: ls_Off with a fraction blinks fast
            /// (0.1 - 0.1 s period), ls_On with a fraction is a duty cycle (1.25 - on for a
            /// quarter of 1 s), ls_Blink with a fraction blinks slowly (2.2 - 5 s period)
            static LegacyLightMode parse(float p_value);
    };
} // namespace godot
