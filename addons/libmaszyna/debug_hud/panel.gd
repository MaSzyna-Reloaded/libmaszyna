@tool
extends Container
class_name DebugPanel

var _dirty: bool = false
var _filter: String = ""

@export var text: String:
    set(x):
        _dirty = true
        text = x

@export var title: String:
    set(x):
        _dirty = true
        title = x


func _process(_delta: float) -> void:
    if _dirty:
        _dirty = false
        $%Title.text = title
        # a title cut short by the panel's width is read whole under the mouse
        $%Title.tooltip_text = title
        var final_text: String = text
        if _filter:
            var _tokens: PackedStringArray = _filter.split(" ")
            var _f: Array[String] = []
            for line: String in text.split("\n"):
                var _matched_count: int = 0
                for _t: String in _tokens:
                    if not _t or line.match("*" + _t + "*"):
                        _matched_count += 1
                if _tokens.size() == _matched_count:
                    _f.append(line)

            final_text = "\n".join(_f)
        $DebugPanel/RichTextLabel.text = final_text


func _ready() -> void:
    _dirty = true


func _on_filter_text_changed(new_text: String) -> void:
    _filter = new_text


func _on_clear_button_up() -> void:
    _filter = ""
    $%Filter.text = ""


func _try_release_filter_focus(event: InputEvent) -> void:
    if not event is InputEventMouseButton:
        return
    var mouse_button: InputEventMouseButton = event as InputEventMouseButton
    if not mouse_button.pressed or mouse_button.button_index != MOUSE_BUTTON_LEFT:
        return

    var filter: Control = $%Filter as Control
    if not filter.has_focus():
        return

    if not filter.get_global_rect().has_point(mouse_button.global_position):
        filter.release_focus()


func _input(event: InputEvent) -> void:
    _try_release_filter_focus(event)
