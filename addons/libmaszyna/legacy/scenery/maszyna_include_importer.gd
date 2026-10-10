@tool
extends RefCounted

## What an include of terrain is named with (parser.cpp:330)
const TERRAIN_INCLUDE:String = "_ter.scm"


func import(p: MaszynaParser, context: MaszynaImporterContext):
    var tokens = p.get_tokens_until("end")
    tokens.pop_back()
    var include_token:Variant = tokens.pop_front()
    if not include_token:
        # a truncated or malformed "include" - nothing to resolve
        context.cacheable = false
        push_error("Include without a filename at offset %d" % p.get_position())
        return []
    # with a region file the terrain is the file's (parser.cpp:330, "SBT found, ignoring")
    if context.binary_terrain_state and String(include_token).to_lower().contains(TERRAIN_INCLUDE):
        return []
    var filename:String = resolve_filename(include_token)
    var final_path = UserSettings.get_maszyna_game_dir().path_join("scenery").path_join(filename)
    var file = FileAccess.open(final_path, FileAccess.READ)
    var parameters = {}
    for i in range(tokens.size()):
        parameters["p%s" % (i+1)] = tokens[i]
    if file:
        # The original engine uses the same "include" for parameterised object instances and for
        # parts of the scenery. An object - placed thousands of times (grass.inc, tree.inc), any
        # size - is parsed in place, as is a small part: a task of its own kept a whole context
        # until its parent's merge, gigabytes for a large scenery. A larger part without parameters
        # is parsed by a queue worker and merged at the end of the current file; at least
        # SUBSCENE_MIN_SIZE, it is a (cached) subscene. Inside an open trainset every include is
        # parsed in place, as the original's parser reads it: its vehicles join the trainset where
        # the include stands.
        if (
            context.queue and not parameters and not context.trainset
            and file.get_length() >= SceneryInstancer.INLINE_INCLUDE_MAX_SIZE
        ):
            if (
                file.get_length() >= SceneryInstancer.SUBSCENE_MIN_SIZE
                and context.subscene_depth < SceneryInstancer.SUBSCENE_MAX_DEPTH
            ):
                return [context.submit_include(SceneryInstancer.parse_subscene_task, filename, parameters)]
            return [context.submit_include(SceneryInstancer.parse_file_task, filename, parameters)]
        context.push_state()
        context.include_depth += 1
        var objects = SceneryInstancer.parse_file(filename, parameters, context)
        context.pop_state()
        return objects
    else:
        context.cacheable = false
        push_error("Cannot load include file: " + final_path)
        return []


## Include path relative to scenery/, as it exists on disk. Also used by SceneryInstancer's
## include prescan.
func resolve_filename(filename: String) -> String:
    var scenery_dir: String = UserSettings.get_maszyna_game_dir().path_join("scenery")
    return MaszynaDataPath.resolve(scenery_dir, filename)
