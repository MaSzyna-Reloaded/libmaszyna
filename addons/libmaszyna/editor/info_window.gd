@tool
extends Window

func _ready():
    $VBoxContainer/OKButton.pressed.connect(hide)
    close_requested.connect(hide)

func show_message(message: String):
    $VBoxContainer/Label.text = message
    popup_centered()
