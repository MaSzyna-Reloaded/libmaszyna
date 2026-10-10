#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    /// The memory of the whole process, as the system sees it - not Godot's own count of its
    /// allocations (Performance.MEMORY_STATIC), which misses native buffers and memory the
    /// allocator keeps after it is freed. glibc keeps what each thread's arena freed, so a parse
    /// on a dozen workers leaves the resident size high long after its data is gone.
    class ProcessMemory : public Object {
            GDCLASS(ProcessMemory, Object)

        protected:
            static void _bind_methods();

        public:
            /* Resident set size in bytes; -1 where it is not known (Linux only) */
            static int64_t get_resident_bytes();
            /* Gives the memory the allocator keeps free back to the system (glibc only) */
            static void release_unused();
    };
} // namespace godot
