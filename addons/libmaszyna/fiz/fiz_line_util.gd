@tool
extends RefCounted
class_name FizLineUtil

## Shared helpers built on top of a MaszynaParser instance already scoped to a single FIZ
## line (see FizVehicleBuilder, which creates one such instance per line and hands
## it to the matching section parser's parse()/parse_row(), exactly like the original
## LoadFIZ_* code constructing "a fresh cParser parser(line)" per line/row). Comment stripping
## (`//`, `/* */`) is MaszynaParser's own job during tokenization - nothing here reimplements
## it.

const _STOPS: Array = [" ", "\t", "\n", "\r", ";"]


## Pulls every `Key=Value` token out of `p` (until eof) into a String->String dictionary. Keys
## are kept case-sensitive, matching the original `extract_value`'s exact-substring lookup. A key
## given twice keeps its first value, as `extract_value`'s `find()` does (utilities.h:170) - the
## data relies on it (BR285's `Engine:` says `Vadd=5.5 ... Vadd=0.0`). A value written after spaces
## (`BCM= 6.5`) is the next token, as `extract_value` skips the spaces after the `=`
## (utilities.h:151); that token is still read as a key of its own.
static func read_key_values(p: MaszynaParser) -> Dictionary:
    var tokens: Array[String] = []
    while not p.eof_reached():
        var token: String = p.next_token(_STOPS)
        if token:
            tokens.append(token)
    var result: Dictionary = {}
    for i: int in tokens.size():
        var token: String = tokens[i]
        var eq: int = token.find("=")
        if eq <= 0:
            continue
        var key: String = token.substr(0, eq)
        if result.has(key):
            continue
        var value: String = token.substr(eq + 1)
        if not value and i + 1 < tokens.size():
            value = tokens[i + 1]
        result[key] = value
    return result


static func get_string(kv: Dictionary, key: String, default_value: String = "") -> String:
    return kv.get(key, default_value)


static func has_key(kv: Dictionary, key: String) -> bool:
    return kv.has(key)


static func get_float(kv: Dictionary, key: String, default_value: float = 0.0) -> float:
    var v: String = kv.get(key, "")
    if not v:
        return default_value
    return v.to_float()


static func get_int(kv: Dictionary, key: String, default_value: int = 0) -> int:
    var v: String = kv.get(key, "")
    if not v:
        return default_value
    return v.to_int()


## FIZ boolean convention: literal `Yes`/`No` tokens (case-insensitive). Anything else -
## including an absent key - is false.
static func get_bool(kv: Dictionary, key: String, default_value: bool = false) -> bool:
    var v: String = kv.get(key, "")
    if not v:
        return default_value
    return v.to_lower() == "yes"
