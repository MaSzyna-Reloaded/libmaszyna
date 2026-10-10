@tool
extends VBoxContainer

signal item_drag_started(item_data: NodebankGridItem)
signal item_drag_end(item_data: NodebankGridItem)

const ITEMS_PER_PAGE: int = 50

var _builder: Node
var _library: Node
var _preview_request_id: int = 0
var _current_page: int = 0
var _current_group: int = -1
var _plugin_runtime: bool = false
var _drag_preview: bool = false

@onready var _group_selector: OptionButton = $Toolbar/GroupSelector
@onready var _reload_button: Button = $Toolbar/ReloadButton
@onready var _status_label: Label = $Toolbar/Status
@onready var _items_grid: HFlowContainer = $Content/ScrollContainer/ItemsGrid
@onready var _preview_host: Control = $Content/PreviewPanel/PreviewBox/PreviewHost
@onready var _preview_title: Label = $Content/PreviewPanel/PreviewBox/PreviewTitle
@onready var _preview_status: Label = $Content/PreviewPanel/PreviewBox/PreviewStatus
@onready var _search_edit: LineEdit = $Toolbar/Search
@onready var _page_label: Label = $Toolbar/PageNumber
@onready var _page_next_button: Button = $Toolbar/PageNext
@onready var _page_prev_button: Button = $Toolbar/PagePrev
@onready var _preview_renderer = $NodebankPreviewRenderer
@onready var _preview_texture_rect: TextureRect = $Content/PreviewPanel/PreviewBox/PreviewHost/PreviewTexture
@onready var _drag_preview_panel := $DragPreviewPanel
@onready var _drag_preview_texture_rect := $DragPreviewPanel/DragPreviewTexture

func _ready() -> void:
    if _plugin_runtime and visible:
        load_library(false)
        if _library.get_child_count() > 0:
            _group_selector.select(0)
        refresh_items()

func _enter_tree() -> void:
    GameDataServer.data_reload_requested.connect(_on_data_reload_requested)

func _exit_tree() -> void:
    GameDataServer.data_reload_requested.disconnect(_on_data_reload_requested)
    _clear_items()

    if _library:
        _library.queue_free()

func set_plugin_runtime():
    _plugin_runtime = true

func load_library(force_rebuild: bool = false) -> void:
    _clear_items()

    _library = NodebankLibraryBuilder.load_library(force_rebuild).instantiate()

    _group_selector.clear()

    var groups: Array[Node] = _library.get_children()
    for group: Node in groups:
        _group_selector.add_item(group.name)

    if groups.size() > 0:
        _group_selector.select(0)

func refresh_items() -> void:
    _clear_items()

    var search_query = _search_edit.text.strip_edges().to_lower()
    var is_searching: bool = not not search_query
    var search_root = _library
    var filters: Array[Callable] = []

    if search_query:
        filters.append(func (x):
            return (
                x.model_filename.to_lower().contains(search_query)
                or " ".join(x.skins).to_lower().contains(search_query)
            )
        )

    if not is_searching and _current_group >= 0 and _current_group < _library.get_child_count():
        search_root = _library.get_child(_current_group)
        if not search_root:
            search_root = _library

    var results: Array[NodebankGridItem] = []

    for item: E3DModelInstance in search_root.find_children("", "E3DModelInstance", true):
        var is_matching = true
        if is_searching:
            is_matching = filters.reduce(func(accum, filter: Callable): return accum and filter.call(item), true)

        if is_matching:
            var grid_item = NodebankGridItem.new()
            grid_item.model = item
            results.append(grid_item)


    var total_count: int = results.size()
    var total_pages: int = clampi(ceil(float(total_count) / ITEMS_PER_PAGE), 1, 999)

    _current_page = clampi(_current_page, 0, total_pages - 1)

    _status_label.text = "%d items" % total_count
    _page_label.text = "%d/%d" % [_current_page + 1, total_pages]
    _page_prev_button.disabled = _current_page <= 0
    _page_next_button.disabled = _current_page >= total_pages - 1

    var start_idx: int = _current_page * ITEMS_PER_PAGE
    var end_idx: int = min(start_idx + ITEMS_PER_PAGE, total_count)

    for result in results.slice(start_idx, end_idx):
        var tile: Button = NodebankItemTile.new()
        tile.configure(result, _preview_renderer)
        tile.tile_selected.connect(_on_item_selected)
        tile.tile_dragged.connect(_on_item_dragged)
        tile.tile_dropped.connect(_on_item_dropped)
        _items_grid.add_child(tile)

    _clear_preview()

func _clear_items() -> void:
    _stop_drag_preview()
    for child: Node in _items_grid.get_children():
        child.queue_free()

func _clear_preview() -> void:
    if _preview_texture_rect:
        _preview_texture_rect.texture = null
    _preview_title.text = ""
    _preview_status.text = ""

func _on_search_text_changed(_new_text: String) -> void:
    _current_page = 0
    refresh_items()

func _on_page_prev_pressed() -> void:
    _current_page -= 1
    refresh_items()

func _on_page_next_pressed() -> void:
    _current_page += 1
    refresh_items()

func _on_group_selected(index: int) -> void:
    _current_page = 0
    _current_group = index
    refresh_items()

func _on_reload_pressed() -> void:
    load_library(true)
    refresh_items()

func _on_data_reload_requested() -> void:
    if visible:
        load_library(true)
        refresh_items()

func _on_item_selected(item_data: NodebankGridItem) -> void:
    _preview_title.text = item_data.model.name
    _preview_status.text = item_data.model.data_path.path_join(item_data.model.model_filename)
    _preview_texture_rect.texture = await _preview_renderer.get_preview(item_data)

func _on_item_dragged(item_data: NodebankGridItem) -> void:
    _drag_preview_texture_rect.texture = item_data.preview_texture
    if item_data.preview_texture:
        _drag_preview_panel.visible = true
        _drag_preview = true
    item_drag_started.emit(item_data)

func _on_item_dropped(item_data: NodebankGridItem) -> void:
    item_drag_end.emit(item_data)
    _stop_drag_preview()

func _on_visibility_changed() -> void:
    if visible and _plugin_runtime:
        load_library(false)
        refresh_items()
    elif not visible:
        _stop_drag_preview()

func _process(_delta: float) -> void:
    if _drag_preview:
        _process_drag_preview(_delta)

func _process_drag_preview(_delta: float) -> void:
    var mouse_position: Vector2 = get_local_mouse_position()
    if not Rect2(Vector2.ZERO, size).has_point(mouse_position):
        _drag_preview_panel.visible = false
        return
    elif not _drag_preview_panel.visible:
        _drag_preview_panel.visible = true

    var preview_size: Vector2 = _drag_preview_panel.size
    _drag_preview_panel.global_position = get_global_mouse_position() - preview_size * 0.5

func _stop_drag_preview() -> void:
    _drag_preview = false
    _drag_preview_texture_rect.texture = null
    _drag_preview_panel.visible = false
