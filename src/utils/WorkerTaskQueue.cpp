#include "utils/WorkerTaskQueue.hpp"
#include <godot_cpp/classes/os.hpp>

namespace godot {
    void WorkerTaskQueue::_bind_methods() {
        ClassDB::bind_method(D_METHOD("drain"), &WorkerTaskQueue::drain);
        ClassDB::bind_method(D_METHOD("submit", "task"), &WorkerTaskQueue::submit);
        ClassDB::bind_method(D_METHOD("is_done", "task_id"), &WorkerTaskQueue::is_done);
        ClassDB::bind_method(D_METHOD("wait", "task_id"), &WorkerTaskQueue::wait);
        ClassDB::bind_method(D_METHOD("get_completed_count"), &WorkerTaskQueue::get_completed_count);
        ClassDB::bind_method(D_METHOD("get_submitted_count"), &WorkerTaskQueue::get_submitted_count);
        ClassDB::bind_method(D_METHOD("get_worker_count"), &WorkerTaskQueue::get_worker_count);
    }

    WorkerTaskQueue::WorkerTaskQueue() {
        semaphore.instantiate();
    }

    /// Queued tasks are dropped, running ones are finished before the workers are joined.
    WorkerTaskQueue::~WorkerTaskQueue() {
        drain();
    }

    void WorkerTaskQueue::drain() {
        {
            MutexLock lock(mutex);
            exiting = true;
            pending.clear();
        }
        // a queue that never got a task has no workers to wake, and post(0) is an error
        if (!workers.is_empty()) {
            semaphore->post(static_cast<int32_t>(workers.size()));
        }
        for (const Ref<Thread> &worker: workers) {
            worker->wait_to_finish();
        }
        workers.clear();
    }

    int WorkerTaskQueue::submit(const Callable &p_task) {
        // the workers start with the first task, not in the constructor (the object is fully set up then)
        if (workers.is_empty()) {
            for (int i = 0; i < get_worker_count(); i++) {
                Ref<Thread> worker;
                worker.instantiate();
                worker->start(callable_mp(this, &WorkerTaskQueue::_worker_loop));
                workers.push_back(worker);
            }
        }
        int task_id = 0;
        {
            MutexLock lock(mutex);
            task_id = next_id++;
            tasks[task_id].callable = p_task;
            pending.push_back(task_id);
        }
        semaphore->post();
        return task_id;
    }

    bool WorkerTaskQueue::is_done(const int p_task_id) const {
        MutexLock lock(mutex);
        // a queue being torn down runs nothing more, so whoever polls this has to be let go
        const Task *task = tasks.getptr(p_task_id);
        return exiting || (task != nullptr && task->done);
    }

    /// Returns the task result and forgets the task; runs the awaited task here if it is queued.
    Variant WorkerTaskQueue::wait(const int p_task_id) {
        while (true) {
            {
                MutexLock lock(mutex);
                /* A task waits here for a task it submitted, and drain() drops what is queued -
                 * so without this the waiter waits for something that will never run, its worker
                 * is never joined, and the join blocks the main thread for good. Giving up is the
                 * only answer: the result is not wanted any more either (see `FINDINGS.md`,
                 * 2026-09-24). */
                if (exiting) {
                    return Variant();
                }
                const HashMap<int, Task>::Iterator task = tasks.find(p_task_id);
                ERR_FAIL_COND_V_MSG(task == tasks.end(), Variant(), vformat("Unknown task id: %d", p_task_id));
                if (task->value.done) {
                    Variant result = task->value.result;
                    tasks.remove(task);
                    return result;
                }
            }
            // Only the awaited task, never an arbitrary queued one: an unrelated task waits for
            // its own tasks on this stack, so a scenery's thousands of includes would nest until
            // the worker thread's stack runs out.
            if (!_run_task(p_task_id)) {
                // the task runs on another thread
                OS::get_singleton()->delay_usec(AWAIT_POLL_USEC);
            }
        }
    }

    int WorkerTaskQueue::get_completed_count() const {
        MutexLock lock(mutex);
        return completed;
    }

    /// Tasks submitted so far - task ids are handed out in order from 0
    int WorkerTaskQueue::get_submitted_count() const {
        MutexLock lock(mutex);
        return next_id;
    }

    int WorkerTaskQueue::get_worker_count() const {
        return MAX(OS::get_singleton()->get_processor_count() - RESERVED_CORES, 1);
    }

    /// Runs one queued task, false when it is not queued any more (it runs on another thread).
    bool WorkerTaskQueue::_run_task(const int p_task_id) {
        Callable callable;
        {
            MutexLock lock(mutex);
            if (!pending.erase(p_task_id)) {
                return false;
            }
            callable = _take_callable(p_task_id);
        }

        _run(p_task_id, callable);
        return true;
    }

    /// Takes the task's callable out of the queue - caller holds the mutex.
    Callable WorkerTaskQueue::_take_callable(const int p_task_id) {
        Task &task = tasks[p_task_id];
        const Callable callable = task.callable;
        task.callable = Callable();
        return callable;
    }

    void WorkerTaskQueue::_run(const int p_task_id, Callable &p_callable) {
        const Variant result = p_callable.call();
        // Drop the task's references (it may hold this queue) before it is reported as done, so the
        // last reference to the queue is never released on a worker thread (joining itself)
        p_callable = Callable();

        MutexLock lock(mutex);
        Task &task = tasks[p_task_id];
        task.result = result;
        task.done = true;
        completed++;
    }

    void WorkerTaskQueue::_worker_loop() {
        while (true) {
            semaphore->wait();
            Callable callable;
            int task_id = 0;
            {
                MutexLock lock(mutex);
                if (exiting) {
                    return;
                }
                // a waiting thread may have taken the task already
                if (pending.is_empty()) {
                    continue;
                }
                task_id = pending.front()->get();
                pending.pop_front();
                callable = _take_callable(task_id);
            }
            _run(task_id, callable);
        }
    }
} // namespace godot
