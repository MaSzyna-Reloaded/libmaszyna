#include "MaszynaLegacyEventCondition.hpp"
#include "macros.hpp"
#include "tracks/TrackServer.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MaszynaLegacyEventCondition::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_probability", "probability"), &MaszynaLegacyEventCondition::set_probability);
        ClassDB::bind_method(D_METHOD("get_probability"), &MaszynaLegacyEventCondition::get_probability);
        ClassDB::bind_method(D_METHOD("set_memories", "memories"), &MaszynaLegacyEventCondition::set_memories);
        ClassDB::bind_method(D_METHOD("get_memories"), &MaszynaLegacyEventCondition::get_memories);
        ClassDB::bind_method(D_METHOD("set_text", "text"), &MaszynaLegacyEventCondition::set_text);
        ClassDB::bind_method(D_METHOD("get_text"), &MaszynaLegacyEventCondition::get_text);
        ClassDB::bind_method(D_METHOD("set_value1", "value1"), &MaszynaLegacyEventCondition::set_value1);
        ClassDB::bind_method(D_METHOD("get_value1"), &MaszynaLegacyEventCondition::get_value1);
        ClassDB::bind_method(D_METHOD("set_value2", "value2"), &MaszynaLegacyEventCondition::set_value2);
        ClassDB::bind_method(D_METHOD("get_value2"), &MaszynaLegacyEventCondition::get_value2);
        ClassDB::bind_method(D_METHOD("set_mask", "mask"), &MaszynaLegacyEventCondition::set_mask);
        ClassDB::bind_method(D_METHOD("get_mask"), &MaszynaLegacyEventCondition::get_mask);

        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "probability", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_probability",
                "get_probability");
        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "memories", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_memories",
                "get_memories");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value1"), "set_value1", "get_value1");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value2"), "set_value2", "get_value2");
        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "mask", PROPERTY_HINT_FLAGS, "Text,Value 1,Value 2"), "set_mask",
                "get_mask");

        ClassDB::bind_method(
                D_METHOD("set_text_operator", "operator"), &MaszynaLegacyEventCondition::set_text_operator);
        ClassDB::bind_method(D_METHOD("get_text_operator"), &MaszynaLegacyEventCondition::get_text_operator);
        ClassDB::bind_method(
                D_METHOD("set_value1_operator", "operator"), &MaszynaLegacyEventCondition::set_value1_operator);
        ClassDB::bind_method(D_METHOD("get_value1_operator"), &MaszynaLegacyEventCondition::get_value1_operator);
        ClassDB::bind_method(
                D_METHOD("set_value2_operator", "operator"), &MaszynaLegacyEventCondition::set_value2_operator);
        ClassDB::bind_method(D_METHOD("get_value2_operator"), &MaszynaLegacyEventCondition::get_value2_operator);
        ClassDB::bind_method(D_METHOD("set_pass", "pass"), &MaszynaLegacyEventCondition::set_pass);
        ClassDB::bind_method(D_METHOD("get_pass"), &MaszynaLegacyEventCondition::get_pass);
        ClassDB::bind_method(D_METHOD("set_tracks", "tracks"), &MaszynaLegacyEventCondition::set_tracks);
        ClassDB::bind_method(D_METHOD("get_tracks"), &MaszynaLegacyEventCondition::get_tracks);
        ClassDB::bind_method(D_METHOD("set_track_test", "track_test"), &MaszynaLegacyEventCondition::set_track_test);
        ClassDB::bind_method(D_METHOD("get_track_test"), &MaszynaLegacyEventCondition::get_track_test);

        const String operator_hint = enum_hint(
                {{"==", OPERATOR_EQUAL},
                 {"!=", OPERATOR_NOT_EQUAL},
                 {"<", OPERATOR_LESS},
                 {">", OPERATOR_GREATER},
                 {"<=", OPERATOR_LESS_EQUAL},
                 {">=", OPERATOR_GREATER_EQUAL}});
        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "text_operator", PROPERTY_HINT_ENUM, operator_hint), "set_text_operator",
                "get_text_operator");
        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "value1_operator", PROPERTY_HINT_ENUM, operator_hint), "set_value1_operator",
                "get_value1_operator");
        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "value2_operator", PROPERTY_HINT_ENUM, operator_hint), "set_value2_operator",
                "get_value2_operator");
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "pass", PROPERTY_HINT_ENUM,
                        enum_hint({{"All", PASS_ALL}, {"Any", PASS_ANY}, {"None", PASS_NONE}})),
                "set_pass", "get_pass");
        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "tracks", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_tracks", "get_tracks");
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "track_test", PROPERTY_HINT_ENUM,
                        enum_hint(
                                {{"None", TRACK_TEST_NONE},
                                 {"Occupied", TRACK_TEST_OCCUPIED},
                                 {"Free", TRACK_TEST_FREE}})),
                "set_track_test", "get_track_test");

        BIND_ENUM_CONSTANT(OPERATOR_EQUAL);
        BIND_ENUM_CONSTANT(OPERATOR_NOT_EQUAL);
        BIND_ENUM_CONSTANT(OPERATOR_LESS);
        BIND_ENUM_CONSTANT(OPERATOR_GREATER);
        BIND_ENUM_CONSTANT(OPERATOR_LESS_EQUAL);
        BIND_ENUM_CONSTANT(OPERATOR_GREATER_EQUAL);
        BIND_ENUM_CONSTANT(PASS_ALL);
        BIND_ENUM_CONSTANT(PASS_ANY);
        BIND_ENUM_CONSTANT(PASS_NONE);
        BIND_ENUM_CONSTANT(TRACK_TEST_NONE);
        BIND_ENUM_CONSTANT(TRACK_TEST_OCCUPIED);
        BIND_ENUM_CONSTANT(TRACK_TEST_FREE);
    }

    /// compare() (comparison.h:27-38)
    template<typename T>
    bool MaszynaLegacyEventCondition::_compare(const T &p_left, const T &p_right, const Operator p_operator) {
        switch (p_operator) {
            case OPERATOR_EQUAL:
                return p_left == p_right;
            case OPERATOR_NOT_EQUAL:
                return !(p_left == p_right);
            case OPERATOR_LESS:
                return p_left < p_right;
            case OPERATOR_GREATER:
                return p_right < p_left;
            case OPERATOR_LESS_EQUAL:
                return !(p_right < p_left);
            case OPERATOR_GREATER_EQUAL:
                return !(p_left < p_right);
        }
        return false;
    }

    /// event_conditions::test() (Event.cpp:62-176): the probability first, then the tracks, then
    /// the memories
    bool MaszynaLegacyEventCondition::test(const RID &p_event, const RID &p_activator) const {
        if (UtilityFunctions::randf() > probability) {
            return false;
        }
        if (!(track_test == TRACK_TEST_NONE)) {
            const TrackServer *track_server = TrackServer::get_instance();
            ERR_FAIL_NULL_V(track_server, false);
            for (int i = 0; i < tracks.size(); i++) {
                if (!(track_server->track_is_occupied(tracks[i]) == (track_test == TRACK_TEST_OCCUPIED))) {
                    return false;
                }
            }
        }
        const ScenarioEventServer *server = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL_V(server, false);
        const int wildcard = static_cast<int>(text.find(TEXT_WILDCARD));
        for (int i = 0; i < memories.size(); i++) {
            const RID memory = memories[i];
            bool passed = false;
            bool failed = false;
            if (mask.has_flag(ScenarioEventServer::MEMORY_FIELD_TEXT)) {
                const String memory_text = server->memory_get_text(memory);
                const bool result =
                        wildcard < 0
                                ? _compare(memory_text, text, text_operator)
                                : _compare(memory_text.substr(0, wildcard), text.substr(0, wildcard), text_operator);
                passed = passed || result;
                failed = failed || !result;
            }
            if (mask.has_flag(ScenarioEventServer::MEMORY_FIELD_VALUE1)) {
                const bool result = _compare(server->memory_get_value1(memory), value1, value1_operator);
                passed = passed || result;
                failed = failed || !result;
            }
            if (mask.has_flag(ScenarioEventServer::MEMORY_FIELD_VALUE2)) {
                const bool result = _compare(server->memory_get_value2(memory), value2, value2_operator);
                passed = passed || result;
                failed = failed || !result;
            }
            bool memory_passed = false;
            if (pass == PASS_ALL) {
                memory_passed = !failed;
            } else if (pass == PASS_ANY) {
                memory_passed = passed;
            } else {
                memory_passed = !passed;
            }
            if (!memory_passed) {
                return false;
            }
        }
        return true;
    }

    void MaszynaLegacyEventCondition::set_probability(const double p_probability) {
        probability = p_probability;
    }

    double MaszynaLegacyEventCondition::get_probability() const {
        return probability;
    }

    void MaszynaLegacyEventCondition::set_memories(const TypedArray<RID> &p_memories) {
        memories = p_memories;
    }

    TypedArray<RID> MaszynaLegacyEventCondition::get_memories() const {
        return memories;
    }

    void MaszynaLegacyEventCondition::set_text(const String &p_text) {
        text = p_text;
    }

    String MaszynaLegacyEventCondition::get_text() const {
        return text;
    }

    void MaszynaLegacyEventCondition::set_value1(const double p_value1) {
        value1 = p_value1;
    }

    double MaszynaLegacyEventCondition::get_value1() const {
        return value1;
    }

    void MaszynaLegacyEventCondition::set_value2(const double p_value2) {
        value2 = p_value2;
    }

    double MaszynaLegacyEventCondition::get_value2() const {
        return value2;
    }

    void MaszynaLegacyEventCondition::set_mask(const BitField<ScenarioEventServer::MemoryField> p_mask) {
        mask = p_mask;
    }

    BitField<ScenarioEventServer::MemoryField> MaszynaLegacyEventCondition::get_mask() const {
        return mask;
    }

    void MaszynaLegacyEventCondition::set_text_operator(const Operator p_operator) {
        text_operator = p_operator;
    }

    MaszynaLegacyEventCondition::Operator MaszynaLegacyEventCondition::get_text_operator() const {
        return text_operator;
    }

    void MaszynaLegacyEventCondition::set_value1_operator(const Operator p_operator) {
        value1_operator = p_operator;
    }

    MaszynaLegacyEventCondition::Operator MaszynaLegacyEventCondition::get_value1_operator() const {
        return value1_operator;
    }

    void MaszynaLegacyEventCondition::set_value2_operator(const Operator p_operator) {
        value2_operator = p_operator;
    }

    MaszynaLegacyEventCondition::Operator MaszynaLegacyEventCondition::get_value2_operator() const {
        return value2_operator;
    }

    void MaszynaLegacyEventCondition::set_pass(const Pass p_pass) {
        pass = p_pass;
    }

    MaszynaLegacyEventCondition::Pass MaszynaLegacyEventCondition::get_pass() const {
        return pass;
    }

    void MaszynaLegacyEventCondition::set_tracks(const TypedArray<RID> &p_tracks) {
        tracks = p_tracks;
    }

    TypedArray<RID> MaszynaLegacyEventCondition::get_tracks() const {
        return tracks;
    }

    void MaszynaLegacyEventCondition::set_track_test(const TrackTest p_track_test) {
        track_test = p_track_test;
    }

    MaszynaLegacyEventCondition::TrackTest MaszynaLegacyEventCondition::get_track_test() const {
        return track_test;
    }
} // namespace godot
