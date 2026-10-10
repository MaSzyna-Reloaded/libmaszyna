extends MaszynaGutTest

## WorkerTaskQueue runs tasks on worker threads; scenery includes parsed as queue tasks
## give the same result, in the same order, as parsing them in place.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"

var _previous_game_dir:String = ""
var _depth_mutex:Mutex = Mutex.new()
var _thread_depths:Dictionary[int, int] = {}
var _max_depth:int = 0


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_more_tasks_than_workers_return_results_by_id() -> void:
    var queue := WorkerTaskQueue.new()
    var task_count:int = queue.get_worker_count() * 4
    var task_ids:Array[int] = []
    for i:int in task_count:
        task_ids.append(queue.submit(_double.bind(i)))
    for i:int in task_count:
        assert_eq(queue.wait(task_ids[i]), i * 2)
    assert_eq(queue.get_completed_count(), task_count)
    assert_eq(queue.get_submitted_count(), task_count)


func test_nested_waiting_tasks_do_not_deadlock() -> void:
    var queue := WorkerTaskQueue.new()
    var task_id:int = queue.submit(_count_leaves.bind(queue, 6))
    assert_eq(queue.wait(task_id), 64)


## wait() must run the task it waits for and nothing else: running an arbitrary queued task
## nests that task's own wait() on the same stack, which overflowed the stack of a worker thread
## on real sceneries (thousands of includes). Waiting in reverse order makes the awaited task
## something other than the oldest pending one, which is what the old wait() picked.
func test_waiting_runs_only_the_awaited_task() -> void:
    var queue := WorkerTaskQueue.new()
    var task_ids:Array[int] = []
    for i:int in 200:
        task_ids.append(queue.submit(_waiting_task.bind(queue, i)))
    for i:int in range(task_ids.size() - 1, -1, -1):
        assert_eq(queue.wait(task_ids[i]), i * 2)
    assert_lt(_max_depth, 4)


func test_threaded_includes_match_in_place_parsing() -> void:
    var in_place := MaszynaImporterContext.new()
    SceneryInstancer.parse_file("threaded/root.scn", {}, in_place)

    var queue := WorkerTaskQueue.new()
    var threaded:MaszynaImporterContext = SceneryInstancer.parse_file_task(
        "threaded/root.scn", {}, MaszynaImporterContext.new().get_state(), queue
    )

    var expected:Array[String] = ["root1", "a1", "a_b", "a2", "root2", "b1", "root3"]
    assert_eq(_describe(in_place.models).map(func(entry:Array) -> String: return entry[0]), expected)
    assert_eq(_describe(threaded.models), _describe(in_place.models))
    assert_eq(threaded.dependencies.keys().size(), 3)
    assert_true(threaded.cacheable)


## An include reached while a trainset is open is parsed in place, whatever its size: its vehicles
## join the trainset where the include stands, with their own gaps and couplings
func test_an_include_inside_a_trainset_keeps_its_place_in_it() -> void:
    var queue := WorkerTaskQueue.new()
    var context:MaszynaImporterContext = SceneryInstancer.parse_file_task(
        "trainset/root.scn", {}, MaszynaImporterContext.new().get_state(), queue
    )

    assert_eq(context.trainsets.size(), 2, "the trainset, and the dynamic outside it as one of its own")
    var trainset:MaszynaTrainsetData = context.trainsets[0]
    assert_eq(trainset.track_name, "main_track")
    assert_eq(trainset.name, "express")
    assert_eq(trainset.timetable, "express", "its driver's timetable is named after it")
    assert_eq(trainset.dynamics.map(func(dynamic:MaszynaDynamicData) -> String: return dynamic.name),
            ["first", "middle", "last"])
    assert_almost_eq(trainset.dynamics[1].gap, 0.25, 0.001, "the included vehicle keeps its gap")
    assert_eq(trainset.dynamics[1].coupling, 7, "and its coupling")
    var lone:MaszynaTrainsetData = context.trainsets[1]
    assert_eq(lone.track_name, "side_track")
    assert_eq(lone.timetable, "", "a dynamic outside a trainset has no timetable")
    assert_eq(lone.dynamics.size(), 1)


func _describe(models:Array[MaszynaModelData]) -> Array:
    return models.map(func(model:MaszynaModelData) -> Array: return [model.model_filename, model.position])


func _double(value:int) -> int:
    return value * 2


## _double() as a task waiting for a task of its own, counting how deep tasks nest per thread
func _waiting_task(queue:WorkerTaskQueue, value:int) -> int:
    _enter_task()
    var result:int = queue.wait(queue.submit(_double.bind(value)))
    _leave_task()
    return result


func _enter_task() -> void:
    var thread_id:int = OS.get_thread_caller_id()
    _depth_mutex.lock()
    var depth:int = int(_thread_depths.get(thread_id, 0)) + 1
    _thread_depths[thread_id] = depth
    _max_depth = maxi(_max_depth, depth)
    _depth_mutex.unlock()


func _leave_task() -> void:
    var thread_id:int = OS.get_thread_caller_id()
    _depth_mutex.lock()
    _thread_depths[thread_id] = int(_thread_depths[thread_id]) - 1
    _depth_mutex.unlock()


func _count_leaves(queue:WorkerTaskQueue, depth:int) -> int:
    if depth == 0:
        return 1
    var first:int = queue.submit(_count_leaves.bind(queue, depth - 1))
    var second:int = queue.submit(_count_leaves.bind(queue, depth - 1))
    return queue.wait(first) + queue.wait(second)
