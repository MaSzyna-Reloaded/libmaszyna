extends PoweredTrainPart
var _t = 0.0
var _log: GameLogger = GameLog.get_logger("game")


func _process_powered(delta):
    _t += delta
    if _t > 2.0:
        _t = 0.0
        _log.debug("Powered train part example: POWERED")

func _process_unpowered(delta):
    _t += delta
    if _t > 2.0:
        _t = 0.0
        _log.debug("Powered train part example: UNPOWERED")
