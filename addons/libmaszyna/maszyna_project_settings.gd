@tool
class_name MaszynaProjectSettings
extends RefCounted

## libmaszyna's project settings read in GDScript, each with its default value - its owner's
## constant - and its hint, registered by the editor plugin (libmaszyna.gd) and as the game starts
## (libmaszyna_runtime.gd), so the game knows their default values too: a setting at its default
## value is not saved in project.godot. The settings read in C++ are registered by their owners as
## the library starts (MaszynaPlugin::register_classes(), ReloadedProjectSetting)

## The texture size limits to choose from
const TEXTURE_SIZE_HINT: String = "512:512,1024:1024,2048:2048,4096:4096,8192:8192"


static func register() -> void:
    # The glow an indicator lamp throws into the cab while it is lit (an OmniLight3D at the lamp)
    ReloadedProjectSetting.add(
        MmdCabinInstancer.INDICATOR_GLOW_ENABLED_SETTING, MmdCabinInstancer.INDICATOR_GLOW_ENABLED_DEFAULT,
        TYPE_BOOL
    )
    ReloadedProjectSetting.add(
        MmdCabinInstancer.INDICATOR_GLOW_ENERGY_SETTING, MmdCabinInstancer.INDICATOR_GLOW_ENERGY_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,2.0,0.005"
    )
    ReloadedProjectSetting.add(
        MmdCabinInstancer.INDICATOR_GLOW_RANGE_SETTING, MmdCabinInstancer.INDICATOR_GLOW_RANGE_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.01,2.0,0.01,suffix:m"
    )
    # A lamp of several pieces - three alerter lamps in one mesh, a backlight overlay per gauge -
    # gets a light at each piece, the backlight in each overlay's own colour
    ReloadedProjectSetting.add(
        MmdCabinInstancer.REAL_INSTRUMENTS_LIGHTS_SETTING, MmdCabinInstancer.REAL_INSTRUMENTS_LIGHTS_DEFAULT,
        TYPE_BOOL
    )
    ReloadedProjectSetting.add(
        MmdCabinInstancer.INSTRUMENT_GLOW_ENERGY_SETTING, MmdCabinInstancer.INSTRUMENT_GLOW_ENERGY_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.001"
    )
    ReloadedProjectSetting.add(
        MmdCabinInstancer.INSTRUMENT_GLOW_RANGE_SETTING, MmdCabinInstancer.INSTRUMENT_GLOW_RANGE_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.01,2.0,0.01,suffix:m"
    )
    # A Python cab screen throws a glow in the colour of what it shows - weak, but far enough to
    # light the desk around it
    ReloadedProjectSetting.add(
        CabinPythonScreen.SCREEN_GLOW_ENABLED_SETTING, CabinPythonScreen.SCREEN_GLOW_ENABLED_DEFAULT, TYPE_BOOL
    )
    ReloadedProjectSetting.add(
        CabinPythonScreen.SCREEN_GLOW_ENERGY_SETTING, CabinPythonScreen.SCREEN_GLOW_ENERGY_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,2.0,0.005"
    )
    ReloadedProjectSetting.add(
        CabinPythonScreen.SCREEN_GLOW_RANGE_SETTING, CabinPythonScreen.SCREEN_GLOW_RANGE_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.05,4.0,0.05,suffix:m"
    )
    # A signal's or a lamp's free spotlight is drawn as a point of a constant size on the screen, the
    # way the original draws it, so it stays visible long after the lens is smaller than a pixel
    ReloadedProjectSetting.add(
        MaszynaMaterialManager.RAILWAY_LIGHTS_VISIBILITY_IMPROVED_SETTING,
        MaszynaMaterialManager.RAILWAY_LIGHTS_VISIBILITY_IMPROVED_DEFAULT, TYPE_BOOL
    )
    # How bright whatever self-illuminates (a clock's dial, a lit window) is drawn: the original's is the
    # texture's own colour on a screen that never exposes for the night, ours is exposed up with it
    ReloadedProjectSetting.add(
        MaszynaMaterialManager.SELFILLUM_ENERGY_SETTING, MaszynaMaterialManager.SELFILLUM_ENERGY_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0,2,0.01"
    )
    # A vehicle's mirror glass reflects the scene: a camera mirrored across it renders the glass at
    # its size on the screen while the mirror is in view within 30 m (PlanarMirror3D) - a scene
    # render per mirror
    ReloadedProjectSetting.add(
        MaszynaVehicleSystem.REAL_MIRRORS_SETTING, MaszynaVehicleSystem.REAL_MIRRORS_DEFAULT, TYPE_BOOL
    )
    # How many times the original's pointsize such a point is drawn; the original's 4 makes blobs
    ReloadedProjectSetting.add(
        MaszynaMaterialManager.RAILWAY_LIGHTS_POINT_SIZE_MULTIPLIER_SETTING,
        MaszynaMaterialManager.RAILWAY_LIGHTS_POINT_SIZE_MULTIPLIER_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.5,8.0,0.1"
    )
    # How much of the original's glare alpha the light's glare is drawn with; the full value makes the
    # billboard read as an overlay
    ReloadedProjectSetting.add(
        MaszynaMaterialManager.RAILWAY_LIGHTS_GLARE_INTENSITY_SETTING,
        MaszynaMaterialManager.RAILWAY_LIGHTS_GLARE_INTENSITY_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.05"
    )
    # The share of a light's cone (falloff angle) past which its glare fades out towards the cone's
    # edge; 1.0 keeps only the model's own hotspot-to-falloff band, a degree wide on vehicle lamps
    ReloadedProjectSetting.add(
        MaszynaMaterialManager.RAILWAY_LIGHTS_GLARE_FADE_START_SETTING,
        MaszynaMaterialManager.RAILWAY_LIGHTS_GLARE_FADE_START_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.05"
    )
    # How many degrees a glare's rays turn per degree the camera goes off the light's axis; 0 keeps
    # the same star from every side
    ReloadedProjectSetting.add(
        MaszynaMaterialManager.RAILWAY_LIGHTS_GLARE_ROTATION_RATIO_SETTING,
        MaszynaMaterialManager.RAILWAY_LIGHTS_GLARE_ROTATION_RATIO_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,4.0,0.1"
    )
    # A glare's size at the edge of the light's cone, as a share of its size on the axis; 1.0 keeps
    # the original's constant 2.5 m
    ReloadedProjectSetting.add(
        MaszynaMaterialManager.RAILWAY_LIGHTS_GLARE_EDGE_SIZE_SETTING,
        MaszynaMaterialManager.RAILWAY_LIGHTS_GLARE_EDGE_SIZE_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.05"
    )
    # The least share of the screen's height a glare spans, however far the light; 0 lets it shrink
    # with the distance like the original's 2.5 m
    ReloadedProjectSetting.add(
        MaszynaMaterialManager.RAILWAY_LIGHTS_GLARE_MIN_SCREEN_SIZE_SETTING,
        MaszynaMaterialManager.RAILWAY_LIGHTS_GLARE_MIN_SCREEN_SIZE_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,0.2,0.005"
    )
    # The sprite of the models' emitters: the original's single one, or the core's own smoke; the
    # labels come from the enum itself, so they cannot drift from what the code branches on
    ReloadedProjectSetting.add(
        MaszynaSmokeSourceLibrary.GENERATOR_MODE_SETTING, MaszynaSmokeSourceLibrary.GeneratorMode.ORIGINAL,
        TYPE_INT, PROPERTY_HINT_ENUM, ",".join(MaszynaSmokeSourceLibrary.GeneratorMode.keys())
    )
    ReloadedProjectSetting.add(
        MaszynaMaterialManager.DDS_MAX_TEXTURE_SIZE_SETTING, MaszynaMaterialManager.DDS_MAX_TEXTURE_SIZE_DEFAULT,
        TYPE_INT, PROPERTY_HINT_ENUM, TEXTURE_SIZE_HINT
    )
    ReloadedProjectSetting.add(
        MmdCabinInstancer.DDS_MAX_CAB_TEXTURE_SIZE_SETTING, MmdCabinInstancer.DDS_MAX_CAB_TEXTURE_SIZE_DEFAULT,
        TYPE_INT, PROPERTY_HINT_ENUM, TEXTURE_SIZE_HINT
    )
