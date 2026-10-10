extends MaszynaGutTest

## Regression (FINDINGS.md, 2026-10-06 "Crash at 0% loading on D3D12"): the Windows build was set
## to D3D12 with the Godot 4.6 bump, while the game is developed and tested on Vulkan only. A
## compute effect that Vulkan took (gnd-skydome's sun shafts) removed the D3D12 device in the first
## frames of a scenery load on a player's machine.
const WINDOWS_DRIVER_SETTING: String = "rendering/rendering_device/driver.windows"


func test_windows_renders_on_vulkan() -> void:
    assert_eq(
        str(ProjectSettings.get_setting(WINDOWS_DRIVER_SETTING)),
        "vulkan",
        "Windows runs on the renderer the game is tested on",
    )
