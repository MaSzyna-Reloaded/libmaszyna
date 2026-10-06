#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /// The game's data - everything read from the MaSzyna game directory - and what is kept of it:
    /// the caches on disk and what the servers hold in memory and have built from it.
    ///
    /// It knows none of them. Each owner follows its requests itself, the way every ResourceCache
    /// follows cache_clear_requested: "Clear cache" throws the caches on disk away, and a reload -
    /// the game directory changed, or "Reload models" asked - goes in two rounds: every owner first
    /// drops what it holds in memory (data_unload_requested), then builds again what it has built
    /// (data_reload_requested). Whatever is built takes its data from its owner when it needs it,
    /// so by the second round every owner hands out the new data, in whatever order they rebuild.
    class GameDataServer : public Object {
            GDCLASS(GameDataServer, Object)

        private:
            static GameDataServer *singleton;
            static constexpr const char *BUILD_NUMBER_PATH = "res://build_number.txt";
            static constexpr const char *BUILD_SECTION = "app";
            static constexpr const char *BUILD_NUMBER_KEY = "build_number";

            /* Stamped by the build itself (cmake/write_build_number.cmake), empty in a checkout
             * that was never built */
            String build_number;
            bool build_version_checked = false;

        protected:
            static void _bind_methods();

        public:
            static const char *cache_clear_requested_signal;
            static const char *data_unload_requested_signal;
            static const char *data_reload_requested_signal;

            /* Set as the server is created, so the servers created after it follow it from their
             * own constructors */
            static GameDataServer *get_instance();

            GameDataServer();
            ~GameDataServer() override;

            /* Throws the caches on disk away: every cache follows cache_clear_requested */
            void cache_clear();
            /* Everything read from the game directory is read again: dropped in memory by every
             * owner, then built again - the game directory changed, or the data in it */
            void data_reload();
            String build_get_number() const;
            /* Clears every cache once per run when the build behind them is not the one that
             * wrote them */
            bool build_check_version();
    };
} // namespace godot
