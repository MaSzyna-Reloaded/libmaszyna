#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
    /// The HUD - the one owner of what it shows: which of its panels are open, the HUD shown at
    /// all, and the vehicle whose card is open. The HUD's scenes draw this state and ask for its
    /// changes here - a menu, a close button, a key, a script alike; the server knows no panel of
    /// its own, a panel is a name the HUD gives it.
    class HUDServer : public Object {
            GDCLASS(HUDServer, Object)

        public:
            /// A panel was opened or closed (panel: StringName, visible: bool)
            static const char *panel_visibility_changed_signal;
            /// The HUD was shown or hidden (visible: bool)
            static const char *hud_visibility_changed_signal;
            /// The card shows another vehicle (vehicle: RID, invalid when closed)
            static const char *card_changed_signal;

            static HUDServer *get_instance() {
                return Object::cast_to<HUDServer>(Engine::get_singleton()->get_singleton("HUDServer"));
            }

        private:
            HashMap<StringName, bool> panels;
            bool hud_visible = true;
            RID card_vehicle;

            void _on_vehicle_freed(const RID &p_vehicle);

        protected:
            static void _bind_methods();

        public:
            HUDServer();

            void panel_set_visible(const StringName &p_panel, bool p_visible);
            /// A panel never set is closed
            bool panel_is_visible(const StringName &p_panel) const;
            void panel_toggle(const StringName &p_panel);
            void hud_set_visible(bool p_visible);
            bool hud_is_visible() const;
            /// The card of the vehicle, the one open showing it instead
            void card_open(const RID &p_vehicle);
            void card_close();
            RID card_get_vehicle() const;
    };
} // namespace godot
