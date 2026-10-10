class_name InputEventNames

## A key or a pad's button named for the player to read: a punctuation key by its sign and a
## numpad key by its sign or digit ("Kp -", not Godot's "Kp Subtract"), with the modifiers before it
## ("Ctrl+Shift+W"). Every place that shows a key names it here.

## Godot's names of the keys a player reads better as their signs (OS.get_keycode_string())
const KEY_NAMES:Dictionary[Key, String] = {
    KEY_APOSTROPHE: "'",
    KEY_QUOTELEFT: "`",
    KEY_BACKSLASH: "\\",
    KEY_BRACKETLEFT: "[",
    KEY_BRACKETRIGHT: "]",
    KEY_COMMA: ",",
    KEY_EQUAL: "=",
    KEY_MINUS: "-",
    KEY_PERIOD: ".",
    KEY_SEMICOLON: ";",
    KEY_SLASH: "/",
    KEY_KP_ADD: "Kp +",
    KEY_KP_SUBTRACT: "Kp -",
    KEY_KP_MULTIPLY: "Kp *",
    KEY_KP_DIVIDE: "Kp /",
    KEY_KP_PERIOD: "Kp .",
    KEY_PAGEUP: "PgUp",
    KEY_PAGEDOWN: "PgDn",
}


## `key` (with its modifier mask, as InputEventKey.get_keycode_with_modifiers() gives it)
static func key_name(key:Key) -> String:
    var code:Key = key & KEY_CODE_MASK
    var modifiers:String = OS.get_keycode_string(key & KEY_MODIFIER_MASK)
    var name:String = KEY_NAMES.get(code, OS.get_keycode_string(code))
    return modifiers + "+" + name if modifiers else name


## `event` - a key by its key code (its physical one when it has none), a pad's button by its
## number
static func event_name(event:InputEvent) -> String:
    if event is InputEventKey:
        var key:InputEventKey = event as InputEventKey
        return key_name(key.get_keycode_with_modifiers() if key.keycode else key.get_physical_keycode_with_modifiers())
    # as_text() names the button on every pad ("Joypad Button 0 (Bottom Action, Sony Cross, ...)"),
    # wider than a window, which cannot be narrower than its widest row
    if event is InputEventJoypadButton:
        return "Joypad Button %d" % (event as InputEventJoypadButton).button_index
    return event.as_text()
