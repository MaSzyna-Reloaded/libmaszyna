#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /* The people of the world - the player and every AI driver - by their handles and names. A
     * person is only a handle and a name here: where it sits and in what role is VehicleServer's,
     * what an AI driver thinks is DriverServer's, the player is PlayerServer's. */
    class PersonServer : public Object {
            GDCLASS(PersonServer, Object)

        public:
            static PersonServer *get_instance() {
                return Object::cast_to<PersonServer>(Engine::get_singleton()->get_singleton("PersonServer"));
            }

        private:
            /* Every person and its name */
            HashMap<RID, String> persons;

        protected:
            static void _bind_methods();

        public:
            /* A person came to be (person: RID) */
            static const char *person_created_signal;
            /* A person has another name (person: RID, previous: String) */
            static const char *person_name_changed_signal;
            /* The person is going (person: RID) - whoever keeps something of it lets it go; its
             * name can still be read */
            static const char *person_freed_signal;

            RID person_create(const String &p_name = String());
            void person_free(const RID &p_person);
            bool person_exists(const RID &p_person) const;
            void person_set_name(const RID &p_person, const String &p_name);
            String person_get_name(const RID &p_person) const;
    };
} // namespace godot
