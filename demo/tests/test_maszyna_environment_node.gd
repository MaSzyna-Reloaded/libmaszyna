extends MaszynaGutTest

var _previous_season: MaszynaEnvironment.Season
var _previous_weather: MaszynaEnvironment.Weather


func before_each() -> void:
    _previous_season = MaterialManager.season
    _previous_weather = MaterialManager.weather


func after_each() -> void:
    MaterialManager.season = _previous_season
    MaterialManager.weather = _previous_weather


func test_creates_internal_environment_hierarchy() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()

    assert_true(environment_node._sky_environment is MaszynaSkyEnvironment)
    assert_true(environment_node._sky_environment is TokisanSky3DMaszynaEnvironment)
    assert_same(environment_node._sky_environment.environment_node, environment_node)
    assert_eq(environment_node.get_child_count(), 0)
    assert_eq(environment_node.get_child_count(true), 1)

    var world_environment: WorldEnvironment = (
        environment_node.get_child(0, true) as WorldEnvironment
    )
    assert_not_null(world_environment)
    assert_not_null(world_environment.environment)
    assert_eq(world_environment.get_child_count(), 0)
    assert_eq(world_environment.get_child_count(true), 4)
    assert_not_null(world_environment.get_node_or_null("SunLight") as DirectionalLight3D)
    assert_not_null(world_environment.get_node_or_null("MoonLight") as DirectionalLight3D)
    assert_not_null(world_environment.get_node_or_null("SkyDome") as SkyDome)
    assert_not_null(world_environment.get_node_or_null("TimeOfDay") as TimeOfDay)
    _assert_children_have_no_owner(environment_node)


func test_does_not_duplicate_generated_environment() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()

    environment_node._ensure_environment()
    environment_node._ensure_environment()

    assert_eq(environment_node.get_child_count(true), 1)


func test_generated_environment_is_not_packed() -> void:
    var scene_root: Node = add_child_autofree(Node.new())
    var environment_node: MaszynaEnvironmentNode = MaszynaEnvironmentNode.new()
    var packed_scene: PackedScene = PackedScene.new()

    scene_root.add_child(environment_node)
    environment_node.owner = scene_root

    assert_eq(packed_scene.pack(scene_root), OK)
    assert_eq(packed_scene.get_state().get_node_count(), 2)


func test_maps_cloudiness_to_both_cloud_layers() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var sky_dome: SkyDome = _get_tokisan_environment(environment_node).sky_dome

    environment_node.cloudiness = 0.8
    environment_node._process(0.0)

    assert_true(sky_dome.cirrus_visible)
    assert_true(sky_dome.cumulus_visible)
    assert_almost_eq(sky_dome.cirrus_coverage, 0.8, 0.000001)
    assert_almost_eq(sky_dome.cumulus_coverage, 0.88, 0.000001)

    environment_node.cloudiness = 0.0
    environment_node._process(0.0)

    assert_false(sky_dome.cirrus_visible)
    assert_false(sky_dome.cumulus_visible)


func test_full_cloudiness_hides_celestial_bodies() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var sky_dome: SkyDome = _get_tokisan_environment(environment_node).sky_dome

    environment_node.cloudiness = 0.5
    environment_node._process(0.0)

    assert_almost_eq(sky_dome.sun_disk_intensity, 30.0, 0.000001)
    assert_eq(sky_dome.moon_color, Color.WHITE)

    environment_node.cloudiness = 1.0
    environment_node._process(0.0)

    assert_almost_eq(sky_dome.sun_disk_intensity, 0.0, 0.000001)
    assert_eq(sky_dome.moon_color, Color(0.0, 0.0, 0.0, 1.0))
    assert_eq(sky_dome.starmap_color, Color(0.0, 0.0, 0.0, 1.0))
    assert_eq(sky_dome.star_field_color, Color(0.0, 0.0, 0.0, 1.0))


func test_applies_day_and_night_light_configuration() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var tokisan_environment: TokisanSky3DMaszynaEnvironment = (
        _get_tokisan_environment(environment_node)
    )

    environment_node.day_light_shadow_enabled = false
    environment_node.day_light_shadow_mode = DirectionalLight3D.SHADOW_ORTHOGONAL
    environment_node.day_light_shadow_blur = 0.5
    environment_node.day_light_shadow_opacity = 0.75
    environment_node.day_light_shadow_bias = 0.2
    environment_node.day_light_shadow_normal_bias = 1.5
    environment_node.day_light_shadow_max_distance = 250.0
    environment_node.day_light_volumetric_fog_energy = 12.5
    environment_node.night_light_shadow_enabled = false
    environment_node.night_light_shadow_mode = DirectionalLight3D.SHADOW_PARALLEL_2_SPLITS
    environment_node.night_light_shadow_blur = 1.5
    environment_node.night_light_shadow_opacity = 0.6
    environment_node.night_light_shadow_bias = 0.3
    environment_node.night_light_shadow_normal_bias = 2.5
    environment_node.night_light_shadow_max_distance = 150.0
    environment_node.night_light_volumetric_fog_energy = 8.5
    environment_node._process(0.0)

    assert_false(tokisan_environment.sun_light.shadow_enabled)
    assert_eq(
        tokisan_environment.sun_light.directional_shadow_mode,
        DirectionalLight3D.SHADOW_ORTHOGONAL
    )
    assert_almost_eq(tokisan_environment.sun_light.shadow_blur, 0.5, 0.000001)
    assert_almost_eq(tokisan_environment.sun_light.shadow_opacity, 0.75, 0.000001)
    assert_almost_eq(tokisan_environment.sun_light.shadow_bias, 0.2, 0.000001)
    assert_almost_eq(tokisan_environment.sun_light.shadow_normal_bias, 1.5, 0.000001)
    assert_almost_eq(
        tokisan_environment.sun_light.directional_shadow_max_distance, 250.0, 0.000001
    )
    assert_almost_eq(
        tokisan_environment.sun_light.light_volumetric_fog_energy, 12.5, 0.000001
    )
    assert_false(tokisan_environment.moon_light.shadow_enabled)
    assert_eq(
        tokisan_environment.moon_light.directional_shadow_mode,
        DirectionalLight3D.SHADOW_PARALLEL_2_SPLITS
    )
    assert_almost_eq(tokisan_environment.moon_light.shadow_blur, 1.5, 0.000001)
    assert_almost_eq(tokisan_environment.moon_light.shadow_opacity, 0.6, 0.000001)
    assert_almost_eq(tokisan_environment.moon_light.shadow_bias, 0.3, 0.000001)
    assert_almost_eq(tokisan_environment.moon_light.shadow_normal_bias, 2.5, 0.000001)
    assert_almost_eq(
        tokisan_environment.moon_light.directional_shadow_max_distance, 150.0, 0.000001
    )
    assert_almost_eq(
        tokisan_environment.moon_light.light_volumetric_fog_energy, 8.5, 0.000001
    )


func test_maps_fog_controls_to_depth_and_volumetric_fog() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var sky_dome: SkyDome = _get_tokisan_environment(environment_node).sky_dome

    environment_node.fog_density = 0.4
    environment_node.fog_range_start = 250.0
    environment_node.fog_range_end = 800.0
    environment_node._process(0.0)

    assert_true(environment_node._environment.fog_enabled)
    assert_almost_eq(environment_node._environment.fog_density, 0.4, 0.000001)
    assert_almost_eq(environment_node._environment.fog_depth_begin, 250.0, 0.000001)
    assert_almost_eq(environment_node._environment.fog_depth_end, 800.0, 0.000001)
    assert_almost_eq(environment_node._environment.fog_sky_affect, 0.4, 0.000001)
    assert_false(sky_dome.fog_visible)
    assert_eq(
        environment_node._environment.volumetric_fog_enabled,
        bool(UserSettings.get_setting("render", "volumetric_fog_enabled", true))
    )
    assert_almost_eq(
        environment_node._environment.volumetric_fog_density, 0.0000510275, 0.000000001
    )
    assert_almost_eq(
        environment_node._environment.volumetric_fog_length, 800.0, 0.000001
    )
    assert_almost_eq(
        environment_node._environment.volumetric_fog_sky_affect, 1.0, 0.000001
    )


func test_fog_switch_disables_all_fog_layers() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var sky_dome: SkyDome = _get_tokisan_environment(environment_node).sky_dome

    environment_node.fog_enabled = false
    environment_node._process(0.0)

    assert_false(environment_node._environment.fog_enabled)
    assert_false(environment_node._environment.volumetric_fog_enabled)
    assert_false(sky_dome.fog_visible)


func test_zero_fog_density_hides_fog_quad_without_hiding_clouds() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var sky_dome: SkyDome = _get_tokisan_environment(environment_node).sky_dome

    environment_node.fog_enabled = true
    environment_node.fog_density = 0.0
    environment_node.cloudiness = 0.5
    environment_node._process(0.0)

    assert_false(environment_node._environment.fog_enabled)
    assert_false(environment_node._environment.volumetric_fog_enabled)
    assert_false(sky_dome.fog_visible)
    assert_false(sky_dome.fog_mesh.visible)
    assert_true(sky_dome.cirrus_visible)
    assert_true(sky_dome.cumulus_visible)


func test_depth_fog_affects_sky_with_the_same_density_as_geometry() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()

    environment_node.fog_density = 0.001
    environment_node._process(0.0)

    assert_almost_eq(environment_node._environment.fog_sky_affect, 0.001, 0.000001)

    environment_node.fog_density = 0.2
    environment_node._process(0.0)

    assert_almost_eq(environment_node._environment.fog_sky_affect, 0.2, 0.000001)

    environment_node.fog_density = 1.0
    environment_node._process(0.0)

    assert_almost_eq(environment_node._environment.fog_sky_affect, 1.0, 0.000001)


func test_dense_short_range_fog_does_not_draw_tokisan_sky_overlay() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var sky_dome: SkyDome = _get_tokisan_environment(environment_node).sky_dome

    environment_node.fog_enabled = true
    environment_node.fog_density = 1.0
    environment_node.fog_range_start = 0.0
    environment_node.fog_range_end = 20.0
    environment_node.cloudiness = 1.0
    environment_node._process(0.0)

    assert_true(environment_node._environment.fog_enabled)
    assert_almost_eq(environment_node._environment.fog_density, 1.0, 0.000001)
    assert_almost_eq(environment_node._environment.fog_sky_affect, 1.0, 0.000001)
    assert_almost_eq(environment_node._environment.fog_depth_begin, 0.0, 0.000001)
    assert_almost_eq(environment_node._environment.fog_depth_end, 20.0, 0.000001)
    assert_almost_eq(
        environment_node._environment.volumetric_fog_length, 64.0, 0.000001
    )
    assert_false(sky_dome.fog_visible)
    assert_false(sky_dome.fog_mesh.visible)


func test_clamps_fog_end_after_start() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()

    environment_node.fog_range_start = 250.0
    environment_node.fog_range_end = 100.0
    environment_node._process(0.0)

    assert_almost_eq(environment_node._environment.fog_depth_end, 250.001, 0.00001)
    assert_almost_eq(
        environment_node._environment.volumetric_fog_length, 250.001, 0.00001
    )


func test_applies_time_location_and_simulation_configuration() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var tokisan_environment: TokisanSky3DMaszynaEnvironment = (
        _get_tokisan_environment(environment_node)
    )
    var time_of_day: TimeOfDay = tokisan_environment.time_of_day

    environment_node.latitude = 50.271
    environment_node.longitude = 19.04
    environment_node.timezone_offset = 2
    environment_node.simulation_speed = 100.0
    environment_node._process(0.0)

    assert_almost_eq(time_of_day.latitude, deg_to_rad(50.271), 0.000001)
    assert_almost_eq(time_of_day.longitude, deg_to_rad(19.04), 0.000001)
    assert_almost_eq(time_of_day.utc, 2.0, 0.000001)
    assert_almost_eq(time_of_day.minutes_per_day, 14.4, 0.000001)
    assert_eq(time_of_day.dome_path, NodePath("../SkyDome"))
    assert_eq(tokisan_environment.sky_dome.sun_light_path, NodePath("../SunLight"))
    assert_eq(tokisan_environment.sky_dome.moon_light_path, NodePath("../MoonLight"))


func test_proxies_season_and_weather_to_material_manager() -> void:
    var environment_node: MaszynaEnvironmentNode = MaszynaEnvironmentNode.new()

    environment_node.season = MaszynaEnvironment.Season.SEASON_WINTER
    environment_node.weather = MaszynaEnvironment.Weather.WEATHER_RAIN

    assert_eq(MaterialManager.season, MaszynaEnvironment.Season.SEASON_WINTER)
    assert_eq(MaterialManager.weather, MaszynaEnvironment.Weather.WEATHER_RAIN)
    environment_node.free()


func test_sets_season_from_manual_date_thresholds() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var cases: Array[Array] = [
        [5, 3, 2026, MaszynaEnvironment.Season.SEASON_WINTER],
        [7, 3, 2026, MaszynaEnvironment.Season.SEASON_SPRING],
        [7, 6, 2026, MaszynaEnvironment.Season.SEASON_SPRING],
        [8, 6, 2026, MaszynaEnvironment.Season.SEASON_SUMMER],
        [9, 9, 2026, MaszynaEnvironment.Season.SEASON_SUMMER],
        [10, 9, 2026, MaszynaEnvironment.Season.SEASON_AUTUMN],
        [7, 12, 2026, MaszynaEnvironment.Season.SEASON_AUTUMN],
        [8, 12, 2026, MaszynaEnvironment.Season.SEASON_WINTER],
    ]

    for case_data: Array in cases:
        environment_node.day = case_data[0]
        environment_node.month = case_data[1]
        environment_node.year = case_data[2]
        environment_node._process(0.0)

        assert_eq(environment_node.season, case_data[3])
        assert_eq(MaterialManager.season, case_data[3])


func test_set_date_normalizes_invalid_date_via_time_of_day() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()

    environment_node.set_date(2025, 4, 31)

    assert_eq(environment_node.day, 1)
    assert_eq(environment_node.month, 5)
    assert_eq(environment_node.year, 2025)


func test_uses_time_of_day_date_when_system_time_enabled() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var time_of_day: TimeOfDay = _get_tokisan_environment(environment_node).time_of_day

    environment_node.day = 1
    environment_node.month = 1
    environment_node.year = 2026
    environment_node.use_system_time = true
    environment_node._process(0.0)

    assert_eq(environment_node.day, time_of_day.day)
    assert_eq(environment_node.month, time_of_day.month)
    assert_eq(environment_node.year, time_of_day.year)
    assert_eq(
        environment_node.season,
        environment_node._season_from_year_day(
            environment_node._get_year_day(
                environment_node.day, environment_node.month, environment_node.year
            )
        )
    )


func _create_environment_node() -> MaszynaEnvironmentNode:
    return add_child_autofree(MaszynaEnvironmentNode.new()) as MaszynaEnvironmentNode


func _get_tokisan_environment(
    environment_node: MaszynaEnvironmentNode
) -> TokisanSky3DMaszynaEnvironment:
    return environment_node._sky_environment as TokisanSky3DMaszynaEnvironment


func _assert_children_have_no_owner(parent: Node) -> void:
    for child: Node in parent.get_children(true):
        assert_null(child.owner)
        _assert_children_have_no_owner(child)
