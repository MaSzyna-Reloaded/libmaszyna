extends VBoxContainer
class_name MaszynaWorldPanel

## Live view of a world of the game's own format (MaszynaWorld): its sectors - how many are
## wanted around the camera and how many loaded (MaszynaWorldSectors) - the packs of its blocks
## its source has mounted (MaszynaWorldPackSectorSource), and the last sectors loaded and unloaded
## and packs mounted. Shown in the HUD (DebugHud.attach_world()); refreshed only while it is visible.

## Seconds between refreshes
const REFRESH_INTERVAL:float = 0.25
## Lines of the log kept, the newest first
const MAX_LOG_ROWS:int = 40
const BYTES_PER_MB:float = 1024.0 * 1024.0
const MSEC_PER_SEC:float = 1000.0
## Width of a row's caption [px], as the scenery streaming panel's
const CAPTION_WIDTH:float = 130.0

var _world:MaszynaWorld = null
## The world's source, while it is the packs' one: its blocks are listed
var _source:MaszynaWorldPackSectorSource = null
## Block id -> how many of the world's sectors are in it
var _block_sectors:Dictionary[String, int] = {}
var _rows:Dictionary[String, Label] = {}
var _blocks_label:Label = Label.new()
var _log_label:Label = Label.new()
var _log:PackedStringArray = []
var _timer:Timer = Timer.new()


func _ready() -> void:
    for caption:String in ["World", "Origin", "Sectors", "Wanted", "Loaded", "Waiting", "Camera sector", "Blocks"]:
        var row:HBoxContainer = HBoxContainer.new()
        var caption_label:Label = Label.new()
        caption_label.text = caption
        caption_label.custom_minimum_size = Vector2(CAPTION_WIDTH, 0)
        var value_label:Label = Label.new()
        value_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
        value_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
        row.add_child(caption_label)
        row.add_child(value_label)
        add_child(row)
        _rows[caption] = value_label
    for section:Array in [["Packs", _blocks_label], ["Log", _log_label]]:
        var header:Label = Label.new()
        header.text = section[0]
        add_child(header)
        add_child(section[1])
    _timer.wait_time = REFRESH_INTERVAL
    _timer.timeout.connect(_refresh)
    add_child(_timer)
    _refresh()
    if is_visible_in_tree():
        _timer.start()


func _notification(what:int) -> void:
    if what == NOTIFICATION_VISIBILITY_CHANGED and _timer.is_inside_tree():
        if is_visible_in_tree():
            _refresh()
            _timer.start()
            return
        _timer.stop()


## The world shown, its sectors' and blocks' events logged from now on; null while there is none
## of the own format
func attach_world(world:MaszynaWorld) -> void:
    if _world:
        _world.sectors.sector_loaded.disconnect(_on_sector_loaded)
        _world.sectors.sector_unloaded.disconnect(_on_sector_unloaded)
    if _source:
        _source.block_mounted.disconnect(_on_block_mounted)
    _world = world
    _source = world.sector_source as MaszynaWorldPackSectorSource if world else null
    _block_sectors.clear()
    _log.clear()
    if _world:
        _world.sectors.sector_loaded.connect(_on_sector_loaded)
        _world.sectors.sector_unloaded.connect(_on_sector_unloaded)
        for id:String in _world.sector_ids:
            var block:String = MaszynaWorld.block_of(id)
            _block_sectors[block] = _block_sectors.get(block, 0) + 1
    if _source:
        _source.block_mounted.connect(_on_block_mounted)
    _refresh()


func _on_sector_loaded(id:String) -> void:
    _add_log(tr("loaded %s") % id)


func _on_sector_unloaded(id:String) -> void:
    _add_log(tr("unloaded %s") % id)


func _on_block_mounted(block:String) -> void:
    _add_log(tr("mounted %s") % block if _source.get_blocks()[block]["mounted"] else tr("no pack of %s") % block)


func _add_log(line:String) -> void:
    _log.insert(0, "%7.1f s  %s" % [Time.get_ticks_msec() / MSEC_PER_SEC, line])
    _log.resize(mini(_log.size(), MAX_LOG_ROWS))


func _refresh() -> void:
    if not _world:
        for caption:String in _rows:
            _rows[caption].text = ""
        _rows["World"].text = tr("none of the own format")
        _blocks_label.text = ""
        _log_label.text = ""
        return
    var sectors:MaszynaWorldSectors = _world.sectors
    var wanted:PackedStringArray = sectors.get_wanted_sectors()
    # block id -> its loaded sectors
    var loaded_in:Dictionary[String, int] = {}
    var loaded:int = 0
    for id:String in wanted:
        if sectors.is_sector_loaded(id):
            loaded += 1
            loaded_in[MaszynaWorld.block_of(id)] = loaded_in.get(MaszynaWorld.block_of(id), 0) + 1
    _rows["World"].text = _world.world_name
    _rows["Origin"].text = "E %.0f N %.0f" % [_world.origin.x, _world.origin.y]
    _rows["Sectors"].text = str(sectors.get_sector_count())
    _rows["Wanted"].text = str(wanted.size())
    _rows["Loaded"].text = str(loaded)
    _rows["Waiting"].text = str(wanted.size() - loaded)
    var camera_sector:String = sectors.get_sector_at(SceneryStreamingServer.streaming_get_camera_position()) \
            if SceneryStreamingServer.streaming_has_camera() else ""
    _rows["Camera sector"].text = "%s (%s)" % [camera_sector, MaszynaWorld.block_of(camera_sector)] if camera_sector \
            else tr("none")
    var blocks:Dictionary[String, Dictionary] = _source.get_blocks() if _source else {}
    _rows["Blocks"].text = tr("%d mounted / %d") % [blocks.size(), _block_sectors.size()]
    var lines:PackedStringArray = []
    for block:String in blocks:
        var info:Dictionary = blocks[block]
        lines.append(tr("%s  %.1f MB  at %.1f s  %d/%d loaded") % [block, info["bytes"] / BYTES_PER_MB,
                info["msec"] / MSEC_PER_SEC, loaded_in.get(block, 0), _block_sectors.get(block, 0)]
                if info["mounted"] else tr("%s  no pack") % block)
    _blocks_label.text = "\n".join(lines)
    _log_label.text = "\n".join(_log)

