@tool
extends TractionData
class_name MaszynaTractionData

## A .scn's traction span: what the core's TractionData says, and the wire's material the original
## draws it with (Traction.cpp)

## What the wires are made of (the .scn's material token)
enum WireMaterial {COPPER, ALUMINIUM}
## The bits of [member TractionData.damage_flag] the original knows: the wires dulled by patina,
## and broken (read by nothing yet)
const DAMAGE_PATINA: int = 1
const DAMAGE_BREAK: int = 128
## The backend a .scn's traction is drawn by as the original draws it
## (MaszynaTractionRenderingBackend, registered by MaszynaSceneryProvider)
const RENDERING_BACKEND: StringName = &"maszyna"

@export var material: WireMaterial = WireMaterial.COPPER


func _init() -> void:
    rendering_backend = RENDERING_BACKEND
