class_name ActionShortcut

## A menu shortcut of an input action - the menu shows its key, and matches it exactly (F2 is not
## Shift+F2)
static func create(action: StringName) -> Shortcut:
    var event: InputEventAction = InputEventAction.new()
    event.action = action
    var shortcut: Shortcut = Shortcut.new()
    shortcut.events = [event]
    return shortcut
