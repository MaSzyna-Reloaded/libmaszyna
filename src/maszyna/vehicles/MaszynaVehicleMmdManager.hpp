#pragma once
#include "MaszynaMmdProvider.hpp"
#include "cache/ResourceCache.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {
    /* The vehicles' MMD data (MaszynaVehicleMmdData): what each type's .mmd says it is built from, in
     * a skin - and of the vehicle itself when its MMD names (p1), the vehicle's own name
     * (DynObj.cpp:5263) - read once and kept on disk: a scenery repeats the same type across its
     * trainsets. An entry is read again when its .mmd or .fiz changes - their own mtimes, not every
     * .e3d or include they reference; the FIZ's for the pantographs measured from the model, the
     * ones it declares and does not give the geometry of - or the reader's format does. The reader
     * (MaszynaRailVehicle3DInstancer) is handed in by the plugin's runtime (set_mmd_data_reader()).
     * Every MMD and every file it includes is read through the provider it holds
     * (set_mmd_provider()): the disk (MaszynaFileMmdProvider) until a tool hands in another. */
    class MaszynaVehicleMmdManager : public Object {
            GDCLASS(MaszynaVehicleMmdManager, Object)

        private:
            static MaszynaVehicleMmdManager *singleton;
            /* Where the data is kept (ResourceCache) */
            static constexpr const char *CACHE_DIRECTORY = "rail_vehicle";
            /* What in an MMD names the vehicle it is read for (MmdCabinInstancer.vehicle_parameters()) */
            static constexpr const char *VEHICLE_NAME_PARAMETER = "(p1)";

            Ref<ResourceCache> cache;
            /* reader, format_version and provider: set on the main thread, read by the vehicles'
             * workers */
            mutable Mutex mutex;
            Callable reader;
            int format_version = 0;
            Ref<MaszynaMmdProvider> provider;

        protected:
            static void _bind_methods();

        public:
            static MaszynaVehicleMmdManager *get_instance();

            MaszynaVehicleMmdManager();
            ~MaszynaVehicleMmdManager() override;

            /* Whether the MMD at p_abs_mmd_path names the vehicle it is read for - its data is then
             * the vehicle's own, not its type's */
            bool names_vehicle(const String &p_abs_mmd_path) const;
            /* Where the MMD files are read from; one of none is the disk's again */
            void set_mmd_provider(const Ref<MaszynaMmdProvider> &p_provider);
            Ref<MaszynaMmdProvider> get_mmd_provider() const;
            /* The MMD file, or one it includes, as its provider has it: whether it is there, what it
             * says (empty for none) */
            bool mmd_file_exists(const String &p_abs_path) const;
            PackedByteArray mmd_read_file(const String &p_abs_path) const;
            /* What reads a vehicle's MMD data: (data_path, file_name, livery, vehicle_id: String) ->
             * MaszynaVehicleMmdData, and the version of what it puts into it - raised whenever it
             * changes */
            void set_mmd_data_reader(const Callable &p_reader, int p_format_version);
            /* The MMD data of the type p_data_path/p_file_name in its skin p_livery - of the vehicle
             * p_vehicle_id when its MMD names the vehicle; null when the data cannot be read */
            Ref<Resource> load_mmd_data(
                    const String &p_data_path, const String &p_file_name, const String &p_livery,
                    const String &p_vehicle_id);
    };
} // namespace godot
