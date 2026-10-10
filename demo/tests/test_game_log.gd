extends MaszynaGutTest

## GameLog only manages: handlers registered by name, assigned to loggers by name, attached to a
## logger by GameLog - in whatever order the three come. The logger calls its handlers itself.

const LOGGER_A:String = "test_game_log_a"
const LOGGER_B:String = "test_game_log_b"
const HANDLER:String = "test_game_log_handler"

## Keeps the lines it is given
class RecordingHandler extends GameLogHandler:
    var lines:Array[String] = []

    func _handle(logger_id:String, _loglevel:GameLog.LogLevel, line:String) -> void:
        lines.append("%s: %s" % [logger_id, line])


var _handler:RecordingHandler


func before_each() -> void:
    _handler = RecordingHandler.new()


func after_each() -> void:
    for logger_id:String in [LOGGER_A, LOGGER_B]:
        if logger_id in GameLog.get_loggers():
            GameLog.remove_logger(logger_id)
    for logger_id:String in [LOGGER_A, LOGGER_B]:
        GameLog.unassign_handler(logger_id, HANDLER)
    GameLog.unregister_handler(HANDLER)


func test_one_handler_serves_the_loggers_it_is_assigned_to() -> void:
    GameLog.register_handler(HANDLER, _handler)
    GameLog.assign_handler(LOGGER_A, HANDLER)
    GameLog.assign_handler(LOGGER_B, HANDLER)
    GameLog.get_logger(LOGGER_A).info("one")
    GameLog.get_logger(LOGGER_B).info("two")
    assert_eq(_handler.lines, [LOGGER_A + ": one", LOGGER_B + ": two"])


func test_a_handler_registered_after_its_assignment_and_the_logger_is_attached() -> void:
    var logger:GameLogger = GameLog.get_logger(LOGGER_A)
    GameLog.assign_handler(LOGGER_A, HANDLER)
    GameLog.assign_handler(LOGGER_B, HANDLER)
    logger.info("before")
    GameLog.register_handler(HANDLER, _handler)
    logger.info("after")
    assert_eq(_handler.lines, [LOGGER_A + ": after"], "nothing of the logger before the handler came")


func test_a_logger_created_after_the_assignment_gets_the_handler() -> void:
    GameLog.register_handler(HANDLER, _handler)
    GameLog.assign_handler(LOGGER_A, HANDLER)
    GameLog.assign_handler(LOGGER_B, HANDLER)
    assert_false(LOGGER_A in GameLog.get_loggers(), "no logger before it is asked for")
    GameLog.get_logger(LOGGER_A).info("line")
    assert_eq(_handler.lines, [LOGGER_A + ": line"])


func test_unassigned_and_unregistered_handlers_get_nothing() -> void:
    GameLog.register_handler(HANDLER, _handler)
    GameLog.assign_handler(LOGGER_A, HANDLER)
    GameLog.assign_handler(LOGGER_B, HANDLER)
    GameLog.unassign_handler(LOGGER_A, HANDLER)
    GameLog.get_logger(LOGGER_A).info("unassigned")
    GameLog.get_logger(LOGGER_B).info("assigned")
    GameLog.unregister_handler(HANDLER)
    GameLog.get_logger(LOGGER_B).info("unregistered")
    GameLog.register_handler(HANDLER, _handler)
    GameLog.assign_handler(LOGGER_A, HANDLER)
    assert_eq(_handler.lines, [LOGGER_B + ": assigned"])


func test_a_removed_logger_is_announced_first_and_logs_to_nobody() -> void:
    GameLog.register_handler(HANDLER, _handler)
    GameLog.assign_handler(LOGGER_A, HANDLER)
    GameLog.assign_handler(LOGGER_B, HANDLER)
    var logger:GameLogger = GameLog.get_logger(LOGGER_A)
    var removing:Array[String] = []
    var on_removing:Callable = func(logger_id:String) -> void:
        removing.append(logger_id)
        GameLog.get_logger(logger_id).info("last words")
    GameLog.logger_removing.connect(on_removing)
    GameLog.remove_logger(LOGGER_A)
    GameLog.logger_removing.disconnect(on_removing)
    logger.info("after removal")
    assert_eq(removing, [LOGGER_A])
    assert_false(LOGGER_A in GameLog.get_loggers())
    assert_eq(_handler.lines, [LOGGER_A + ": last words"], "still attached while announced, then detached")
