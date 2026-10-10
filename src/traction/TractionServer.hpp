#pragma once
#include "../tracks/SpatialIndex.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* The electrical topology of the overhead traction wires - kept apart from the rendering of
     * them, the same split TrackServer has against the track rendering. Ports
     * Traction.cpp/TractionPower.cpp of the original engine: named power sources feeding wire
     * spans, a resistive network built once per scenery load
     * (traction_table::InitTraction(), including the pass that feeds a section's open end from
     * the powered span it hangs beside), and the geometric "which wire is above this point" query
     * (scene::basic_cell::update_traction()).
     *
     * Not ported: TTraction::PowerSet()'s section-marker replacement (Traction.cpp:747-806, only
     * relevant when a sectioning cabin sits between two differently named substations) and
     * update_traction()'s pantograph-horn lateral tolerance. Both are refinements on top of the
     * network and contact logic ported here, not needed for a wire to deliver the right voltage. */
    class TractionServer : public Object {
            GDCLASS(TractionServer, Object)

        public:
            static TractionServer *get_instance() {
                return Object::cast_to<TractionServer>(Engine::get_singleton()->get_singleton("TractionServer"));
            }

        private:
            static void _bind_methods();

            /* Original engine: Traction.cpp's own "close enough to be one continuous wire"
             * distance, used both for the connectivity graph and for the tolerance at a span's
             * ends in the contact test. */
            /* Two span ends are the same point. The original's own value, compared per axis
             * (TTraction::TestPoint(), Traction.cpp:355) - a rounder, safer-looking number joins
             * spans the scenery deliberately placed apart, and since the pantograph now follows
             * the chain, a wrong join sends it onto the wrong wire (see `FINDINGS.md`,
             * 2026-09-20, where the same mistake on track endpoints merged every double slip). */
            static constexpr double WIRE_JOIN_EPSILON = 0.025;
            /* How far past a span's own end the pantograph still counts as on it. An absolute
             * tolerance rather than a fraction of the span: hand-authored scenery leaves ends a
             * few centimetres apart, and a span whose neighbour is beyond WIRE_JOIN_EPSILON has
             * no chain to follow, so this is what carries the contact across the gap. */
            static constexpr double SPAN_END_TOLERANCE = 0.25;
            static constexpr double GRID_CELL_SIZE = 500.0;
            static constexpr double QUERY_MARGIN = 10.0;
            /* How far a wire may sit outside the collector's own half width and still be caught,
             * by the pantograph's guide horn (DynObj.cpp:93, fWidthExtra), and how much higher a
             * wire at the horn's tip counts than one on the slider (scene.cpp:109). */
            static constexpr double HORN_CONTACT_RISE = 0.15;
            /* An upper bound on stepping along the chain of spans in one frame, the original's own
             * (DynObj.cpp:8743) - a chain that loops would otherwise never end. */
            static constexpr int MAX_WIRE_HOPS = 30;
            /// Traction.cpp:107-110 - a span declaring 0.01 Ohm/km (the old default) gets 0.075
            static constexpr double LEGACY_RESISTIVITY = 0.01;
            static constexpr double DEFAULT_RESISTIVITY = 0.075;
            /* TTraction::iLast bits (Traction.h:31): the span ends its section (Traction.cpp:400),
             * or is the second to last (Traction.cpp:404). */
            static constexpr int LAST_SPAN = 0x1;
            static constexpr int SECOND_LAST_SPAN = 0x2;
            /* Either flag of TTraction::iLast sends the pantograph back to an area search
             * (DynObj.cpp:8747). */
            static constexpr int LAST_SPAN_FLAGS = LAST_SPAN | SECOND_LAST_SPAN;
            /* How far a span's box in the spatial index reaches past its ends - the wrapper's own,
             * the original searches whole scene cells */
            static constexpr double WIRE_AABB_MARGIN = 5.0;
            /* The network's leakage, so an unloaded network is never an open circuit
             * (TractionPower.h:58-59, TractionPower.cpp:112-113) */
            static constexpr double LEAKAGE_ADMITTANCE = 1e-10;
            /* Below it a load still counts as present: the fuse timer waits for it to go
             * (TractionPower.cpp:122) */
            static constexpr double FUSE_LOAD_RESISTANCE = 100.0;
            /* A recuperating network raises the substation's voltage (TractionPower.cpp:128) */
            static constexpr double RECUPERATION_VOLTAGE_FACTOR = 1.083;
            /* The resistance a span assumes with no current drawn (Traction.cpp:483-486) */
            static constexpr double NO_LOAD_RESISTANCE = 10000.0;
            /* How far from a section's open end the powered span it hangs beside may lie - the
             * bounding radius of one scene cell the original searches (scene.h:221,
             * 0.5 * sqrt(2) * EU07_CELLSIZE 250). */
            static constexpr double SECTION_END_SEARCH_RADIUS = 176.78;

            /// Mirrors TTractionPowerSource's own members and their defaults (TractionPower.h:48-59).
            struct PowerSource {
                    String name;
                    double nominal_voltage = 0.0;
                    double voltage_frequency = 0.0;
                    double internal_resistance = 0.2;
                    double max_output_current = 0.0;
                    double fast_fuse_timeout = 1.0;
                    int fast_fuse_repetition = 3;
                    double slow_fuse_timeout = 60.0;
                    bool recuperation = false;
                    bool is_autogenerated = false;
                    /* TTractionPowerSource::bSection - the data names a section of the network,
                     * not a substation. A section feeds nothing by itself; the spans belonging to
                     * it take their power through the network from a substation somewhere along
                     * it (TTraction::PowerSet(), Traction.cpp:460). */
                    bool is_section = false;

                    double total_current = 0.0;
                    double total_admittance = LEAKAGE_ADMITTANCE;
                    double total_previous_admittance = LEAKAGE_ADMITTANCE;
                    bool fast_fuse = false;
                    bool slow_fuse = false;
                    double fuse_timer = 0.0;
                    int fuse_counter = 0;

                    bool fuse() const {
                        return fast_fuse || slow_fuse;
                    }
                    /// Port of TTractionPowerSource::Update(dt).
                    void tick(double p_delta);
                    /* What the source puts out by the load of the previous tick - the part of
                     * TTractionPowerSource::CurrentGet(res) both halves below share */
                    double output_current() const;
                    /* The current a load of p_resistance takes - the reading half of
                     * TTractionPowerSource::CurrentGet(res) (TractionPower.cpp:117) */
                    double current_get(double p_resistance) const;
                    /* A load of p_resistance drawn in this tick, counted by the next one - the
                     * writing half of CurrentGet(res): the admittance added, and a fuse waiting
                     * for the load to go (TractionPower.cpp:119-130) */
                    void draw(double p_resistance);
            };

            struct Wire {
                    Vector3 p1;
                    Vector3 p2;
                    String power_supply_name;
                    double nominal_voltage = 0.0;
                    double max_current = 0.0;
                    double resistivity = 0.0; // Ohm/m

                    /// psPowered - a directly powered (short-circuit) wire.
                    RID power_source;
                    /// psSection - which section of the network this span belongs to, if any.
                    RID section;
                    RID power_near[2];                   // psPower
                    double resistance[2] = {-1.0, -1.0}; // fResistance
                    RID next[2];                         // hvNext
                    int next_endpoint[2] = {-1, -1};     // iNext
                    /* TTraction::iLast - bit 0: this span ends a section, bit 1: the one after it
                     * does (TTraction::WhereIs(), Traction.cpp:392). Either makes a pantograph
                     * stop trusting the chain and look around instead. */
                    int last_flags = 0;
                    /* The span shares its running with another (TTraction::hvParallel) - the
                     * sibling is not reachable along hvNext, so the chain must not be trusted
                     * here either. Named in the data as `parallel <name>`. */
                    String parallel_name;
                    bool has_parallel = false;

                    double length() const {
                        return p1.distance_to(p2);
                    }
            };

            HashMap<RID, PowerSource> power_sources;
            HashMap<String, RID> power_sources_by_name;
            int64_t next_power_source_id = 0;
            HashMap<RID, Wire> wires;
            int64_t next_wire_id = 0;
            Ref<SpatialIndex> spatial_index;

            /* A source feeding a span and the resistance it sees */
            struct Feed {
                    RID source;
                    double resistance = 0.0;
            };

            void _on_simulation_advanced(double p_seconds);
            /* The span's feeds for a load of p_resistance on it, into p_feeds; returns how many */
            int _wire_feeds(const Wire &p_wire, double p_resistance, Feed (&p_feeds)[2]) const;
            /* The load the vehicle stands for, by its previous reading (Traction.cpp:483-486) */
            static double _load_resistance(double p_assumed_voltage, double p_current);
            void _resolve_power_sources();
            void _connect_wires();
            /// Port of TTraction::WhereIs() over every span, once the chain is built.
            void _mark_section_ends();
            /// Port of the `parallel` half of traction_table::InitTraction() (Traction.cpp:830-856).
            void _resolve_parallel_spans();
            void _propagate_resistance();
            void _resistance_walk(const RID &p_from_wire, int p_direction, double p_resistance, const RID &p_source);
            /// Port of the section-ends pass of traction_table::InitTraction() (Traction.cpp:858-895).
            void _connect_section_ends();
            /* Height of one wire above a point along the pantograph's plane; INF when the plane
             * misses the span, or the wire is below the point or beyond the reach of the collector
             * and its horns (scene::basic_cell::update_traction()'s geometry test). */
            double _wire_height_above(
                    const Wire &p_wire, const Vector3 &p_position, const Vector3 &p_up, const Vector3 &p_forward,
                    const Vector3 &p_left, double p_width, double p_horn_width) const;

        public:
            TractionServer();

            RID power_source_create();
            void power_source_set_params(
                    const RID &p_power_source, const String &p_name, double p_nominal_voltage,
                    double p_voltage_frequency, double p_internal_resistance, double p_max_output_current,
                    double p_fast_fuse_timeout, int p_fast_fuse_repetition, double p_slow_fuse_timeout,
                    bool p_recuperation, bool p_is_autogenerated, bool p_is_section = false);
            RID power_source_get_rid_by_name(const String &p_name) const;
            /* TTractionPowerSource::VoltageSet() - what a scenery `voltage` event changes */
            void power_source_set_nominal_voltage(const RID &p_power_source, double p_voltage);
            double power_source_get_nominal_voltage(const RID &p_power_source) const;
            void power_source_free(const RID &p_power_source);

            RID wire_create();
            /* p_resistivity is the span's resistivity as a scenery declares it, in Ohm/km. */
            void wire_set_params(
                    const RID &p_wire, const Vector3 &p_p1, const Vector3 &p_p2, const String &p_power_supply_name,
                    double p_nominal_voltage, double p_max_current, double p_resistivity);
            void wire_free(const RID &p_wire);
            /* The span this one shares its running with, by name, as the data declares it. "none"
             * and "*" mean the author knows there is one but did not name it. */
            void wire_set_parallel(const RID &p_wire, const String &p_parallel_name);

            /* Resolves named power sources (auto-creating one from a wire's own nominal values
             * when the name matches none - traction_table::InitTraction()'s fallback,
             * Traction.cpp:704-721), connects wire endpoints into a network and propagates
             * source and resistance outward from every directly powered wire. Called once per
             * scenery load, after every wire and source of that load exists. */
            void network_build();

            /* Port of TTraction::VoltageGet(u, i), the reading half: the voltage on the span for
             * the sources' load of the previous tick. `p_assumed_voltage` is the vehicle's own
             * previous reading, used only to derive an equivalent load resistance - the same
             * relaxation the original uses - and `p_current` the instantaneous draw. */
            double wire_get_voltage(const RID &p_wire, double p_assumed_voltage, double p_current) const;
            /* The other half of TTraction::VoltageGet(u, i): the load drawn from the span in this
             * tick, which the sources feeding it count on their next tick. Called once per
             * collector per step, with the same arguments as wire_get_voltage(). */
            void wire_draw_current(const RID &p_wire, double p_assumed_voltage, double p_current);

            /* Which wire span passes above a point, as {rid, height}. `height` is INF when no
             * wire is found - the original's own "no wire in reach" (scene.cpp resets
             * PantTraction to DBL_MAX before every scan), which the raise simulation relies on to
             * keep extending the pantograph to its joint limit. */
            Dictionary wire_find_above_with_height(
                    const Vector3 &p_position, const Vector3 &p_up, const Vector3 &p_forward, const Vector3 &p_left,
                    double p_width, double p_horn_width) const;

            /* Where a pantograph already on `p_from_wire` is now, as {rid, height}. Running off
             * the end of a span is not a loss of contact: the spans are a doubly linked list and
             * the original steps along it until the point falls inside one
             * (vehicle_table::update_traction(), DynObj.cpp:8742-8770). An invalid `rid` means
             * the chain ran out or the wire left the collector's reach sideways, which is the
             * original's own signal to search the area instead. */
            Dictionary wire_follow_above(
                    const RID &p_from_wire, const Vector3 &p_position, const Vector3 &p_up, const Vector3 &p_forward,
                    const Vector3 &p_left, double p_width, double p_horn_width) const;
    };
} // namespace godot
