#pragma once
#include "maszyna/scenery/MaszynaTrackData.hpp"
#include "maszyna/tracks/MaszynaTrackLooks.hpp"
#include "maszyna/tracks/MaszynaTrackMeshBuilder.hpp"
#include "rendering/TrackRenderingBackend.hpp"
#include "tracks/TrackCurve.hpp"
#include "tracks/TrackEndpointRef.hpp"
#include "tracks/TrackServer.hpp"
#include "utils/RIDHash.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    /* The tracks drawn as the original draws them (Track.cpp): rails of a rail profile
     * (<game dir>/models/tory/railprofile_<name>.txt), a switch's blades moving with its offsets, a
     * trackbed of a section, a road of its surface and shoulders, each of its textures. A .scn's
     * track (MaszynaTrackData) is drawn with its own textures; any other with the textures its
     * features map to (LOOKS_PATH). A streamed track is drawn with the tracks around it, and a
     * streamed road with the roads around it, in a cell of CELL_SIZE streamed by itself; a track
     * that is not streamed (a hand-placed one) is drawn by itself. Registered with
     * TrackRenderingServer as NAME. */
    class MaszynaTrackRenderingBackend : public TrackRenderingBackend {
            GDCLASS(MaszynaTrackRenderingBackend, TrackRenderingBackend)

        public:
            /// What a track drawn by itself draws, each in a RenderingServer instance of its own; a
            /// track drawn by its cell has only its blades
            enum Slot {
                SLOT_PRIMARY_RAIL,
                SLOT_SECONDARY_RAIL,
                SLOT_PRIMARY_BLADE,
                SLOT_SECONDARY_BLADE,
                SLOT_TRACKBED,
                SLOT_MAX,
            };
            /// What a cell draws, each one mesh of a surface per material in a RenderingServer
            /// instance of its own: the rails cast the sun's shadow, the bed - the trackbeds, a road
            /// cell's roads - lies on the ground and casts none
            enum CellLayer {
                CELL_LAYER_RAILS,
                CELL_LAYER_BED,
                CELL_LAYER_MAX,
            };

            /// Its name with TrackRenderingServer, the one MaszynaTrackData names
            static constexpr const char *NAME = MaszynaTrackData::RENDERING_BACKEND;
            /// What a TrackData's features are drawn with: textures and sections by bed and by surface
            static constexpr const char *LOOKS_PATH = "res://addons/libmaszyna/tracks/maszyna_track_looks.tres";
            /// Side of a cell [m]: one cell is built in one go by the streaming, so it holds about as
            /// many tracks as one frame's build budget fits (a town's ~100 road segments)
            static constexpr double CELL_SIZE = 125.0;

        private:
            using RailProfile = MaszynaTrackMeshBuilder::RailProfile;
            using ProfilePoint = MaszynaTrackMeshBuilder::ProfilePoint;
            using Section = MaszynaTrackMeshBuilder::Section;
            using Geometry = MaszynaTrackMeshBuilder::Geometry;
            using CurveShape = MaszynaTrackMeshBuilder::CurveShape;
            using SwitchBladeLayout = MaszynaTrackMeshBuilder::SwitchBladeLayout;
            using BladePose = MaszynaTrackMeshBuilder::BladePose;
            using RoadLook = MaszynaTrackMeshBuilder::RoadLook;
            using TrackPiece = MaszynaTrackMeshBuilder::TrackPiece;
            using PieceGeometry = MaszynaTrackMeshBuilder::PieceGeometry;

            /// The track's curves and sizes as TrackServer has them, read on the main thread
            struct TrackGeometry {
                    RID track;
                    Ref<TrackCurve> curve1;
                    Ref<TrackCurve> curve2;
                    double width = 0.0;
                    TrackServer::SwitchCommonPoint switch_common_endpoint_index = TrackServer::POINT_NONE;
                    double switch_f_offset1 = 0.0;
                    double switch_f_offset2 = 0.0;

                    bool is_valid() const;
            };
            /* What a track is drawn of and with, read on the main thread from TrackServer, its look and
             * its neighbours (_read_track_piece()): its piece, of which MaszynaTrackMeshBuilder computes
             * its geometry - on the streaming's worker for a track of a cell, which reads nothing else of
             * the backend - and its materials by name */
            struct TrackSnapshot {
                    RID track_render;
                    TrackPiece piece;
                    /// The rails' (the main curve's and the blades') and a road surface's material...
                    String primary_material;
                    /// ...the second curve's rails' and blade's...
                    String secondary_material;
                    /// ...and the trackbed's, a road's sides'; none draws no trackbed
                    String bed_material;
            };
            /// A blade of a switch, as its switch has moved it
            struct Blade {
                    BladePose pose;
                    double offset = 0.0;
            };
            /// What the backend keeps of a track it draws
            struct Track {
                    RID track;
                    RID scenario;
                    bool visible = true;
                    /// Made when the track first draws something there: a plain track has no blades, a road
                    /// no rails, a track of a cell only its blades
                    RID instances[SLOT_MAX];
                    Ref<ArrayMesh> meshes[SLOT_MAX];
                    /// Where the meshes stand, the start of the track's curve: their vertices are relative to
                    /// it, as floats keep them, and their instances are placed there in the world's precision
                    /// - vertices in the world's coordinates shook as the camera moved (#351)
                    Vector3 origin;
                    Blade primary_blade;
                    Blade secondary_blade;
                    bool switch_is_right = false;
                    double tex_length = MaszynaTrackData::DEFAULT_TEX_LENGTH;
                    double tex_height = 0.0;
                    double tex_width = 0.0;
                    double tex_slope = 0.0;
                    String railprofile = MaszynaTrackData::DEFAULT_RAILPROFILE;
                    String material1_name;
                    String material2_name;
                    String material_trackbed_name;
                    /// A streamed track is drawn by the cell holding it, but for its blades
                    RID cell;
            };
            /// What a cell holds: the streamed roads, or the streamed tracks
            enum CellKind {
                CELL_ROADS,
                CELL_TRACKS,
                CELL_KIND_MAX,
            };
            /* The streamed tracks (or roads) of one square of the map in one scenario, drawn as one mesh
             * of a surface per material in each layer: a town has thousands of road segments, a large
             * station hundreds of tracks, each a draw call or two of a few triangles when drawn by
             * itself. Measured on an imported Olkusz (2026-10-13, 3904 roads): 42-44 ms a frame drawn
             * by themselves against 27-36 ms without them; 5742 roads in cells: +375 draw calls, the
             * frame time of the scenery without roads. Its geometry is computed on the streaming's
             * worker from its pieces (_cell_preload()); the main thread only makes the meshes of it */
            struct Cell {
                    CellKind kind = CELL_TRACKS;
                    RID scenario;
                    Vector2i coords;
                    /// The cell's centre, where its meshes stand and it is streamed from (Track::origin);
                    /// set as it is made
                    Vector3 origin;
                    /// Track render RIDs of the tracks in the cell
                    Vector<RID> tracks;
                    /// Its visible tracks as the worker computes the cell's geometry from them: written on
                    /// the main thread under cells_mutex, read by the worker under it
                    Vector<TrackSnapshot> snapshots;
                    RID stream_rid;
                    bool streamed = false;
                    RID instances[CELL_LAYER_MAX];
                    Ref<ArrayMesh> meshes[CELL_LAYER_MAX];
            };
            struct CellKey {
                    RID scenario;
                    Vector2i coords;
                    CellKind kind = CELL_TRACKS;

                    bool operator==(const CellKey &p_other) const;
            };
            struct CellKeyHasher {
                    static uint32_t hash(const CellKey &p_key);
            };

            RIDMap<Track> tracks;
            /// Cells by their RID, and by their scenario, coordinates and kind. The map is changed on
            /// the main thread under cells_mutex: the worker looks a cell's pieces up in it
            RIDMap<Cell> cells;
            HashMap<CellKey, RID, CellKeyHasher> cells_by_key;
            Mutex cells_mutex;
            /// The streaming's owners of the road cells and of the track cells
            int stream_owners[CELL_KIND_MAX] = {};
            /// The rail profile of a track that names none, and of one the data has not got
            RailProfile default_rail_profile;
            /// The rail profiles read so far, by name - the data's, until it is unloaded
            HashMap<String, RailProfile> rail_profiles;
            /// Texture length ("size:" y) by material name, -1 for none - asked once per material
            HashMap<String, double> material_texture_lengths;
            /// Width/height of the texture by material name, asked once per material (roads)
            HashMap<String, double> material_texture_ratios;
            /// Tracks draw both faces - an open rail profile with its front faces culled casts almost nothing
            Dictionary track_material_options;
            Ref<MaszynaTrackLooks> looks;
            /// A number of a rail profile's line
            Ref<RegEx> rail_profile_number;

            static TrackGeometry _read_track_geometry(const RID &p_track);
            static CurveShape _read_curve_shape(const Ref<TrackCurve> &p_curve);
            static bool _is_road(const RID &p_track, const TrackGeometry &p_geometry);
            static bool _is_switch_track(const TrackGeometry &p_geometry);
            static double _get_track_roll(const TrackGeometry &p_geometry, int p_endpoint_index);
            static SwitchBladeLayout
            _switch_blade_layout(bool p_is_right_switch, double p_f_offset1, double p_f_offset2, double p_f_max_offset);
            static bool _has_trackbed_profile_options(const Track *p_track);

            /// The track TrackServer's p_track is drawn as by this backend; null for none
            Track *_find_track(const RID &p_track);
            /// The rail profile of the name, read from the game directory the first time it is drawn
            const RailProfile &_load_rail_profile(const String &p_name);
            /// The length a material's texture covers ("size:" of its .mat), -1 if it declares none -
            /// asked of ReloadedMaterialServer the first time
            double _load_texture_length(const String &p_material_name);
            /// The width/height ratio of a material's texture - a road's surface is as wide as its
            /// texture's length times it (fTexRatio, Track.cpp:610-621); 1 when the texture is not known
            double _load_texture_ratio(const String &p_material_name);
            /// TTrack::texture_length() (Track.cpp:2484-2494): the material's size, else the track's own
            double _load_road_texture_length(const Track &p_track, const String &p_material_name);
            /// A surface of p_arrays added to p_mesh, drawn with the material of the name; an unnamed one
            /// has no material - ReloadedMaterialServer would hand back the missing-texture placeholder
            /// instead (Track.cpp:485-491 keeps a null handle)
            void
            _mesh_add_surface(const Ref<ArrayMesh> &p_mesh, const Array &p_arrays, const String &p_material_name) const;
            /// The material the track's trackbed is drawn with, by its name - its own or a neighbour's -
            /// and the length its texture covers; an empty name for no trackbed
            void _resolve_trackbed(const Track &p_track, String &p_r_material_name, double &p_r_texture_length);
            const Track *_copy_adjacent_trackbed_material(const Track *p_track, RIDSet &p_visited);
            Vector<RID> _get_trackbed_material_sources(const RID &p_track, bool p_is_switch) const;
            void _build_switch_trackbed_sections(
                    const Track &p_track, const TrackGeometry &p_geometry, TrackServer::SwitchTrack p_switch_track,
                    double p_start_roll, double p_end_roll, double p_rail_height, double p_texture_length,
                    Section &p_r_start, Section &p_r_end);
            /// What the track is drawn of and with now, read on the main thread; false when it has no
            /// curve
            bool _read_track_piece(const RID &p_track_render, const Track &p_track, TrackSnapshot &p_snapshot);
            /// A road's surface and sides (Track.cpp:1479-1517) into the snapshot, read with the next road
            void _read_road_piece(const Track &p_track, const TrackGeometry &p_geometry, TrackSnapshot &p_snapshot);
            /// Draws the slot's mesh in the slot's instance, made as the slot first draws something
            static void _slot_draw(Track &p_track, Slot p_slot);
            static void _reset_track_instances(Track &p_track);
            static void _clear_track_meshes(Track &p_track);
            /// The streamed track goes into the cell of its kind of its scenario holding its start, the
            /// cell made and registered with the streaming as its first track comes
            void _cell_add(const RID &p_track_render, Track &p_track);
            void _cell_remove(const RID &p_track_render, Track &p_track);
            /// The track's piece in its cell as the track is now - none while it is hidden - and the cell
            /// streamed again with it
            void _cell_update_track(const RID &p_track_render, const Track &p_track);
            /* The cell streamed from its pieces as they are now: registered with SceneryStreamingServer,
             * which builds a piece again only as it is registered again - so a cell streamed again lets
             * its old piece of the streaming go with what it built, and its new one is computed on the
             * worker and built as the camera wants it */
            void _cell_stream(const RID &p_cell_rid, Cell &p_cell);
            /// The streaming's preload of a cell, on its worker: the geometry of the cell's pieces, a
            /// surface's arrays per layer and material, and its switches' blades
            Variant _cell_preload(const RID &p_cell);
            /// The streaming's build of a cell: its meshes made of what the preload computed
            void _cell_build(const RID &p_cell, const Variant &p_preloaded);
            void _cell_clear(const RID &p_cell);
            void _on_data_unload_requested();
            /// A cell of the kind of the track's kind: the road cell of a streamed road, the track cell
            /// of any other track; none for a track drawn by itself
            RID _track_get_cell(const RID &p_track_render, CellKind p_kind) const;
            RID _cell_get_instance(const RID &p_cell, CellKind p_kind, CellLayer p_layer) const;
            Ref<ArrayMesh> _cell_get_mesh(const RID &p_cell, CellKind p_kind, CellLayer p_layer) const;

        protected:
            static void _bind_methods();

        public:
            MaszynaTrackRenderingBackend();
            ~MaszynaTrackRenderingBackend() override;

            void track_create(const RID &p_track_render, const RID &p_track) override;
            void track_free(const RID &p_track_render) override;
            void track_set_description(const RID &p_track_render, const Ref<TrackData> &p_description) override;
            void track_set_visible(const RID &p_track_render, bool p_visible) override;
            void track_set_scenario(const RID &p_track_render, const RID &p_scenario) override;
            void track_set_switch_blade_offsets(const RID &p_track_render, double p_offset1, double p_offset2) override;
            /// Every track is streamed in the cell holding it: a road in a road cell, a track in a
            /// track cell
            bool track_stream(const RID &p_track_render) override;
            void track_build(const RID &p_track_render) override;
            /// The track's meshes go as it leaves the camera's range
            void track_clear(const RID &p_track_render) override;
            /// The tracks borrowing their trackbed are built again, and the cells' pieces read again
            /// with the neighbours they have now: a trackbed turns into the next one, a road into the
            /// next one it is joined to
            void topology_rebuilt() override;

            /// The meshes the track draws now, by their Slot
            Dictionary track_get_meshes(const RID &p_track_render) const;
            /// The instances the track has made so far, by their Slot
            Dictionary track_get_instances(const RID &p_track_render) const;
            /// What the track is drawn with: its materials' names (material1, material2,
            /// material_trackbed), its texture's tex_length, tex_height, tex_width and tex_slope and its
            /// railprofile, by MaszynaTrackData's names
            Dictionary track_get_look(const RID &p_track_render) const;
            /// The road cell drawing the streamed road; none for any other track
            RID track_get_road_cell(const RID &p_track_render) const;
            bool road_cell_exists(const RID &p_cell) const;
            /// The cell's instance; none before it is first built
            RID road_cell_get_instance(const RID &p_cell) const;
            /// The cell's roads as one mesh while it is built; null otherwise
            Ref<ArrayMesh> road_cell_get_mesh(const RID &p_cell) const;
            /// The track cell drawing the streamed track; none for a road and a track drawn by itself
            RID track_get_track_cell(const RID &p_track_render) const;
            bool track_cell_exists(const RID &p_cell) const;
            /// The cell's instance of the layer; none before it is first built
            RID track_cell_get_instance(const RID &p_cell, CellLayer p_layer) const;
            /// The layer of the cell's tracks as one mesh while the cell is built; null otherwise
            Ref<ArrayMesh> track_cell_get_mesh(const RID &p_cell, CellLayer p_layer) const;

            /// How far apart a track of p_curve is sampled as it is drawn [m]
            static double mesh_curve_bake_interval(const Ref<TrackCurve> &p_curve);
            /// Which rail of each curve of a switch is its blade, and how far each blade is moved, as a
            /// Dictionary: primary_blade_mirrored, primary_other_mirrored, secondary_blade_mirrored,
            /// secondary_other_mirrored, primary_blade_offset, secondary_blade_offset
            static Dictionary
            switch_blade_layout(bool p_is_right_switch, double p_f_offset1, double p_f_offset2, double p_f_max_offset);
            /// The one track joined to the endpoint of p_track; null when none or several are
            static Ref<TrackEndpointRef> unique_endpoint_connection(const RID &p_track, int p_endpoint_index);
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaTrackRenderingBackend::Slot)
VARIANT_ENUM_CAST(MaszynaTrackRenderingBackend::CellLayer)
