extends MaszynaGutTest

## The emitter template grammar of data/smokesource_*.txt (particles.cpp:21-120) and the values
## E3DRenderingServer derives from it. The fixture is a copy of a real template - the game
## directory is not read.

const FIXTURE:String = "res://tests/fixtures/test_smokesource.txt"
const ATLAS:String = "res://vfx/smoke_atlas.png"

var _previous_mode:int = 0
var _previous_atlas:String = ""
var _previous_dynamic_density:float = 1.0
var _previous_static_density:float = 1.0
var _previous_dynamic_lifetime:float = 1.0
var _previous_static_lifetime:float = 0.5


func before_each() -> void:
    _previous_mode = ProjectSettings.get_setting(SmokeSourceLibrary.GENERATOR_MODE_SETTING, 0)
    _previous_atlas = ProjectSettings.get_setting(SmokeSourceLibrary.ATLAS_SETTING, "")
    _previous_dynamic_density = ProjectSettings.get_setting(SmokeSourceLibrary.DENSITY_DYNAMIC_SETTING, 1.0)
    _previous_static_density = ProjectSettings.get_setting(SmokeSourceLibrary.DENSITY_STATIC_SETTING, 1.0)
    _previous_dynamic_lifetime = ProjectSettings.get_setting(SmokeSourceLibrary.LIFETIME_DYNAMIC_SETTING, 1.0)
    _previous_static_lifetime = ProjectSettings.get_setting(SmokeSourceLibrary.LIFETIME_STATIC_SETTING, 0.5)


func after_each() -> void:
    ProjectSettings.set_setting(SmokeSourceLibrary.GENERATOR_MODE_SETTING, _previous_mode)
    ProjectSettings.set_setting(SmokeSourceLibrary.ATLAS_SETTING, _previous_atlas)
    ProjectSettings.set_setting(SmokeSourceLibrary.DENSITY_DYNAMIC_SETTING, _previous_dynamic_density)
    ProjectSettings.set_setting(SmokeSourceLibrary.DENSITY_STATIC_SETTING, _previous_static_density)
    ProjectSettings.set_setting(SmokeSourceLibrary.LIFETIME_DYNAMIC_SETTING, _previous_dynamic_lifetime)
    ProjectSettings.set_setting(SmokeSourceLibrary.LIFETIME_STATIC_SETTING, _previous_static_lifetime)


func _parse() -> SmokeSource:
    return SmokeSourceLibrary.parse_file(FIXTURE)


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


func test_derives_the_lifetime_and_the_particle_budget() -> void:
    var source:SmokeSource = _parse()

    # a particle dies when its opacity reaches zero (particles.cpp:132)
    assert_almost_eq(source.get_particle_lifetime(), 4.0, 0.001, "0.6 opacity faded at 0.15 per second")
    assert_eq(source.get_particle_amount(500, 1.0, 4.0), 120, "30 particles per second over one lifetime")
    assert_eq(source.get_particle_amount(64, 1.0, 4.0), 64, "the budget is capped")


func test_terminal_size_follows_the_linear_growth() -> void:
    var source:SmokeSource = _parse()

    # mean initial size 0.4, growing by 0.8 per second over 4 seconds, under the 40.0 limit
    assert_almost_eq(source.get_terminal_size(4.0), 3.6, 0.001, "size grows linearly over the lifetime")
    assert_almost_eq(source.get_terminal_size(2.0), 2.0, 0.001, "half the lifetime grows half as far")



func test_modern_mode_draws_the_particles_with_the_flipbook() -> void:
    ProjectSettings.set_setting(
        SmokeSourceLibrary.GENERATOR_MODE_SETTING, SmokeSourceLibrary.GeneratorMode.MODERN)
    ProjectSettings.set_setting(SmokeSourceLibrary.ATLAS_SETTING, ATLAS)

    var atlas:Texture2D = SmokeSourceLibrary.get_atlas()

    assert_not_null(atlas, "Modern mode draws the particles with the project's own atlas")
    var frames:Vector2i = ProjectSettings.get_setting(SmokeSourceLibrary.ATLAS_FRAMES_SETTING, Vector2i.ONE)
    assert_eq(atlas.get_width() % frames.x, 0, "The atlas splits into whole columns")
    assert_eq(atlas.get_height() % frames.y, 0, "The atlas splits into whole rows")


func test_original_mode_keeps_the_single_sprite() -> void:
    ProjectSettings.set_setting(
        SmokeSourceLibrary.GENERATOR_MODE_SETTING, SmokeSourceLibrary.GeneratorMode.ORIGINAL)
    ProjectSettings.set_setting(SmokeSourceLibrary.ATLAS_SETTING, ATLAS)

    assert_null(SmokeSourceLibrary.get_atlas(), "Original mode uses no flipbook at all")


func test_a_vehicle_and_the_scenery_take_their_own_density() -> void:
    ProjectSettings.set_setting(SmokeSourceLibrary.DENSITY_DYNAMIC_SETTING, 3.0)
    ProjectSettings.set_setting(SmokeSourceLibrary.DENSITY_STATIC_SETTING, 1.0)

    assert_almost_eq(
        SmokeSourceLibrary.get_density(E3DRenderingServer.INSTANCE_KIND_DYNAMIC), 3.0, 0.001,
        "A vehicle smokes at the vehicle density")
    assert_almost_eq(
        SmokeSourceLibrary.get_density(E3DRenderingServer.INSTANCE_KIND_STATIC), 1.0, 0.001,
        "A chimney smokes at the scenery density")


func test_the_density_multiplies_the_particle_budget() -> void:
    var source:SmokeSource = _parse()

    # the pool has to grow with the rate, or the emitter runs out of slots and spawns less
    assert_eq(source.get_particle_amount(500, 1.0, 4.0), 120, "30 particles per second over one lifetime")
    assert_eq(source.get_particle_amount(500, 3.0, 4.0), 360, "Three times as dense needs three times the pool")


func test_a_static_emitter_is_given_a_shorter_lifetime() -> void:
    ProjectSettings.set_setting(SmokeSourceLibrary.LIFETIME_DYNAMIC_SETTING, 1.0)
    ProjectSettings.set_setting(SmokeSourceLibrary.LIFETIME_STATIC_SETTING, 0.5)
    var source:SmokeSource = _parse()

    assert_almost_eq(
        SmokeSourceLibrary.get_lifetime(source, E3DRenderingServer.INSTANCE_KIND_DYNAMIC), 4.0, 0.001,
        "A vehicle keeps the template's own lifetime")
    assert_almost_eq(
        SmokeSourceLibrary.get_lifetime(source, E3DRenderingServer.INSTANCE_KIND_STATIC), 2.0, 0.001,
        "A static emitter is given half of it")


func test_each_kind_has_its_own_particle_budget() -> void:
    assert_eq(
        SmokeSourceLibrary.get_max_particles(E3DRenderingServer.INSTANCE_KIND_DYNAMIC),
        ProjectSettings.get_setting(SmokeSourceLibrary.MAX_PARTICLES_DYNAMIC_SETTING,
            SmokeSourceLibrary.DEFAULT_MAX_PARTICLES_DYNAMIC),
        "A vehicle draws on the dynamic budget")
    assert_eq(
        SmokeSourceLibrary.get_max_particles(E3DRenderingServer.INSTANCE_KIND_STATIC),
        ProjectSettings.get_setting(SmokeSourceLibrary.MAX_PARTICLES_STATIC_SETTING,
            SmokeSourceLibrary.DEFAULT_MAX_PARTICLES_STATIC),
        "A static emitter draws on the static one")
