extends GutTest

## A hidden HUD window does nothing: its content's _process and Timers stop with it, and run again
## when it is shown (the Diagnostics windows read the vehicle's state dump ten times a second)


func test_a_hidden_window_stops_its_content() -> void:
    var window:HUDWindow = HUDWindow.new()
    var refresh:Timer = Timer.new()
    window.add_child(refresh)
    add_child_autofree(window)

    window.visible = false
    assert_false(refresh.can_process(), "hidden: its timer stands")

    window.visible = true
    assert_true(refresh.can_process(), "shown: its timer runs again")
