#pragma once
#include "cache/ResourceCache.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/rail/RailVehicleWipers.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {
    /* The vehicles' .fiz files as their descriptions - a vehicle's model, a vehicle is built from a
     * copy of it (VehicleServer::controller_configure()) - each read once and kept on disk: a
     * scenery repeats the same file across many trainset entries, and parsing it is the expensive
     * part. An entry is read again when its file changes (its mtime - an `include`d one's is not
     * seen), the reader's format does or the wipers the game's patch gives the file do. The reader is the FIZ parser (FizVehicleBuilder), handed in
     * by the plugin's runtime (set_description_reader()). */
    class MaszynaFizManager : public Object {
            GDCLASS(MaszynaFizManager, Object)

        private:
            static MaszynaFizManager *singleton;
            /* Where the descriptions are kept (ResourceCache) */
            static constexpr const char *CACHE_DIRECTORY = "fiz";

            Ref<ResourceCache> cache;
            /* reader and format_version: set on the main thread, read by the vehicles' workers */
            mutable Mutex mutex;
            Callable reader;
            int format_version = 0;

        protected:
            static void _bind_methods();

        public:
            static MaszynaFizManager *get_instance();

            MaszynaFizManager();
            ~MaszynaFizManager() override;

            /* What reads a .fiz into its description: (fiz_path: String, wipers: RailVehicleWipers)
             * -> VehicleController, and the version of what it builds - raised whenever the parser
             * changes it */
            void set_description_reader(const Callable &p_reader, int p_format_version);
            /* The vehicle the .fiz of p_data_path/p_fiz_filename in the game's data describes;
             * p_wipers - the wipers the game's patch gives a FIZ of no WiperList
             * (MaszynaVehicleSystem.fiz_get_patched_wipers()), none without a patch */
            Ref<VehicleController> load_description(
                    const String &p_data_path, const String &p_fiz_filename,
                    const Ref<RailVehicleWipers> &p_wipers = Ref<RailVehicleWipers>());
            /* The vehicle the .fiz at p_fiz_path describes, with p_wipers as load_description();
             * null without a reader */
            Ref<VehicleController> load_description_at(
                    const String &p_fiz_path, const Ref<RailVehicleWipers> &p_wipers = Ref<RailVehicleWipers>());
    };
} // namespace godot
