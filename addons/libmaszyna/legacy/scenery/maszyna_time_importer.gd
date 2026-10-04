@tool
extends RefCounted


func import(p: MaszynaParser, _context: MaszynaImporterContext) -> Array:
    var tokens = p.get_tokens_until("endtime")
    if not tokens:
        return []

    var start_time:float = parse_hhmm_to_float(tokens[0])
    if is_nan(start_time):
        return []

    var node := MaszynaTimeNode.new()
    node.name = "Time"
    node.start_time = start_time
    return [node]


func parse_hhmm_to_float(value: String) -> float:
    var parts := value.strip_edges().split(":")
    if parts.size() != 2:
        return NAN

    if not parts[0].is_valid_int() or not parts[1].is_valid_int():
        return NAN

    var h := parts[0].to_int()
    var m := parts[1].to_int()

    if h < 0 or h > 24:
        return NAN
    if m < 0 or m > 59:
        return NAN
    if h == 24 and m != 0:
        return NAN

    return float(h) + float(m) / 60.0
