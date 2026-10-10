#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/semaphore.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {
    /// FIFO of tasks (Callables) run by max(processor count - 2, 1) worker threads of its own -
    /// never Godot's WorkerThreadPool, which the engine's own loading waits for: a task that waits
    /// for the main thread there deadlocks it (docs/findings-archive.md, 2026-10-02).
    /// A task may submit further tasks and wait() for them: the waiting thread runs the task it
    /// waits for, so nested waiting never deadlocks the workers and never nests deeper than the
    /// tasks themselves. Used to parse scenery includes, to preload streamed pieces
    /// (SceneryStreamingServer) and to read vehicle profiles.
    class WorkerTaskQueue : public RefCounted {
            GDCLASS(WorkerTaskQueue, RefCounted)

        private:
            /* How long a wait sleeps while the awaited task runs on another thread */
            static constexpr uint64_t AWAIT_POLL_USEC = 100;
            /* Cores the workers leave to the rest of the engine */
            static constexpr int RESERVED_CORES = 2;
            struct Task {
                    Callable callable;
                    Variant result;
                    bool done = false;
            };

            Mutex mutex;
            Ref<Semaphore> semaphore;
            Vector<Ref<Thread>> workers;
            HashMap<int, Task> tasks;
            List<int> pending;
            int next_id = 0;
            int completed = 0;
            bool exiting = false;

            bool _run_task(int p_task_id);
            Callable _take_callable(int p_task_id);
            void _run(int p_task_id, Callable &p_callable);
            void _worker_loop();

        protected:
            static void _bind_methods();

        public:
            WorkerTaskQueue();
            ~WorkerTaskQueue() override;

            /* Drops what is queued and joins the workers, finishing whatever is running. Called
             * while the scene tree is still alive: the tasks are GDScript and they call GDScript
             * handlers (MaszynaParser's), so a worker still inside one when the scripts go away
             * jumps into freed code (see `FINDINGS.md`, 2026-09-24). The destructor does the same,
             * but it runs when the last reference goes - which is during that teardown, not
             * before it. */
            void drain();
            int submit(const Callable &p_task);
            bool is_done(int p_task_id) const;
            Variant wait(int p_task_id);
            int get_completed_count() const;
            int get_submitted_count() const;
            int get_worker_count() const;
    };
} // namespace godot
