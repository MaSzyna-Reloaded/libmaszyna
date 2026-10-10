@tool
extends Container
class_name HUDWindow

const HEADER_HEIGHT: float = 28.0
const CONTENT_PADDING: float = 10.0
const BORDER_WIDTH: float = 1.0
const TITLE_MARGIN_X: float = 8.0
const TITLE_BASELINE_OFFSET: float = 18.0
const CLOSE_BUTTON_MARGIN: float = 4.0
const CLOSE_BUTTON_SIZE: float = 20.0
const CLOSE_BUTTON_STROKE: float = 2.0
const DEFAULT_MINIMUM_SIZE: Vector2 = Vector2(100.0, 50.0)
const RESIZE_HANDLE_SIZE: Vector2 = Vector2(24.0, 24.0)
const RESIZE_HANDLE_TEXTURE: Texture2D = preload("drag-handle-corner-line.svg")

var _dragging: bool = false
var _drag_grab_offset: Vector2 = Vector2.ZERO
var _resizing: bool = false
var _resize_start_mouse: Vector2 = Vector2.ZERO
var _resize_start_size: Vector2 = Vector2.ZERO
var _resize_handle: TextureRect

@export var drag_enabled: bool = true:
    set(value):
        drag_enabled = value
        queue_redraw()

@export var resize_enabled: bool = true:
    set(value):
        resize_enabled = value
        _update_resize_handle()
        queue_redraw()

@export var window_decorations: bool = true:
    set(value):
        window_decorations = value
        queue_sort()
        _update_resize_handle()
        queue_redraw()

@export var allow_close: bool = true:
    set(value):
        allow_close = value
        queue_redraw()

@export var title: String = "":
    set(value):
        title = value
        queue_redraw()


func _enter_tree() -> void:
    _ensure_resize_handle()


func _ready() -> void:
    top_level = true
    clip_contents = true
    if custom_minimum_size == Vector2.ZERO:
        custom_minimum_size = DEFAULT_MINIMUM_SIZE
    _refresh_window_state()
    call_deferred("_refresh_window_state")


func _exit_tree() -> void:
    if _resize_handle != null:
        _resize_handle.queue_free()
        _resize_handle = null


func _notification(what: int) -> void:
    if what == NOTIFICATION_SORT_CHILDREN:
        _layout_children()
        return
    if what == NOTIFICATION_VISIBILITY_CHANGED:
        # a hidden window does nothing: its content's _process and Timers stop with it (the
        # Diagnostics windows read the vehicle's state dump ten times a second)
        if not Engine.is_editor_hint():
            process_mode = Node.PROCESS_MODE_INHERIT if visible else Node.PROCESS_MODE_DISABLED
        _update_resize_handle()
        queue_redraw()
        return
    if what == NOTIFICATION_RESIZED:
        queue_redraw()
        _update_resize_handle_position()
        return
    # the title is drawn, not held by a Label, so it is translated here
    if what == NOTIFICATION_TRANSLATION_CHANGED:
        queue_redraw()


func _process(_delta: float) -> void:
    _update_resize_handle_position()


func _draw() -> void:
    var frame_rect: Rect2 = Rect2(Vector2.ZERO, size)
    var border_color: Color = Color(0.160034, 0.223886, 0.359375, 1.0)
    var body_color: Color = Color(0.437927, 0.546875, 0.444736, 1.0)

    draw_rect(frame_rect, body_color, true)
    draw_rect(frame_rect, border_color, false, BORDER_WIDTH)

    if not window_decorations:
        return

    var header_rect: Rect2 = _get_header_rect()
    var header_color: Color = Color(0.14713949, 0.46044487, 0.34612492, 0.8901961)
    var close_color: Color = Color(0.85, 0.9, 0.88, 1.0)
    draw_rect(header_rect, header_color, true)
    if allow_close:
        var close_rect: Rect2 = _get_close_button_rect()
        draw_rect(close_rect, Color(0.0, 0.0, 0.0, 0.12), true)
        draw_line(close_rect.position + Vector2(4.0, 4.0), close_rect.end - Vector2(4.0, 4.0), close_color, CLOSE_BUTTON_STROKE)
        draw_line(
            Vector2(close_rect.position.x + 4.0, close_rect.end.y - 4.0),
            Vector2(close_rect.end.x - 4.0, close_rect.position.y + 4.0),
            close_color,
            CLOSE_BUTTON_STROKE
        )

    if not title:
        return

    var font: Font = get_theme_default_font()
    var font_size: int = get_theme_default_font_size()
    if font == null:
        return
    draw_string(
        font,
        Vector2(TITLE_MARGIN_X, TITLE_BASELINE_OFFSET),
        atr(title),
        HORIZONTAL_ALIGNMENT_LEFT,
        -1.0,
        font_size
    )


func _get_minimum_size() -> Vector2:
    var minimum_size: Vector2 = DEFAULT_MINIMUM_SIZE
    var content_minimum_size: Vector2 = Vector2.ZERO

    for child: Node in get_children():
        if _is_managed_child(child):
            continue
        if child is Control:
            var child_control: Control = child as Control
            content_minimum_size = content_minimum_size.max(child_control.get_combined_minimum_size())

    content_minimum_size.x += CONTENT_PADDING * 2.0
    content_minimum_size.y += CONTENT_PADDING * 2.0
    if window_decorations:
        content_minimum_size.y += HEADER_HEIGHT

    return minimum_size.max(content_minimum_size)


func _is_managed_child(child: Node) -> bool:
    return child == _resize_handle


func _get_header_rect() -> Rect2:
    if not window_decorations:
        return Rect2(Vector2.ZERO, Vector2.ZERO)
    return Rect2(Vector2.ZERO, Vector2(size.x, HEADER_HEIGHT))


func _get_close_button_rect() -> Rect2:
    return Rect2(
        Vector2(size.x - CLOSE_BUTTON_MARGIN - CLOSE_BUTTON_SIZE, CLOSE_BUTTON_MARGIN),
        Vector2(CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE)
    )


func _get_content_rect() -> Rect2:
    var top_offset: float = HEADER_HEIGHT if window_decorations else 0.0
    var content_position: Vector2 = Vector2(CONTENT_PADDING, top_offset + CONTENT_PADDING)
    var content_size: Vector2 = Vector2(
        maxf(size.x - CONTENT_PADDING * 2.0, 0.0),
        maxf(size.y - top_offset - CONTENT_PADDING * 2.0, 0.0)
    )
    return Rect2(content_position, content_size)


func _layout_children() -> void:
    if not is_node_ready():
        return

    var content_rect: Rect2 = _get_content_rect()
    for child: Node in get_children():
        if _is_managed_child(child):
            continue
        if child is Control:
            var child_control: Control = child as Control
            child_control.clip_contents = true
            fit_child_in_rect(child_control, content_rect)


func _bring_to_front() -> void:
    move_to_front()
    _update_resize_handle_position()


func _refresh_window_state() -> void:
    if not is_node_ready():
        return

    # a window stretched by its anchors (the top bar) takes its size from them
    if anchor_left == anchor_right and anchor_top == anchor_bottom:
        var minimum_size: Vector2 = get_combined_minimum_size()
        if size == Vector2.ZERO:
            size = minimum_size
        else:
            size = Vector2(
                maxf(size.x, minimum_size.x),
                maxf(size.y, minimum_size.y)
            )

    _update_resize_handle()
    queue_sort()
    queue_redraw()


func _update_resize_handle() -> void:
    if not is_node_ready():
        return

    var resize_handle: Control = _resize_handle
    if resize_handle == null:
        return

    resize_handle.visible = window_decorations and resize_enabled
    _update_resize_handle_position()


func _update_resize_handle_position() -> void:
    if not is_node_ready():
        return

    var handle: Control = _resize_handle
    if handle == null or not handle.visible:
        return

    handle.size = handle.custom_minimum_size
    handle.global_position = global_position + size - handle.size


func _input(event: InputEvent) -> void:
    if _resizing:
        if event is InputEventMouseMotion:
            var mouse_motion: InputEventMouseMotion = event as InputEventMouseMotion
            var requested_size: Vector2 = _resize_start_size + (mouse_motion.global_position - _resize_start_mouse)
            size = Vector2(
                maxf(requested_size.x, custom_minimum_size.x),
                maxf(requested_size.y, custom_minimum_size.y)
            )
            queue_sort()
            queue_redraw()
            _update_resize_handle_position()
            get_viewport().set_input_as_handled()
            return
        if event is InputEventMouseButton:
            var mouse_button: InputEventMouseButton = event as InputEventMouseButton
            if not mouse_button.pressed and mouse_button.button_index == MOUSE_BUTTON_LEFT:
                _resizing = false
                get_viewport().set_input_as_handled()
                return

    if window_decorations and drag_enabled and _dragging:
        if event is InputEventMouseMotion:
            var mouse_motion: InputEventMouseMotion = event as InputEventMouseMotion
            global_position = mouse_motion.global_position - _drag_grab_offset
            get_viewport().set_input_as_handled()
            return
        if event is InputEventMouseButton:
            var mouse_button: InputEventMouseButton = event as InputEventMouseButton
            if not mouse_button.pressed and mouse_button.button_index == MOUSE_BUTTON_LEFT:
                _dragging = false
                get_viewport().set_input_as_handled()
                return

    return


func _gui_input(event: InputEvent) -> void:
    if not event is InputEventMouseButton:
        return

    var mouse_button: InputEventMouseButton = event as InputEventMouseButton
    if mouse_button.button_index != MOUSE_BUTTON_LEFT or not mouse_button.pressed:
        return

    _bring_to_front()
    if window_decorations and allow_close and _get_close_button_rect().has_point(mouse_button.position):
        visible = false
        get_viewport().set_input_as_handled()
        return
    if not window_decorations or not drag_enabled:
        return
    if not _get_header_rect().has_point(mouse_button.position):
        return

    _dragging = true
    _drag_grab_offset = mouse_button.global_position - global_position
    get_viewport().set_input_as_handled()


func _on_resize_handle_gui_input(event: InputEvent) -> void:
    if not window_decorations or not resize_enabled:
        return

    if event is InputEventMouseButton:
        var mouse_button: InputEventMouseButton = event as InputEventMouseButton
        if mouse_button.button_index != MOUSE_BUTTON_LEFT:
            return
        if mouse_button.pressed:
            _bring_to_front()
            _resizing = true
            _dragging = false
            _resize_start_mouse = mouse_button.global_position
            _resize_start_size = size
        else:
            _resizing = false
        get_viewport().set_input_as_handled()


func _ensure_resize_handle() -> void:
    if _resize_handle != null:
        return

    _resize_handle = TextureRect.new()
    _resize_handle.name = "ResizeHandle"
    _resize_handle.top_level = true
    _resize_handle.visible = false
    _resize_handle.custom_minimum_size = RESIZE_HANDLE_SIZE
    _resize_handle.mouse_filter = Control.MOUSE_FILTER_STOP
    _resize_handle.mouse_default_cursor_shape = Control.CURSOR_FDIAGSIZE
    _resize_handle.texture = RESIZE_HANDLE_TEXTURE
    _resize_handle.expand_mode = TextureRect.EXPAND_FIT_WIDTH_PROPORTIONAL
    _resize_handle.stretch_mode = TextureRect.STRETCH_SCALE
    add_child(_resize_handle, false, INTERNAL_MODE_FRONT)
    _resize_handle.gui_input.connect(_on_resize_handle_gui_input)
