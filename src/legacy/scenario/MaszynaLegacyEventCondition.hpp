#pragma once
#include "scenario/ScenarioEventCondition.hpp"
#include "scenario/ScenarioEventServer.hpp"
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's event `condition` (event_conditions, Event.cpp:44-254) and a launcher's
    /// memcell condition (EvLaunch.cpp:100-130, 216-227): `probability`, `trackoccupied`,
    /// `trackfree`, `memcompare` and `memcompareex`. Every memory has to pass; a memory passes as
    /// its pass mode combines the masked fields compared (TMemCell::Compare(), MemCell.cpp:130-185),
    /// a text with a `*` compared up to it. `memcompare` is all fields `==`.
    class MaszynaLegacyEventCondition : public ScenarioEventCondition {
            GDCLASS(MaszynaLegacyEventCondition, ScenarioEventCondition)

        public:
            /// The part of a `memcompare` text that is compared is the part before it
            static constexpr const char *TEXT_WILDCARD = "*";

            /// comparison_operator (comparison.h:12-19)
            enum Operator {
                OPERATOR_EQUAL,
                OPERATOR_NOT_EQUAL,
                OPERATOR_LESS,
                OPERATOR_GREATER,
                OPERATOR_LESS_EQUAL,
                OPERATOR_GREATER_EQUAL,
            };

            /// How the compared fields of one memory combine (comparison_pass, comparison.h:21-25)
            enum Pass {
                PASS_ALL,
                PASS_ANY,
                PASS_NONE,
            };

            /// What the tracks have to be (flags::track_busy, flags::track_free)
            enum TrackTest {
                TRACK_TEST_NONE,
                TRACK_TEST_OCCUPIED,
                TRACK_TEST_FREE,
            };

        private:
            double probability = 1.0;
            TypedArray<RID> memories;
            String text;
            double value1 = 0.0;
            double value2 = 0.0;
            BitField<ScenarioEventServer::MemoryField> mask = 0;
            Operator text_operator = OPERATOR_EQUAL;
            Operator value1_operator = OPERATOR_EQUAL;
            Operator value2_operator = OPERATOR_EQUAL;
            Pass pass = PASS_ALL;
            TypedArray<RID> tracks;
            TrackTest track_test = TRACK_TEST_NONE;

            template<typename T>
            static bool _compare(const T &p_left, const T &p_right, Operator p_operator);

        protected:
            static void _bind_methods();

            bool test(const RID &p_event, const RID &p_activator) const override;

        public:
            /// Passes this often, 1 for always (Event.cpp:68-78)
            void set_probability(double p_probability);
            double get_probability() const;
            void set_memories(const TypedArray<RID> &p_memories);
            TypedArray<RID> get_memories() const;
            void set_text(const String &p_text);
            String get_text() const;
            void set_value1(double p_value1);
            double get_value1() const;
            void set_value2(double p_value2);
            double get_value2() const;
            void set_mask(BitField<ScenarioEventServer::MemoryField> p_mask);
            BitField<ScenarioEventServer::MemoryField> get_mask() const;
            void set_text_operator(Operator p_operator);
            Operator get_text_operator() const;
            void set_value1_operator(Operator p_operator);
            Operator get_value1_operator() const;
            void set_value2_operator(Operator p_operator);
            Operator get_value2_operator() const;
            void set_pass(Pass p_pass);
            Pass get_pass() const;
            /// TrackServer tracks every one of which has to be as track_test says
            void set_tracks(const TypedArray<RID> &p_tracks);
            TypedArray<RID> get_tracks() const;
            void set_track_test(TrackTest p_track_test);
            TrackTest get_track_test() const;
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaLegacyEventCondition::Operator)
VARIANT_ENUM_CAST(MaszynaLegacyEventCondition::Pass)
VARIANT_ENUM_CAST(MaszynaLegacyEventCondition::TrackTest)
