@tool
extends RefCounted

## Original engine's "sky:" token only ever carried a legacy skybox model filename
## (deserialize_sky(), simulationstateserializer.cpp - "sky model", nothing else defined).
## The current sky is fully procedural (MaszynaEnvironmentNode), which has
## no equivalent model to load - there is nothing left to import here, just consume the token so
## the parser stream stays in sync. Scenario time still applies via "time:" (see
## maszyna_time_importer.gd), against whichever MaszynaEnvironmentNode is already live in the
## scene - scenery files aren't expected to declare their own per-load environment node.
func import(p: MaszynaParser, _context: MaszynaImporterContext) -> Array:
    p.get_tokens_until("endsky")
    return []
