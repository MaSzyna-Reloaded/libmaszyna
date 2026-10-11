#include "MaszynaLegacyMemoryAction.hpp"
#include "driver/DriverServer.hpp"
#include "macros.hpp"
#include "tracks/TrackServer.hpp"

namespace godot {
    void MaszynaLegacyMemoryAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_memories", "memories"), &MaszynaLegacyMemoryAction::set_memories);
        ClassDB::bind_method(D_METHOD("get_memories"), &MaszynaLegacyMemoryAction::get_memories);
        ClassDB::bind_method(D_METHOD("set_source", "source"), &MaszynaLegacyMemoryAction::set_source);
        ClassDB::bind_method(D_METHOD("get_source"), &MaszynaLegacyMemoryAction::get_source);
        ClassDB::bind_method(D_METHOD("set_text", "text"), &MaszynaLegacyMemoryAction::set_text);
        ClassDB::bind_method(D_METHOD("get_text"), &MaszynaLegacyMemoryAction::get_text);
        ClassDB::bind_method(D_METHOD("set_value1", "value1"), &MaszynaLegacyMemoryAction::set_value1);
        ClassDB::bind_method(D_METHOD("get_value1"), &MaszynaLegacyMemoryAction::get_value1);
        ClassDB::bind_method(D_METHOD("set_value2", "value2"), &MaszynaLegacyMemoryAction::set_value2);
        ClassDB::bind_method(D_METHOD("get_value2"), &MaszynaLegacyMemoryAction::get_value2);
        ClassDB::bind_method(D_METHOD("set_mask", "mask"), &MaszynaLegacyMemoryAction::set_mask);
        ClassDB::bind_method(D_METHOD("get_mask"), &MaszynaLegacyMemoryAction::get_mask);
        ClassDB::bind_method(D_METHOD("set_mode", "mode"), &MaszynaLegacyMemoryAction::set_mode);
        ClassDB::bind_method(D_METHOD("get_mode"), &MaszynaLegacyMemoryAction::get_mode);

        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "memories", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_memories",
                "get_memories");
        ADD_PROPERTY(PropertyInfo(Variant::RID, "source"), "set_source", "get_source");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value1"), "set_value1", "get_value1");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value2"), "set_value2", "get_value2");
        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "mask", PROPERTY_HINT_FLAGS, "Text,Value 1,Value 2"), "set_mask",
                "get_mask");
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "mode", PROPERTY_HINT_ENUM,
                        enum_hint(
                                {{"Set", MODE_SET},
                                 {"Add", MODE_ADD},
                                 {"Isolated busy", MODE_ISOLATED_BUSY},
                                 {"Isolated free", MODE_ISOLATED_FREE}})),
                "set_mode", "get_mode");

        BIND_ENUM_CONSTANT(MODE_SET);
        BIND_ENUM_CONSTANT(MODE_ADD);
        BIND_ENUM_CONSTANT(MODE_ISOLATED_BUSY);
        BIND_ENUM_CONSTANT(MODE_ISOLATED_FREE);
    }

    /// TMemCell::UpdateValues() (MemCell.cpp:28-50) on every memory
    void MaszynaLegacyMemoryAction::run(const RID &p_event, const RID &p_activator) {
        ScenarioEventServer *server = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(server);
        const bool copy = source.is_valid();
        const String new_text = copy ? server->memory_get_text(source) : text;
        const double new_value1 = copy ? server->memory_get_value1(source) : value1;
        const double new_value2 = copy ? server->memory_get_value2(source) : value2;
        const bool add = mode == MODE_ADD;
        for (int i = 0; i < memories.size(); i++) {
            const RID memory = memories[i];
            String memory_text = server->memory_get_text(memory);
            double memory_value1 = server->memory_get_value1(memory);
            double memory_value2 = server->memory_get_value2(memory);
            if (mask.has_flag(ScenarioEventServer::MEMORY_FIELD_TEXT)) {
                memory_text = add ? memory_text + new_text : new_text;
            }
            if (mask.has_flag(ScenarioEventServer::MEMORY_FIELD_VALUE1)) {
                memory_value1 = add ? memory_value1 + new_value1 : new_value1;
            }
            if (mask.has_flag(ScenarioEventServer::MEMORY_FIELD_VALUE2)) {
                switch (mode) {
                    case MODE_SET:
                        memory_value2 = new_value2;
                        break;
                    case MODE_ADD:
                        memory_value2 += new_value2;
                        break;
                    case MODE_ISOLATED_BUSY:
                        memory_value2 = static_cast<int>(memory_value2) | 1;
                        break;
                    case MODE_ISOLATED_FREE:
                        memory_value2 = static_cast<int>(memory_value2) & ~ISOLATED_FREE_MASK;
                        break;
                }
            }
            server->memory_set_values(memory, memory_text, memory_value1, memory_value2);
            // updatevalues/addvalues also give the memory's command to the driver of every vehicle
            // on its track (Event.cpp:538-547); copyvalues and a section's own memory do not
            if (copy || mode == MODE_ISOLATED_BUSY || mode == MODE_ISOLATED_FREE) {
                continue;
            }
            const RID track = server->memory_get_track(memory);
            TrackServer *tracks = TrackServer::get_instance();
            DriverServer *drivers = DriverServer::get_instance();
            if (!track.is_valid() || tracks == nullptr || drivers == nullptr) {
                continue;
            }
            const TypedArray<RID> vehicles = tracks->track_get_vehicles(track);
            for (int v = 0; v < vehicles.size(); v++) {
                const RID driver = drivers->vehicle_get_driver(vehicles[v]);
                if (driver.is_valid()) {
                    drivers->driver_send_command(
                            driver, memory_text, memory_value1, memory_value2, server->memory_get_position(memory));
                }
            }
        }
    }

    void MaszynaLegacyMemoryAction::set_memories(const TypedArray<RID> &p_memories) {
        memories = p_memories;
    }

    TypedArray<RID> MaszynaLegacyMemoryAction::get_memories() const {
        return memories;
    }

    void MaszynaLegacyMemoryAction::set_source(const RID &p_source) {
        source = p_source;
    }

    RID MaszynaLegacyMemoryAction::get_source() const {
        return source;
    }

    void MaszynaLegacyMemoryAction::set_text(const String &p_text) {
        text = p_text;
    }

    String MaszynaLegacyMemoryAction::get_text() const {
        return text;
    }

    void MaszynaLegacyMemoryAction::set_value1(const double p_value1) {
        value1 = p_value1;
    }

    double MaszynaLegacyMemoryAction::get_value1() const {
        return value1;
    }

    void MaszynaLegacyMemoryAction::set_value2(const double p_value2) {
        value2 = p_value2;
    }

    double MaszynaLegacyMemoryAction::get_value2() const {
        return value2;
    }

    void MaszynaLegacyMemoryAction::set_mask(const BitField<ScenarioEventServer::MemoryField> p_mask) {
        mask = p_mask;
    }

    BitField<ScenarioEventServer::MemoryField> MaszynaLegacyMemoryAction::get_mask() const {
        return mask;
    }

    void MaszynaLegacyMemoryAction::set_mode(const Mode p_mode) {
        mode = p_mode;
    }

    MaszynaLegacyMemoryAction::Mode MaszynaLegacyMemoryAction::get_mode() const {
        return mode;
    }
} // namespace godot
