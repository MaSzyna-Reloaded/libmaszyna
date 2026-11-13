extends MaszynaGutTest

## libmaszyna's project settings have their default values in the running game too, not only in the
## editor: those read in GDScript registered by MaszynaProjectSettings as the game starts, those read
## in C++ by their owners as the library starts (MaszynaPlugin::register_classes())

## The scenery lights' mode registered by MaszynaE3DLightFactory: Economy (SCENERY_LIGHTS_ECONOMY)
const SCENERY_LIGHTS_MODE_SETTING: String = "maszyna/scenery/lights/mode"
const SCENERY_LIGHTS_ECONOMY: int = 1


func test_the_settings_read_in_gdscript_have_their_defaults() -> void:
    assert_eq(ProjectSettings.property_get_revert(MaszynaMaterialManager.DDS_MAX_TEXTURE_SIZE_SETTING),
            MaszynaMaterialManager.DDS_MAX_TEXTURE_SIZE_DEFAULT, "the texture size limit")
    assert_eq(ProjectSettings.property_get_revert(MmdCabinInstancer.DDS_MAX_CAB_TEXTURE_SIZE_SETTING),
            MmdCabinInstancer.DDS_MAX_CAB_TEXTURE_SIZE_DEFAULT, "the cab's texture size limit")
    assert_eq(ProjectSettings.property_get_revert(CabinPythonScreen.SCREEN_GLOW_ENERGY_SETTING),
            CabinPythonScreen.SCREEN_GLOW_ENERGY_DEFAULT, "a screen's glow")


func test_the_settings_read_in_cpp_have_their_defaults() -> void:
    assert_eq(ProjectSettings.property_get_revert(SCENERY_LIGHTS_MODE_SETTING), SCENERY_LIGHTS_ECONOMY,
            "the scenery lights' mode")
