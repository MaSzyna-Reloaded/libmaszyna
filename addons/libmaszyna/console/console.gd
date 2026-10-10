extends Node

# Based on https://github.com/jitspoe/godot-console (MIT)
# some errors fixed, features needs to be added

# TODO:
# - autocompletion for command arguments
# - losing input focus

var enabled := true
var enable_on_release_build := false : set = set_enable_on_release_build
var pause_enabled := false

signal console_opened
signal console_closed
signal console_toggled
signal console_unknown_command


class ConsoleCommand:
    var function : Callable
    var arguments : PackedStringArray
    var required : int
    var description : String
    func _init(in_function : Callable, in_arguments : PackedStringArray, in_required : int = 0, in_description : String = ""):
        function = in_function
        arguments = in_arguments
        required = in_required
        description = in_description


@onready var control := Control.new()
@onready var rich_label := RichTextLabel.new()
@onready var line_edit := LineEdit.new()

var console_commands := {}
var console_history := []
var console_history_index := 0
var was_paused_already := false


func _ready() -> void:
    var canvas_layer := CanvasLayer.new()
    canvas_layer.layer = 3
    add_child(canvas_layer)
    control.anchor_bottom = 1.0
    control.anchor_right = 1.0
    canvas_layer.add_child(control)
    var style := StyleBoxFlat.new()
    style.bg_color = Color("000000d7")
    rich_label.selection_enabled = true
    rich_label.context_menu_enabled = true
    rich_label.bbcode_enabled = true
    rich_label.scroll_following = true
    rich_label.anchor_right = 1.0
    rich_label.anchor_bottom = 0.5
    rich_label.add_theme_stylebox_override("normal", style)
    control.add_child(rich_label)
    rich_label.append_text(tr("Development console.") + "\n")
    line_edit.anchor_top = 0.5
    line_edit.anchor_right = 1.0
    line_edit.anchor_bottom = 0.5
    line_edit.placeholder_text = "Enter \"help\" for instructions"
    line_edit.focus_mode = Control.FOCUS_ALL
    line_edit.keep_editing_on_text_submit = true
    control.add_child(line_edit)
    line_edit.text_submitted.connect(on_text_entered)
    line_edit.text_changed.connect(on_line_edit_text_changed)
    control.visible = false
    process_mode = PROCESS_MODE_ALWAYS

    add_command("exit", quit, 0, 0, "Exit game")
    add_command("clear", clear, 0, 0, "Clear console")
    add_command("delete_history", delete_history, 0, 0, "Delete console history")
    add_command("help", commands_list, 0, 0, "Print this help")



func _input(event : InputEvent) -> void:
    if (event is InputEventKey):
        # ~ key, the project's action; it also stands as the shortcut of the HUD's menu entry, which
        # never sees the key - it is taken here first
        if (event.is_action_pressed(&"console_toggle", false, true)):
            toggle_console()
            get_tree().get_root().set_input_as_handled()
        elif (event.physical_keycode == KEY_QUOTELEFT and event.is_command_or_control_pressed()): # Toggles console size or opens big console.
            if (event.pressed):
                if (control.visible):
                    toggle_size()
                else:
                    toggle_console()
                    toggle_size()
            get_tree().get_root().set_input_as_handled()
        elif (event.get_physical_keycode_with_modifiers() == KEY_ESCAPE && control.visible): # Disable console on ESC
            if (event.pressed):
                toggle_console()
                get_tree().get_root().set_input_as_handled()
        if (control.visible and event.pressed):
            if (event.get_physical_keycode_with_modifiers() == KEY_UP):
                get_tree().get_root().set_input_as_handled()
                if (console_history_index > 0):
                    console_history_index -= 1
                    if (console_history_index >= 0):
                        line_edit.text = console_history[console_history_index]
                        line_edit.caret_column = line_edit.text.length()
                        reset_autocomplete()
            if (event.get_physical_keycode_with_modifiers() == KEY_DOWN):
                get_tree().get_root().set_input_as_handled()
                if (console_history_index < console_history.size()):
                    console_history_index += 1
                    if (console_history_index < console_history.size()):
                        line_edit.text = console_history[console_history_index]
                        line_edit.caret_column = line_edit.text.length()
                        reset_autocomplete()
                    else:
                        line_edit.text = ""
                        reset_autocomplete()
            if (event.get_physical_keycode_with_modifiers() == KEY_PAGEUP):
                var scroll := rich_label.get_v_scroll_bar()
                var tween := create_tween()
                tween.tween_property(scroll, "value",  scroll.value - (scroll.page - scroll.page * 0.1), 0.1)
                get_tree().get_root().set_input_as_handled()
            if (event.get_physical_keycode_with_modifiers() == KEY_PAGEDOWN):
                var scroll := rich_label.get_v_scroll_bar()
                var tween := create_tween()
                tween.tween_property(scroll, "value",  scroll.value + (scroll.page - scroll.page * 0.1), 0.1)
                get_tree().get_root().set_input_as_handled()
            if (event.get_physical_keycode_with_modifiers() == KEY_TAB):
                autocomplete()
                get_tree().get_root().set_input_as_handled()


var suggestions := []
var current_suggest := 0
var suggesting := false
func autocomplete() -> void:
    if suggesting:
        for i in range(suggestions.size()):
            if current_suggest == i:
                line_edit.text = str(suggestions[i])
                line_edit.caret_column = line_edit.text.length()
                if current_suggest == suggestions.size() - 1:
                    current_suggest = 0
                else:
                    current_suggest += 1
                return
    else:
        suggesting = true

        var sorted_commands := []
        for command in console_commands:
            sorted_commands.append(str(command))
        sorted_commands.sort()
        sorted_commands.reverse()

        var prev_index := 0
        for command in sorted_commands:
            if command.contains(line_edit.text):
                var index : int = command.find(line_edit.text)
                if index <= prev_index:
                    suggestions.push_front(command)
                else:
                    suggestions.push_back(command)
                prev_index = index
        autocomplete()


func reset_autocomplete() -> void:
    suggestions.clear()
    current_suggest = 0
    suggesting = false


func toggle_size() -> void:
    if (control.anchor_bottom == 1.0):
        control.anchor_bottom = 1.9
    else:
        control.anchor_bottom = 1.0


func disable():
    enabled = false
    toggle_console() # Ensure hidden if opened


func enable():
    enabled = true


func toggle_console() -> void:
    set_visible(!control.visible)

func set_visible(visible:bool) -> void:
    if not control or not line_edit:
        return
    if (enabled):
        control.visible = visible
    else:
        control.visible = false

    if (control.visible):
        was_paused_already = get_tree().paused
        get_tree().paused = was_paused_already || pause_enabled
        _restore_line_edit_focus()
        console_opened.emit()
        console_toggled.emit(true)
    else:
        control.anchor_bottom = 1.0
        scroll_to_bottom()
        reset_autocomplete()
        if (pause_enabled && !was_paused_already):
            get_tree().paused = false
        console_closed.emit()
        console_toggled.emit(false)


func is_visible():
    return control.visible


func scroll_to_bottom() -> void:
    var scroll: ScrollBar = rich_label.get_v_scroll_bar()
    scroll.value = scroll.max_value - scroll.page


func print_line(text : String, print_godot := false) -> void:
    if (!rich_label): # Tried to print something before the console was loaded.
        call_deferred("print_line", text)
    else:
        rich_label.append_text(text)
        rich_label.append_text("\n")
        if (print_godot):
            print(text)


func parse_line_input(text : String) -> PackedStringArray:
    var out_array : PackedStringArray
    var first_char := true
    var in_quotes := false
    var escaped := false
    var token : String
    for c in text:
        if (c == '\\'):
            escaped = true
            continue
        elif (escaped):
            if (c == 'n'):
                c = '\n'
            elif (c == 't'):
                c = '\t'
            elif (c == 'r'):
                c = '\r'
            elif (c == 'a'):
                c = '\a'
            elif (c == 'b'):
                c = '\b'
            elif (c == 'f'):
                c = '\f'
            escaped = false
        elif (c == '\"'):
            in_quotes = !in_quotes
            continue
        elif (c == ' ' || c == '\t'):
            if (!in_quotes):
                if token:
                    out_array.push_back(token)
                token = ""
                continue
        token += c
    if token:
        out_array.push_back(token)

    return out_array


const _FALSE_TOKENS := ["false", "no", "off", "0"]

## Console arguments are always plain text tokens - converted explicitly here rather than
## relying on Godot's own String->Variant call coercion, since that coercion is inconsistent:
## numeric strings ("5", "0.5") do convert correctly when calling a bound method that expects
## an int/float, but boolean strings do NOT - any non-empty string (including the literal text
## "false") coerces to true, making it impossible to type a command that turns something off.
## Every other token (including "true"/"yes"/"1") already coerces to true on its own, so only
## the false case needs explicit handling. "0" is safe to include even for commands that take
## an actual integer argument (e.g. main_controller_increase's step) - Godot's own bool->int
## Variant coercion maps false back to 0, so the end result is identical either way.
##
## Numeric tokens are still converted explicitly (rather than left as strings for Godot's own
## coercion to handle) so the int/float distinction is preserved: "5" becomes an int and "0.5"
## a float, matching whichever the target command's parameter actually expects, instead of
## collapsing every number to float.
func _coerce_argument_tokens(arguments : PackedStringArray) -> Array:
    var result : Array = []
    for arg : String in arguments:
        if arg.to_lower() in _FALSE_TOKENS:
            result.append(false)
        elif arg.is_valid_int():
            result.append(arg.to_int())
        elif arg.is_valid_float():
            result.append(arg.to_float())
        else:
            result.append(arg)
    return result


func on_text_entered(new_text : String) -> void:
    scroll_to_bottom()
    reset_autocomplete()
    line_edit.clear()



    if new_text.strip_edges():
        add_input_history(new_text)
        print_line("[i]> " + new_text + "[/i]")
        var text_split := parse_line_input(new_text)
        var text_command := text_split[0]

        if console_commands.has(text_command):
            var arguments := text_split.slice(1)

            if arguments.size() < console_commands[text_command].required:
                print_line("[color=light_coral]	ERROR:[/color] " + tr("Too few arguments! Required < %d >") % console_commands[text_command].required)
                return
            elif arguments.size() > console_commands[text_command].arguments.size():
                print_line("[color=light_coral]	ERROR:[/color] " + tr("Too many arguments! < %d > Max") % console_commands[text_command].arguments.size())
                return

            # Functions fail to call if passed the incorrect number of arguments, so fill out with blank strings.

            console_commands[text_command].function.callv(_coerce_argument_tokens(arguments))
        else:
            console_unknown_command.emit(text_command)
            print_line("[color=light_coral]	ERROR:[/color] " + tr("Command not found."))

    await get_tree().process_frame
    _restore_line_edit_focus()

func on_line_edit_text_changed(new_text : String) -> void:
    reset_autocomplete()


func add_command(command_name : String, function : Callable, arguments = [], required: int = 0, description : String = "") -> void:
    if arguments is int:
        # Legacy call using an argument number
        var param_array : PackedStringArray
        for i in range(arguments):
            param_array.append("arg_" + str(i + 1))
        console_commands[command_name] = ConsoleCommand.new(function, param_array, required, description)

    elif arguments is Array:
        # New array argument system
        var str_args : PackedStringArray
        for argument in arguments:
            str_args.append(str(argument))
        console_commands[command_name] = ConsoleCommand.new(function, str_args, required, description)


func remove_command(command_name : String) -> void:
    console_commands.erase(command_name)


func quit() -> void:
    get_tree().quit()


func clear() -> void:
    rich_label.clear()


func delete_history() -> void:
    console_history.clear()
    console_history_index = 0
    DirAccess.remove_absolute("user://console_history.txt")

func commands_list() -> void:
    var commands := []
    for command in console_commands:
        commands.append(str(command))
    commands.sort()

    for command in commands:
        var arguments_string := ""
        var description : String = tr(console_commands[command].description)
        for i in range(console_commands[command].arguments.size()):
            if i < console_commands[command].required:
                arguments_string += "  [color=cornflower_blue]<" + console_commands[command].arguments[i] + ">[/color]"
            else:
                arguments_string += "  <" + console_commands[command].arguments[i] + ">"
        rich_label.append_text("	[color=light_green]%s[/color][color=gray]%s[/color]:   %s\n" % [command, arguments_string, description])
    rich_label.append_text("\n")


func add_input_history(text : String) -> void:
    if (!console_history.size() || text != console_history.back()): # Don't add consecutive duplicates
        console_history.append(text)
    console_history_index = console_history.size()


func _enter_tree() -> void:
    var console_history_file := FileAccess.open("user://console_history.txt", FileAccess.READ)
    if (console_history_file):
        while (!console_history_file.eof_reached()):
            var line := console_history_file.get_line()
            if (line.length()):
                add_input_history(line)


func _exit_tree() -> void:
    var console_history_file := FileAccess.open("user://console_history.txt", FileAccess.WRITE)
    if (console_history_file):
        var write_index := 0
        var start_write_index := console_history.size() - 100 # Max lines to write
        for line in console_history:
            if (write_index >= start_write_index):
                console_history_file.store_line(line)
            write_index += 1


func set_enable_on_release_build(enable : bool):
    enable_on_release_build = enable
    if (!enable_on_release_build):
        if (!OS.is_debug_build()):
            disable()


func _restore_line_edit_focus() -> void:
    if not control.visible:
        return

    line_edit.grab_focus()
    line_edit.grab_click_focus()
    line_edit.edit()
