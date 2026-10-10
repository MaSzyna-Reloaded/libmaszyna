extends MaszynaGutTest


func test_query_skips_empty_cells() -> void:
    var spatial_index: SpatialIndex = SpatialIndex.new()
    spatial_index.cell_size = 1.0
    var track_rid: RID = RID()

    spatial_index.add(track_rid, Rect2(Vector2(0.0, 0.0), Vector2(0.1, 0.1)))

    assert_eq(
        spatial_index.query(Rect2(Vector2(0.0, -1.0), Vector2(0.1, 1.1))),
        [track_rid]
    )
