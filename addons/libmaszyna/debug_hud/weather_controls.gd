extends HFlowContainer

## Weather and time controls driving MaszynaEnvironmentNode, content of the "Weather and Time" HUD window
## (ported from forest-test-scene ui/WeatherControlsCanvas.gd).

## How often the running clock is read back. The environment announces every other change, so
## this is the only thing left that has to be looked at repeatedly - and a label showing hours and
## minutes gains nothing from being rewritten 60 times a second.
const CLOCK_REFRESH_INTERVAL: float = 0.1
## The time speeds the slider steps through. 0 is not one of them - a slider nudged to its end must
## not stop the world; stopping it is the pause button's
const TIME_SCALE_STEPS: Array[float] = [0.1, 0.25, 0.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 20.0]

## The environment of the world being shown (attach_environment()); null in the menu, where the HUD
## and this window are hidden
var _environment_node: MaszynaEnvironmentNode = null
var _time_slider_dragging: bool = false
var _dirty: bool = true
var _refresh_timer: Timer

@onready var _wind_value_label: Label = %WindValueLabel
@onready var _rain_value_label: Label = %RainValueLabel
@onready var _cloud_value_label: Label = %CloudValueLabel
@onready var _fog_density_value_label: Label = %FogDensityValueLabel
@onready var _fog_distance_value_label: Label = %FogDistanceValueLabel
@onready var _time_value_label: Label = %TimeValueLabel
@onready var _day_value_label: Label = %DayValueLabel
@onready var _month_value_label: Label = %MonthValueLabel
@onready var _year_value_label: Label = %YearValueLabel
@onready var _day_slider: HSlider = %DaySlider
@onready var _month_slider: HSlider = %MonthSlider
@onready var _year_slider: HSlider = %YearSlider
@onready var _time_scale_value_label: Label = %TimeScaleValueLabel
@onready var _wind_strength_slider: HSlider = %WindSlider
@onready var _wind_direction_slider: HSlider = %WindDirectionSlider
@onready var _wind_direction_value_label: Label = %WindDirectionValueLabel
@onready var _rain_slider: HSlider = %RainSlider
@onready var _cloud_slider: HSlider = %CloudSlider
@onready var _fog_density_slider: HSlider = %FogDensitySlider
@onready var _fog_distance_slider: HSlider = %FogDistanceSlider
@onready var _time_slider: HSlider = %TimeSlider
@onready var _time_scale_slider: HSlider = %TimeScaleSlider
@onready var _pause_button: Button = %PauseButton
@onready var _system_time_check_box: CheckBox = %SystemTimeCheckBox
@onready var _simulation_time_label: Label = %SimulationTime


func _ready() -> void:
    _wind_strength_slider.value_changed.connect(_on_wind_strength_changed)
    _wind_direction_slider.value_changed.connect(_on_wind_direction_changed)
    _rain_slider.value_changed.connect(_on_rain_changed)
    _cloud_slider.value_changed.connect(_on_cloud_changed)
    _fog_density_slider.value_changed.connect(_on_fog_density_changed)
    _fog_distance_slider.value_changed.connect(_on_fog_distance_changed)
    _time_slider.value_changed.connect(_on_time_changed)
    _time_slider.drag_started.connect(_on_time_drag_started)
    _time_slider.drag_ended.connect(_on_time_drag_ended)
    _time_scale_slider.max_value = TIME_SCALE_STEPS.size() - 1
    _time_scale_slider.tick_count = TIME_SCALE_STEPS.size()
    _time_scale_slider.value_changed.connect(_on_time_scale_changed)
    _pause_button.set_pressed_no_signal(SimulationServer.simulation_is_paused())
    SimulationServer.simulation_paused.connect(_pause_button.set_pressed_no_signal.bind(true))
    SimulationServer.simulation_unpaused.connect(_pause_button.set_pressed_no_signal.bind(false))
    SimulationServer.simulation_speed_changed.connect(_on_simulation_speed_changed)
    _on_simulation_speed_changed()
    _day_slider.value_changed.connect(_on_day_changed)
    _month_slider.value_changed.connect(_on_month_changed)
    _year_slider.value_changed.connect(_on_year_changed)
    _system_time_check_box.toggled.connect(_on_system_time_toggled)
    _refresh_timer = Timer.new()
    _refresh_timer.wait_time = CLOCK_REFRESH_INTERVAL
    add_child(_refresh_timer)
    _refresh_timer.timeout.connect(_on_refresh_timeout)
    _refresh_timer.start()


func _exit_tree() -> void:
    SimulationServer.simulation_paused.disconnect(_pause_button.set_pressed_no_signal.bind(true))
    SimulationServer.simulation_unpaused.disconnect(_pause_button.set_pressed_no_signal.bind(false))
    SimulationServer.simulation_speed_changed.disconnect(_on_simulation_speed_changed)


## The world made for a scenery brings its environment, and takes it with it to the menu (null)
func attach_environment(environment: MaszynaEnvironmentNode) -> void:
    if _environment_node:
        _environment_node.configuration_changed.disconnect(_on_environment_configuration_changed)
    _environment_node = environment
    if _environment_node:
        _environment_node.configuration_changed.connect(_on_environment_configuration_changed)
        _dirty = true


## The environment applied a change, so everything the window shows is out of date.
func _on_environment_configuration_changed() -> void:
    _dirty = true


func _on_refresh_timeout() -> void:
    if not is_visible_in_tree():
        return
    if _dirty:
        _dirty = false
        _process_dirty()
        return
    _refresh_clock()


## Mirrors the whole environment node, so presets and changes made elsewhere show up in the
## controls.
func _process_dirty() -> void:
    # the system clock drives the time and the date, the sliders only show them - and it runs at
    # its own pace, so the time speed means nothing either
    var editable: bool = not _environment_node.use_system_time
    _system_time_check_box.set_pressed_no_signal(_environment_node.use_system_time)
    _time_slider.editable = editable
    _time_scale_slider.editable = editable
    _day_slider.editable = editable
    _month_slider.editable = editable
    _year_slider.editable = editable
    _refresh_clock()
    _day_slider.set_value_no_signal(_environment_node.day)
    _day_value_label.text = str(_environment_node.day)
    _month_slider.set_value_no_signal(_environment_node.month)
    _month_value_label.text = str(_environment_node.month)
    _year_slider.set_value_no_signal(_environment_node.year)
    _year_value_label.text = str(_environment_node.year)
    _wind_strength_slider.set_value_no_signal(_environment_node.wind_strength)
    _wind_value_label.text = _format_percent(_environment_node.wind_strength)
    _wind_direction_slider.set_value_no_signal(_environment_node.wind_direction)
    _wind_direction_value_label.text = _format_degrees(_environment_node.wind_direction)
    _rain_slider.set_value_no_signal(_environment_node.precipitation)
    _rain_value_label.text = _format_percent(_environment_node.precipitation)
    _cloud_slider.set_value_no_signal(_environment_node.cloudiness)
    _cloud_value_label.text = _format_percent(_environment_node.cloudiness)
    _fog_density_slider.set_value_no_signal(_environment_node.fog_density)
    _fog_density_value_label.text = _format_percent(_environment_node.fog_density)
    _fog_distance_slider.set_value_no_signal(_environment_node.fog_distance)
    _fog_distance_value_label.text = _format_meters(_environment_node.fog_distance)


## The step nearest the speed - one set elsewhere (the speed panel, a script) need not be a step
func _on_simulation_speed_changed() -> void:
    var speed: float = SimulationServer.simulation_speed
    var step: int = 0
    for index: int in TIME_SCALE_STEPS.size():
        if absf(TIME_SCALE_STEPS[index] - speed) < absf(TIME_SCALE_STEPS[step] - speed):
            step = index
    _time_scale_slider.set_value_no_signal(step)
    _time_scale_value_label.text = "%sx" % ("%.2f" % speed).rstrip("0").rstrip(".")


## The only part of the state that moves on its own, so the only part read on a timer.
func _refresh_clock() -> void:
    if _time_slider_dragging:
        return
    _time_slider.set_value_no_signal(_environment_node.current_time)
    _time_value_label.text = _format_time_label(_environment_node.current_time)


## The date and the time the simulation is at, down to the second - read once a second by the
## scene's ClockTimer.
func _on_clock_timer_timeout() -> void:
    if not is_visible_in_tree():
        return
    var seconds_of_day: int = int(wrapf(_environment_node.current_time, 0.0, LibMaszynaUnits.HOURS_PER_DAY)
            * LibMaszynaUnits.SECONDS_PER_HOUR)
    _simulation_time_label.text = "%02d.%02d.%04d %02d:%02d:%02d" % [
            _environment_node.day, _environment_node.month, _environment_node.year,
            seconds_of_day / LibMaszynaUnits.SECONDS_PER_HOUR,
            seconds_of_day / LibMaszynaUnits.SECONDS_PER_MINUTE % LibMaszynaUnits.MINUTES_PER_HOUR,
            seconds_of_day % LibMaszynaUnits.SECONDS_PER_MINUTE]


func _on_wind_strength_changed(value: float) -> void:
    _wind_value_label.text = _format_percent(value)
    _environment_node.wind_strength = value


func _on_wind_direction_changed(value: float) -> void:
    _wind_direction_value_label.text = _format_degrees(value)
    _environment_node.wind_direction = value


func _on_rain_changed(value: float) -> void:
    _rain_value_label.text = _format_percent(value)
    _environment_node.precipitation = value


func _on_cloud_changed(value: float) -> void:
    _cloud_value_label.text = _format_percent(value)
    _environment_node.cloudiness = value


func _on_fog_density_changed(value: float) -> void:
    _fog_density_value_label.text = _format_percent(value)
    _environment_node.fog_density = value


func _on_fog_distance_changed(value: float) -> void:
    _fog_distance_value_label.text = _format_meters(value)
    _environment_node.fog_distance = value


func _on_time_changed(value: float) -> void:
    _time_value_label.text = _format_time_label(value)
    _environment_node.current_time = value


# set_date() normalizes the date (e.g. 31.02 -> 03.03), the bar follows it in _process.
func _on_day_changed(value: float) -> void:
    _environment_node.set_date(_environment_node.year, _environment_node.month, int(value))


func _on_month_changed(value: float) -> void:
    _environment_node.set_date(_environment_node.year, int(value), _environment_node.day)


func _on_year_changed(value: float) -> void:
    _environment_node.set_date(int(value), _environment_node.month, _environment_node.day)


func _on_time_drag_started() -> void:
    _time_slider_dragging = true


func _on_time_drag_ended(_value_changed: bool) -> void:
    _time_slider_dragging = false


func _on_system_time_toggled(pressed: bool) -> void:
    _environment_node.use_system_time = pressed


func _on_time_scale_changed(value: float) -> void:
    SimulationServer.simulation_speed = TIME_SCALE_STEPS[int(value)]


func _on_pause_button_toggled(toggled_on: bool) -> void:
    if toggled_on:
        SimulationServer.simulation_pause()
    else:
        SimulationServer.simulation_unpause()


func _format_percent(value: float) -> String:
    return "%d%%" % int(round(value * 100.0))


func _format_degrees(value: float) -> String:
    return "%d°" % roundi(value)


func _format_meters(value: float) -> String:
    return "%d m" % roundi(value)


func _format_time_label(value: float) -> String:
    var wrapped: float = wrapf(value, 0.0, LibMaszynaUnits.HOURS_PER_DAY)
    var hours: int = int(floor(wrapped))
    var minutes: int = int(round((wrapped - float(hours)) * LibMaszynaUnits.MINUTES_PER_HOUR))
    if minutes >= LibMaszynaUnits.MINUTES_PER_HOUR:
        hours = (hours + 1) % LibMaszynaUnits.HOURS_PER_DAY
        minutes = 0
    return "%02d:%02d" % [hours, minutes]
