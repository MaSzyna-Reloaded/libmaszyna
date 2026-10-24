#include "MaszynaMemoryAction.hpp"
#include "driver/DriverServer.hpp"
#include "macros.hpp"
#include "tracks/TrackServer.hpp"

namespace godot {
    void MaszynaMemoryAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_memories", "memories"), &MaszynaMemoryAction::set_memories);
        ClassDB::bind_method(D_METHOD("get_memories"), &MaszynaMemoryAction::get_memories);
        ClassDB::bind_method(D_METHOD("set_source", "source"), &MaszynaMemoryAction::set_source);
        ClassDB::bind_method(D_METHOD("get_source"), &MaszynaMemoryAction::get_source);
        ClassDB::bind_method(D_METHOD("set_text", "text"), &MaszynaMemoryAction::set_text);
        ClassDB::bind_method(D_METHOD("get_text"), &MaszynaMemoryAction::get_text);
        ClassDB::bind_method(D_METHOD("set_value1", "value1"), &MaszynaMemoryAction::set_value1);
        ClassDB::bind_method(D_METHOD("get_value1"), &MaszynaMemoryAction::get_value1);
        ClassDB::bind_method(D_METHOD("set_value2", "value2"), &MaszynaMemoryAction::set_value2);
        ClassDB::bind_method(D_METHOD("get_value2"), &MaszynaMemoryAction::get_value2);
        ClassDB::bind_method(D_METHOD("set_mask", "mask"), &MaszynaMemoryAction::set_mask);
        ClassDB::bind_method(D_METHOD("get_mask"), &MaszynaMemoryAction::get_mask);
        ClassDB::bind_method(D_METHOD("set_mode", "mode"), &MaszynaMemoryAction::set_mode);
        ClassDB::bind_method(D_METHOD("get_mode"), &MaszynaMemoryAction::get_mode);

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
    void MaszynaMemoryAction::run(const RID &p_event, const RID &p_activator) {
        MaszynaScenarioEventServer *server = MaszynaScenarioEventServer::get_instance();
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
            if (mask.has_flag(MaszynaScenarioEventServer::MEMORY_FIELD_TEXT)) {
                memory_text = add ? memory_text + new_text : new_text;
            }
            if (mask.has_flag(MaszynaScenarioEventServer::MEMORY_FIELD_VALUE1)) {
                memory_value1 = add ? memory_value1 + new_value1 : new_value1;
            }
            if (mask.has_flag(MaszynaScenarioEventServer::MEMORY_FIELD_VALUE2)) {
                switch (mode) {
                    case MODE_SET:
                        memory_value2 = new_value2;
                        break;
                    case MODE_ADD:
                        memory_value2 += new_value2;
                        break;
                    case MODE_ISOLATED_BUSY:
                        memory_value2 = static_cast<int>(memory_value2) | ISOLATED_BUSY_FLAG;
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

    void MaszynaMemoryAction::set_memories(const TypedArray<RID> &p_memories) {
        memories = p_memories;
    }

    TypedArray<RID> MaszynaMemoryAction::get_memories() const {
        return memories;
    }

    void MaszynaMemoryAction::set_source(const RID &p_source) {
        source = p_source;
    }

    RID MaszynaMemoryAction::get_source() const {
        return source;
    }

    void MaszynaMemoryAction::set_text(const String &p_text) {
        text = p_text;
    }

    String MaszynaMemoryAction::get_text() const {
        return text;
    }

    void MaszynaMemoryAction::set_value1(const double p_value1) {
        value1 = p_value1;
    }

    double MaszynaMemoryAction::get_value1() const {
        return value1;
    }

    void MaszynaMemoryAction::set_value2(const double p_value2) {
        value2 = p_value2;
    }

    double MaszynaMemoryAction::get_value2() const {
        return value2;
    }

    void MaszynaMemoryAction::set_mask(const BitField<MaszynaScenarioEventServer::MemoryField> p_mask) {
        mask = p_mask;
    }

    BitField<MaszynaScenarioEventServer::MemoryField> MaszynaMemoryAction::get_mask() const {
        return mask;
    }

    void MaszynaMemoryAction::set_mode(const Mode p_mode) {
        mode = p_mode;
    }

    MaszynaMemoryAction::Mode MaszynaMemoryAction::get_mode() const {
        return mode;
    }
} // namespace godot
