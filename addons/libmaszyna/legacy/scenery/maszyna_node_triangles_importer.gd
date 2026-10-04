@tool
extends RefCounted

## The triangles go to the context's sink as they are read (SceneryTrianglesSink)
func import(p: MaszynaParser, context: MaszynaImporterContext, range_min: float, range_max: float) -> void:
    MaszynaTrianglesImporter.import_triangles(
        p, context.rotate, context.origin, context.triangles_sink, range_min, range_max
    )
