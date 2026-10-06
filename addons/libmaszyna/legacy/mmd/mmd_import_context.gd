extends RefCounted
class_name MmdImportContext

## Cross-file shared state for a single MMD parse pass, mirroring FizImportContext's role for
## FIZ imports. One instance per MmdCabinInstancer.parse() call (shared across `include`d files).

var base_dir:String = ""
var include_depth:int = 0
var cab_number:int = 1
## The (pN) the vehicle's own MMD is read with (MmdCabinInstancer.vehicle_parameters())
var parameters:Dictionary = {}

## Random-file-set `include [a.inc b.inc] end` choices, keyed by "source_file:line" of the
## include directive - owned by the caller (MaszynaDynamicTrainCabin) and passed back in on every
## parse() call so a later cab1<->cab2 rebuild reuses the same choice instead of re-rolling it.
var random_choices:Dictionary = {}

## Accumulated MmdCabinBuildReport.get_diagnostics()-shaped records: severity, code,
## source_file, line, cabin_number, mmd_label, submodel_name, message.
var diagnostics:Array[Dictionary] = []

var _warned_unsupported_labels:Dictionary = {}


func add_diagnostic(
        severity:String, code:String, message:String,
        source_file:String = "", line:int = 0, mmd_label:String = "", submodel_name:String = "") -> void:
    diagnostics.append({
        "severity": severity,
        "code": code,
        "source_file": source_file,
        "line": line,
        "cabin_number": cab_number,
        "mmd_label": mmd_label,
        "submodel_name": submodel_name,
        "message": message,
    })


## One MMD_BINDING_UNSUPPORTED diagnostic per unique label, not per include-expanded occurrence.
func warn_unsupported_label(label:String, source_file:String, line:int) -> void:
    if _warned_unsupported_labels.has(label):
        return
    _warned_unsupported_labels[label] = true
    add_diagnostic(
            "info", "MMD_BINDING_UNSUPPORTED",
            "MMD label '%s' is not in the supported catalog - no widget built for it." % label,
            source_file, line, label)
