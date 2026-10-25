extends MaszynaGutTest

## The emitter template grammar of data/smokesource_*.txt (particles.cpp:21-120) and the sprite
## MaszynaE3DRenderingServer's emitters are drawn with. The fixture is a copy of a real template - the
## game directory is not read.

const FIXTURE:String = "res://tests/fixtures/test_smokesource.txt"
const ATLAS:String = "res://vfx/smoke_atlas.png"
const ATLAS_SETTING:String = "reloaded/smoke/modern/atlas"

var _previous_mode:int = 0
var _previous_atlas:String = ""


func before_each() -> void:
    _previous_mode = ProjectSettings.get_setting(MaszynaSmokeSourceLibrary.GENERATOR_MODE_SETTING, 0)
    _previous_atlas = ProjectSettings.get_setting(ATLAS_SETTING, "")


func after_each() -> void:
    ProjectSettings.set_setting(MaszynaSmokeSourceLibrary.GENERATOR_MODE_SETTING, _previous_mode)
    ProjectSettings.set_setting(ATLAS_SETTING, _previous_atlas)


func _parse() -> SmokeSource:
    return MaszynaSmokeSourceLibrary.parse_file(FIXTURE)


func test_parses_every_block_of_a_template() -> void:
    var source:SmokeSource = _parse()

    assert_not_null(source, "A readable template should parse")
    assert_almost_eq(source.spawn_rate, 30.0, 0.001, "spawn_rate is a top level key")
    assert_almost_eq(source.inclination_max, 15.0, 0.001, "max_inclination comes from initializer:")
    assert_almost_eq(source.velocity_min, 0.25, 0.001, "min_velocity comes from initializer:")
    assert_almost_eq(source.velocity_max, 1.7, 0.001, "max_velocity comes from initializer:")
    assert_almost_eq(source.size_min, 0.2, 0.001, "min_size comes from initializer:")
    assert_almost_eq(source.opacity_max, 0.6, 0.001, "max_opacity comes from initializer:")
    assert_eq(source.color, Color8(50, 56, 59), "color: is a 0-255 RGB triple")
    assert_almost_eq(source.size_step, 0.8, 0.001, "size_change: has its own step")
    assert_almost_eq(source.size_limit_max, 40.0, 0.001, "size_change: has its own max")
    assert_almost_eq(source.opacity_step, -0.15, 0.001, "opacity_change: step is the fade")


func test_modern_mode_draws_the_particles_with_the_core_flipbook() -> void:
    ProjectSettings.set_setting(
        MaszynaSmokeSourceLibrary.GENERATOR_MODE_SETTING, MaszynaSmokeSourceLibrary.GeneratorMode.MODERN)
    ProjectSettings.set_setting(ATLAS_SETTING, ATLAS)

    assert_true(MaszynaSmokeSourceLibrary.uses_atlas(), "Modern mode draws the particles with the core's atlas")


func test_original_mode_keeps_the_single_sprite() -> void:
    ProjectSettings.set_setting(
        MaszynaSmokeSourceLibrary.GENERATOR_MODE_SETTING, MaszynaSmokeSourceLibrary.GeneratorMode.ORIGINAL)
    ProjectSettings.set_setting(ATLAS_SETTING, ATLAS)

    assert_false(MaszynaSmokeSourceLibrary.uses_atlas(), "Original mode uses no flipbook at all")
