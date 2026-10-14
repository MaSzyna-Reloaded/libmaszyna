#include "PersonServer.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *PersonServer::person_created_signal = "person_created";
    const char *PersonServer::person_name_changed_signal = "person_name_changed";
    const char *PersonServer::person_freed_signal = "person_freed";

    void PersonServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("person_create", "name"), &PersonServer::person_create, DEFVAL(String()));
        ClassDB::bind_method(D_METHOD("person_free", "person"), &PersonServer::person_free);
        ClassDB::bind_method(D_METHOD("person_exists", "person"), &PersonServer::person_exists);
        ClassDB::bind_method(D_METHOD("person_set_name", "person", "name"), &PersonServer::person_set_name);
        ClassDB::bind_method(D_METHOD("person_get_name", "person"), &PersonServer::person_get_name);
        ADD_SIGNAL(MethodInfo(person_created_signal, PropertyInfo(Variant::RID, "person")));
        ADD_SIGNAL(MethodInfo(
                person_name_changed_signal, PropertyInfo(Variant::RID, "person"),
                PropertyInfo(Variant::STRING, "previous")));
        ADD_SIGNAL(MethodInfo(person_freed_signal, PropertyInfo(Variant::RID, "person")));
    }

    RID PersonServer::person_create(const String &p_name) {
        const RID person = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        persons.insert(person, p_name);
        emit_signal(person_created_signal, person);
        return person;
    }

    void PersonServer::person_free(const RID &p_person) {
        if (!persons.has(p_person)) {
            return;
        }
        emit_signal(person_freed_signal, p_person);
        persons.erase(p_person);
    }

    bool PersonServer::person_exists(const RID &p_person) const {
        return persons.has(p_person);
    }

    void PersonServer::person_set_name(const RID &p_person, const String &p_name) {
        String *name = persons.getptr(p_person);
        ERR_FAIL_NULL(name);
        if (*name == p_name) {
            return;
        }
        const String previous = *name;
        *name = p_name;
        emit_signal(person_name_changed_signal, p_person, previous);
    }

    String PersonServer::person_get_name(const RID &p_person) const {
        const String *name = persons.getptr(p_person);
        ERR_FAIL_NULL_V(name, String());
        return *name;
    }
} // namespace godot
