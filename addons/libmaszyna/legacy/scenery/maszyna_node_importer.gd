@tool
extends RefCounted

static var track_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_track_importer.gd").new()
static var traction_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_traction_importer.gd").new()
static var model_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_model_importer.gd").new()
static var triangles_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_triangles_importer.gd").new()
static var dynamic_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_dynamic_importer.gd").new()
static var power_source_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_tractionpowersource_importer.gd").new()
static var memcell_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_memcell_importer.gd").new()
static var eventlauncher_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_eventlauncher_importer.gd").new()
static var sound_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_sound_importer.gd").new()

# FIXME: _current_rotation, _origin change to parent nodes
func import(p:MaszynaParser, context: MaszynaImporterContext):
    var range_max = float(p.next_token())
    var range_min = float(p.next_token())
    var name = p.next_token()
    # "none" is this format's sentinel for "no value given" everywhere (skin, load type, ...),
    # not a real name - treating it as one makes every "none"-named track/vehicle after the
    # first collide (TrackServer.track_update() rejects the duplicate and returns early,
    # silently skipping that track's width/type/AABB setup entirely; a vehicle named "none" is
    # left out of VehicleServer's name lookup the same way, Names.h:29).
    if name.to_lower() == "none":
        name = ""

    var type = p.next_token().to_lower()
    var node_name = name if name else "node_%s" % type
    var obj

    match type:
        "eventlauncher":
            var launcher:MaszynaEventLauncherData = eventlauncher_importer.import(p, context)
            launcher.name = name
            launcher.position = launcher.position.rotated(Vector3.UP, context.rotate.y) + context.origin
            context.launchers.append(launcher)

        "triangles":
            # with a region file the shapes are the file's (simulationstateserializer.cpp:567-585)
            if context.binary_terrain:
                p.get_tokens_until("endtri")
            else:
                triangles_importer.import(p, context, range_min, range_max)

        "sound":
            var sound:MaszynaSoundData = sound_importer.import(p, context)
            sound.name = name
            sound.range_max = range_max
            sound.position = sound.position.rotated(Vector3.UP, context.rotate.y) + context.origin
            context.sounds.append(sound)

        "traction":
            var traction = traction_importer.import(p, context)
            if traction:
                context.traction.append(traction)

        "tractionpowersource":
            var power_source = power_source_importer.import(p, context)
            if power_source:
                power_source.name = name
                context.power_sources.append(power_source)

        "model":
            # terrain - a negative range_min - is shapes of the region file when there is one
            # (simulationstateserializer.cpp:502-533)
            var model:MaszynaModelData = null
            if range_min < 0.0 and context.binary_terrain:
                p.get_tokens_until("endmodel")
            else:
                model = model_importer.import(p, context)
            if model:
                model.name = name
                # same context transform as for Node3D objects below
                model.position = model.position.rotated(Vector3.UP, context.rotate.y) + context.origin
                model.rotation += Vector3(context.rotate)
                if range_max > 0:
                    model.range_min = range_min
                    model.range_max = range_max
                context.models.append(model)

        "track":
            var track = track_importer.import(p, context)
            if track:
                track.track_name = name
                context.tracks.append(track)

        "dynamic":
            dynamic_importer.import(p, context).name = name

        "memcell":
            var memcell:MaszynaMemcellData = memcell_importer.import(p, context)
            memcell.name = name
            memcell.position = memcell.position.rotated(Vector3.UP, context.rotate.y) + context.origin
            context.memcells.append(memcell)
        "lines":
            p.get_tokens_until("endline")
            #push_warning("Lines node is not supported yet")
        _:
            #push_error("Unhandled node type: "+type)
            pass
    if obj:
        obj.name = node_name
    if obj is Node3D:
        # Matches the original engine's transform() (simulationstateserializer.cpp): the node's
        # own LOCAL offset is rotated by the enclosing rotate: context before the origin: offset
        # is added - not just translated by context.origin directly. Skipping the rotation step
        # placed every node inside a rotated origin/rotate block (e.g. a traction pole's bracket
        # arm, offset from its pole by tra/sb165-3d.inc's own "rotate 0 (p5) 0") on the wrong side.
        obj.position = (obj.position as Vector3).rotated(Vector3.UP, context.rotate.y) + context.origin
        obj.rotation += Vector3(context.rotate)

    if range_max > 0 and obj and obj is GeometryInstance3D:
        obj.visibility_range_begin = range_min
        obj.visibility_range_end = range_max
    #if obj is MaSzyna_Node:
    #    obj.node_name = node_name
    #    obj.range_min = range_min
    #    obj.range_max = range_max
    # a node that is data (a model, a track, triangles...) leaves nothing among the objects
    return [obj] if obj else []
