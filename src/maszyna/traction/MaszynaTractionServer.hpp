#pragma once
#include "traction/TractionServer.hpp"

namespace godot {
    /* The traction network as the original computes it - Traction.cpp/TractionPower.cpp: named
     * power sources feeding wire spans, a resistive network built once per scenery load
     * (traction_table::InitTraction(), including the pass that feeds a section's open end from the
     * powered span it hangs beside), and the geometric "which wire is above this point" query
     * (scene::basic_cell::update_traction()). It stands in for TractionServer under that name.
     *
     * Not ported: TTraction::PowerSet()'s section-marker replacement (Traction.cpp:747-806, only
     * relevant when a sectioning cabin sits between two differently named substations). A
     * refinement on top of the network ported here, not needed for a wire to deliver the right
     * voltage. */
    class MaszynaTractionServer : public TractionServer {
            GDCLASS(MaszynaTractionServer, TractionServer)

        private:
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
            static constexpr double QUERY_MARGIN = 10.0;
            /* How much higher a wire at the guide horn's tip counts than one on the slider
             * (scene.cpp:109) - the horn reaches past the collector's own half width by
             * DynObj.cpp:93's fWidthExtra. */
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
            /* TTractionPowerSource's defaults (TractionPower.h:48-59), what a source made up for a
             * span naming no existing one keeps besides its voltage and current */
            static constexpr double DEFAULT_INTERNAL_RESISTANCE = 0.2;
            static constexpr double DEFAULT_FAST_FUSE_TIMEOUT = 1.0;
            static constexpr int DEFAULT_FAST_FUSE_REPETITION = 3;
            static constexpr double DEFAULT_SLOW_FUSE_TIMEOUT = 60.0;

            /* What a power source carries and how its fuses stand - TTractionPowerSource's state */
            struct SourceState {
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
            };

            /* Where a span sits in the network - TTraction's links and feeds */
            struct WireNetwork {
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
                     * here either. */
                    bool has_parallel = false;
            };

            /* A source feeding a span and the resistance it sees */
            struct Feed {
                    RID source;
                    double resistance = 0.0;
            };

            HashMap<RID, SourceState> source_states;
            HashMap<RID, WireNetwork> networks;

            /// Port of TTractionPowerSource::Update(dt).
            static void _source_tick(const PowerSource &p_source, SourceState &p_state, double p_delta);
            /* What the source puts out by the load of the previous tick - the part of
             * TTractionPowerSource::CurrentGet(res) both halves below share */
            static double _source_output_current(const PowerSource &p_source, const SourceState &p_state);
            /* The current a load of p_resistance takes - the reading half of
             * TTractionPowerSource::CurrentGet(res) (TractionPower.cpp:117) */
            static double
            _source_current_get(const PowerSource &p_source, const SourceState &p_state, double p_resistance);
            /* A load of p_resistance drawn in this tick, counted by the next one - the writing half
             * of CurrentGet(res): the admittance added, and a fuse waiting for the load to go
             * (TractionPower.cpp:119-130) */
            static void _source_draw(const PowerSource &p_source, SourceState &p_state, double p_resistance);
            /* The span's resistivity in Ohm/m, as the network works in it (Traction.cpp:112) */
            static double _resistivity(const Wire &p_wire);
            /* The span's feeds for a load of p_resistance on it, into p_feeds; returns how many */
            int _wire_feeds(const WireNetwork &p_network, double p_resistance, Feed (&p_feeds)[2]) const;
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
            static double _wire_height_above(
                    const Wire &p_wire, const Vector3 &p_position, const Vector3 &p_up, const Vector3 &p_forward,
                    const Vector3 &p_left, double p_width, double p_horn_width);

        protected:
            static void _bind_methods() {}

            void _simulation_advanced(double p_seconds) override;
            void _power_source_freed(const RID &p_power_source) override;
            void _wire_freed(const RID &p_wire) override;

        public:
            /* Resolves named power sources (auto-creating one from a wire's own nominal values
             * when the name matches none - traction_table::InitTraction()'s fallback,
             * Traction.cpp:704-721), connects wire endpoints into a network and propagates
             * source and resistance outward from every directly powered wire. */
            void network_build() override;
            /* Port of TTraction::VoltageGet(u, i), the reading half: the voltage on the span for
             * the sources' load of the previous tick. `p_assumed_voltage` is the vehicle's own
             * previous reading, used only to derive an equivalent load resistance - the same
             * relaxation the original uses - and `p_current` the instantaneous draw. */
            double wire_get_voltage(const RID &p_wire, double p_assumed_voltage, double p_current) const override;
            /* The other half of TTraction::VoltageGet(u, i) */
            void wire_draw_current(const RID &p_wire, double p_assumed_voltage, double p_current) override;
            /* `height` INF is the original's own "no wire in reach" (scene.cpp resets PantTraction
             * to DBL_MAX before every scan), which the raise simulation relies on to keep extending
             * the pantograph to its joint limit. */
            Dictionary wire_find_above_with_height(
                    const Vector3 &p_position, const Vector3 &p_up, const Vector3 &p_forward, const Vector3 &p_left,
                    double p_width, double p_horn_width) const override;
            /* Running off the end of a span is not a loss of contact: the spans are a doubly linked
             * list and the original steps along it until the point falls inside one
             * (vehicle_table::update_traction(), DynObj.cpp:8742-8770). An invalid `rid` means the
             * chain ran out or the wire left the collector's reach sideways, which is the
             * original's own signal to search the area instead. */
            Dictionary wire_follow_above(
                    const RID &p_from_wire, const Vector3 &p_position, const Vector3 &p_up, const Vector3 &p_forward,
                    const Vector3 &p_left, double p_width, double p_horn_width) const override;
    };
} // namespace godot
