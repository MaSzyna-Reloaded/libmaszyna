#include "MaszynaTractionServer.hpp"
#include "utils/Units.hpp"
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MaszynaTractionServer::_simulation_advanced(const double p_seconds) {
        for (const KeyValue<RID, PowerSource> &entry: _get_power_sources()) {
            _source_tick(entry.value, source_states[entry.key], p_seconds);
        }
    }

    void MaszynaTractionServer::_power_source_freed(const RID &p_power_source) {
        source_states.erase(p_power_source);
    }

    void MaszynaTractionServer::_wire_freed(const RID &p_wire) {
        networks.erase(p_wire);
    }

    void MaszynaTractionServer::_source_tick(const PowerSource &p_source, SourceState &p_state, const double p_delta) {
        if (p_state.fast_fuse || p_state.slow_fuse) {
            p_state.total_current = 0.0;
        }
        if (p_state.total_current > p_source.max_output_current) {
            p_state.fast_fuse = true;
            p_state.fuse_counter += 1;
            if (p_state.fuse_counter > p_source.fast_fuse_repetition) {
                p_state.slow_fuse = true;
                UtilityFunctions::push_warning(
                        vformat("Power overload: \"%s\" disabled for %ss", p_source.name, p_source.slow_fuse_timeout));
            } else {
                UtilityFunctions::push_warning(
                        vformat("Power overload: \"%s\" disabled for %ss", p_source.name, p_source.fast_fuse_timeout));
            }
            p_state.fuse_timer = 0.0;
        }
        if (p_state.fast_fuse || p_state.slow_fuse) {
            p_state.total_admittance = 0.0;
            p_state.fuse_timer += p_delta;
            if (!p_state.slow_fuse) {
                if (p_state.fuse_timer > p_source.fast_fuse_timeout) {
                    p_state.fast_fuse = false;
                }
            } else if (p_state.fuse_timer > p_source.slow_fuse_timeout) {
                p_state.slow_fuse = false;
                p_state.fuse_counter = 0;
            }
        }
        p_state.total_previous_admittance = p_state.total_admittance;
        if (Math::is_zero_approx(p_state.total_previous_admittance)) {
            p_state.total_previous_admittance = LEAKAGE_ADMITTANCE;
        }
        p_state.total_admittance = LEAKAGE_ADMITTANCE;
    }

    double MaszynaTractionServer::_source_output_current(const PowerSource &p_source, const SourceState &p_state) {
        const double nominal = p_state.total_previous_admittance < 0.0
                                       ? p_source.nominal_voltage * RECUPERATION_VOLTAGE_FACTOR
                                       : p_source.nominal_voltage;
        /* An exact comparison like the original's TotalPreviousAdmitance != 0.0 - the LEAKAGE_ADMITTANCE floor
         * the tick sets is within is_zero_approx()'s tolerance, and treating it as zero made every
         * such query return no current. */
        return p_state.total_previous_admittance != 0.0
                       ? nominal / (p_source.internal_resistance + (1.0 / p_state.total_previous_admittance))
                       : 0.0;
    }

    double MaszynaTractionServer::_source_current_get(
            const PowerSource &p_source, const SourceState &p_state, const double p_resistance) {
        if (p_state.fuse()) {
            return 0.0;
        }
        return _source_output_current(p_source, p_state) / (p_resistance * p_state.total_previous_admittance);
    }

    void
    MaszynaTractionServer::_source_draw(const PowerSource &p_source, SourceState &p_state, const double p_resistance) {
        if (p_state.fuse()) {
            if (p_resistance < FUSE_LOAD_RESISTANCE) {
                p_state.fuse_timer = 0.0;
            }
            return;
        }
        if (!Math::is_zero_approx(p_resistance)) {
            p_state.total_admittance += 1.0 / p_resistance;
        }
        p_state.total_current = _source_output_current(p_source, p_state);
    }

    double MaszynaTractionServer::_resistivity(const Wire &p_wire) {
        // Traction.cpp:107-112 - the data gives Ohm/km, the network works in Ohm/m
        return (Math::is_equal_approx(p_wire.resistivity, LEGACY_RESISTIVITY) ? DEFAULT_RESISTIVITY
                                                                              : p_wire.resistivity) /
               Units::METRES_PER_KILOMETRE;
    }

    void MaszynaTractionServer::network_build() {
        _resolve_power_sources();
        _connect_wires();
        _resolve_parallel_spans();
        _mark_section_ends();
        _propagate_resistance();
        _connect_section_ends();
    }

    /* Port of TTraction::WhereIs() (Traction.cpp:392), run over every span once the chain exists.
     * The original computes it lazily per span; here it is one pass, which is the same answer
     * because nothing links spans after network_build(). */
    void MaszynaTractionServer::_mark_section_ends() {
        for (const KeyValue<RID, Wire> &entry: _get_wires()) {
            WireNetwork &network = networks[entry.key];
            for (int end = 0; end < 2; ++end) {
                const WireNetwork *neighbour = networks.getptr(network.next[end]);
                if (neighbour == nullptr) {
                    // no neighbour on this end: this span ends the section
                    network.last_flags |= LAST_SPAN;
                    continue;
                }
                // the neighbour's far end is open, so this span is the second to last
                if (!neighbour->next[1 - network.next_endpoint[end]].is_valid()) {
                    network.last_flags |= SECOND_LAST_SPAN;
                }
            }
        }
    }

    /* Port of the parallel half of traction_table::InitTraction() (Traction.cpp:830-856). The
     * original builds a ring of the spans sharing one running and uses it for two things: dimming
     * them when drawn, and keeping a pantograph from trusting hvNext among them. Only the second
     * is ported - a pantograph needs to know that a sibling it cannot reach along the chain may be
     * the wire actually overhead, and that is a flag, not a ring (the ring is in `TODO.md`). */
    void MaszynaTractionServer::_resolve_parallel_spans() {
        HashMap<String, RID> by_parallel_name;
        for (const KeyValue<RID, Wire> &entry: _get_wires()) {
            if (!entry.value.parallel_name.is_empty()) {
                by_parallel_name[entry.value.parallel_name] = entry.key;
            }
        }
        for (const KeyValue<RID, Wire> &entry: _get_wires()) {
            const Wire &wire = entry.value;
            if (wire.parallel_name.is_empty()) {
                continue;
            }
            WireNetwork &network = networks[entry.key];
            if (wire.parallel_name == "none" || wire.parallel_name == "*") {
                // the author says there is a parallel span but does not name it: search anyway
                network.last_flags |= SECOND_LAST_SPAN;
                continue;
            }
            network.has_parallel = true;
        }
        // the span that was named shares the running just as much as the one naming it
        for (const KeyValue<String, RID> &entry: by_parallel_name) {
            if (WireNetwork *named = networks.getptr(entry.value); named != nullptr) {
                named->has_parallel = true;
            }
        }
    }

    void MaszynaTractionServer::_resolve_power_sources() {
        for (const KeyValue<RID, Wire> &entry: _get_wires()) {
            const Wire &wire = entry.value;
            WireNetwork &network = networks[entry.key];
            if (wire.power_supply_name.is_empty() || wire.power_supply_name == "*" ||
                wire.power_supply_name == "none") {
                continue;
            }
            RID source_rid = power_source_get_rid_by_name(wire.power_supply_name);
            if (!source_rid.is_valid()) {
                UtilityFunctions::push_warning(
                        vformat("Bad scenario: traction piece connected to nonexistent power source \"%s\"",
                                wire.power_supply_name));
                source_rid = power_source_create();
                // TTractionPowerSource::Init() sets the voltage and current, the rest keep their defaults
                // (Traction.cpp:715-720, TractionPower.cpp:27-32)
                power_source_set_params(
                        source_rid, wire.power_supply_name, wire.nominal_voltage, 0.0, DEFAULT_INTERNAL_RESISTANCE,
                        wire.max_current, DEFAULT_FAST_FUSE_TIMEOUT, DEFAULT_FAST_FUSE_REPETITION,
                        DEFAULT_SLOW_FUSE_TIMEOUT, false, true, false);
            }
            /* A section is membership, not a supply: the spans of a section take their power
             * through the network from whatever substation feeds it, which is what
             * TTraction::PowerSet() splits on (Traction.cpp:460). Only a substation makes a span
             * directly powered. Where the data declares both under one name - and this datapack
             * declares every supply twice, as `section` and then as a substation - the last one
             * wins, the way the original's own name table resolves a duplicate (Names.h:38). */
            const PowerSource *source = _get_power_source(source_rid);
            if (source != nullptr && source->is_section) {
                network.section = source_rid;
                continue;
            }
            network.power_source = source_rid;
            network.power_near[0] = source_rid;
            network.power_near[1] = source_rid;
            network.resistance[0] = 0.0;
            network.resistance[1] = 0.0;
        }
    }

    void MaszynaTractionServer::_connect_wires() {
        struct Endpoint {
                RID wire;
                int endpoint_index = 0;
                Vector3 position;
        };
        Vector<Endpoint> endpoints;
        for (const KeyValue<RID, Wire> &entry: _get_wires()) {
            endpoints.push_back({entry.key, 0, entry.value.p1});
            endpoints.push_back({entry.key, 1, entry.value.p2});
        }

        HashMap<Vector2i, Vector<int>> cells;
        for (int index = 0; index < endpoints.size(); ++index) {
            const Vector3 &position = endpoints[index].position;
            const Vector2i cell(
                    static_cast<int32_t>(Math::floor(position.x / WIRE_JOIN_EPSILON)),
                    static_cast<int32_t>(Math::floor(position.z / WIRE_JOIN_EPSILON)));
            cells[cell].push_back(index);
        }

        for (int i = 0; i < endpoints.size(); ++i) {
            const Endpoint &ep1 = endpoints[i];
            WireNetwork *wire1 = networks.getptr(ep1.wire);
            if (wire1 == nullptr || wire1->next[ep1.endpoint_index].is_valid()) {
                continue;
            }
            const Vector2i cell(
                    static_cast<int32_t>(Math::floor(ep1.position.x / WIRE_JOIN_EPSILON)),
                    static_cast<int32_t>(Math::floor(ep1.position.z / WIRE_JOIN_EPSILON)));
            int best_index = -1;
            double best_distance = INFINITY;
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dz = -1; dz <= 1; ++dz) {
                    const Vector<int> *bucket = cells.getptr(cell + Vector2i(dx, dz));
                    if (bucket == nullptr) {
                        continue;
                    }
                    for (int j: *bucket) {
                        if (j == i || endpoints[j].wire == ep1.wire) {
                            continue;
                        }
                        const WireNetwork *wire2 = networks.getptr(endpoints[j].wire);
                        if (wire2 == nullptr || wire2->next[endpoints[j].endpoint_index].is_valid()) {
                            continue;
                        }
                        /* Per axis, like the original's epsilonEqual (Traction.cpp:355) - a
                         * euclidean radius accepts a pair that is off by the tolerance on every
                         * axis at once, which is not the same point. */
                        const Vector3 offset = ep1.position - endpoints[j].position;
                        if (Math::abs(offset.x) > WIRE_JOIN_EPSILON || Math::abs(offset.y) > WIRE_JOIN_EPSILON ||
                            Math::abs(offset.z) > WIRE_JOIN_EPSILON) {
                            continue;
                        }
                        const double distance = offset.length();
                        if (distance < best_distance) {
                            best_distance = distance;
                            best_index = j;
                        }
                    }
                }
            }
            if (best_index < 0) {
                continue;
            }
            const Endpoint &ep2 = endpoints[best_index];
            WireNetwork *wire2 = networks.getptr(ep2.wire);
            wire1->next[ep1.endpoint_index] = ep2.wire;
            wire1->next_endpoint[ep1.endpoint_index] = ep2.endpoint_index;
            wire2->next[ep2.endpoint_index] = ep1.wire;
            wire2->next_endpoint[ep2.endpoint_index] = ep1.endpoint_index;
        }
    }

    /* Port of TTraction::ResistanceCalc(), walking outward from every directly powered wire along
     * the hvNext chain in both directions, until it reaches a wire a nearer source already took. */
    void MaszynaTractionServer::_propagate_resistance() {
        Vector<RID> powered;
        for (const KeyValue<RID, WireNetwork> &entry: networks) {
            if (entry.value.power_source.is_valid()) {
                powered.push_back(entry.key);
            }
        }
        for (const RID &wire_rid: powered) {
            const Wire *wire = _get_wire(wire_rid);
            const WireNetwork &network = networks[wire_rid];
            const double half_resistance = 0.5 * _resistivity(*wire) * wire->length();
            _resistance_walk(wire_rid, 0, half_resistance, network.power_source);
            _resistance_walk(wire_rid, 1, half_resistance, network.power_source);
        }
    }

    void MaszynaTractionServer::_resistance_walk(
            const RID &p_from_wire, const int p_direction, const double p_resistance, const RID &p_source) {
        RID current_wire_rid = p_from_wire;
        int current_direction = p_direction;
        double current_resistance = p_resistance;
        while (true) {
            const WireNetwork *current_wire = networks.getptr(current_wire_rid);
            if (current_wire == nullptr) {
                return;
            }
            const RID next_rid = current_wire->next[current_direction];
            if (!next_rid.is_valid()) {
                return;
            }
            WireNetwork *next_network = networks.getptr(next_rid);
            const Wire *next_wire = _get_wire(next_rid);
            const int next_direction = current_wire->next_endpoint[current_direction];
            if (next_network == nullptr || next_wire == nullptr || next_direction < 0 ||
                next_network->power_near[next_direction].is_valid()) {
                return;
            }
            next_network->power_near[next_direction] = p_source;
            next_network->resistance[next_direction] = current_resistance;
            current_resistance += _resistivity(*next_wire) * next_wire->length();
            current_wire_rid = next_rid;
            current_direction = next_direction ^ 1;
        }
    }

    /* A chain of spans that shares no end with a powered one - the wire over a diverging track,
     * starting beside the main line's span rather than at its end - is fed across the overlap:
     * each open end of a section takes the power of the nearest powered span of the same section
     * (basic_cell::find(), scene.cpp:579) and walks it along its own chain. Repeated while it
     * connects anything, since a newly fed chain can feed the next one (Traction.cpp:858-895). */
    void MaszynaTractionServer::_connect_section_ends() {
        Vector<RID> ends;
        for (const KeyValue<RID, WireNetwork> &entry: networks) {
            if ((entry.value.last_flags & LAST_SPAN) != 0) {
                ends.push_back(entry.key);
            }
        }
        bool connected = true;
        while (connected) {
            connected = false;
            for (int index = 0; index < ends.size(); ++index) {
                WireNetwork *end = networks.getptr(ends[index]);
                const Wire *end_wire = _get_wire(ends[index]);
                if (end == nullptr || end_wire == nullptr) {
                    continue;
                }
                // the chain goes on from one side, the search starts at the open other one
                int direction = 0;
                Vector3 point = end_wire->p2;
                if (end->next[1].is_valid() && !end->next[0].is_valid()) {
                    direction = 1;
                    point = end_wire->p1;
                } else if (!end->next[0].is_valid()) {
                    // a lone span is not connected, as in the original (Traction.cpp:889)
                    ends.write[index] = RID();
                    continue;
                }

                const Vector3 end_vector = end_wire->p2 - end_wire->p1;
                const Vector2 query_center(point.x, point.z);
                const Rect2 query_aabb(
                        query_center - (Vector2(1.0, 1.0) * SECTION_END_SEARCH_RADIUS),
                        Vector2(1.0, 1.0) * SECTION_END_SEARCH_RADIUS * 2.0);
                const TypedArray<RID> candidates = _query_wires(query_aabb);
                RID match_source;
                double match_resistance = 0.0;
                double best_distance = SECTION_END_SEARCH_RADIUS * SECTION_END_SEARCH_RADIUS;
                for (int candidate_index = 0; candidate_index < candidates.size(); ++candidate_index) {
                    const RID candidate_rid = candidates[candidate_index];
                    if (candidate_rid == ends[index] || candidate_rid == end->next[0] ||
                        candidate_rid == end->next[1]) {
                        continue;
                    }
                    const WireNetwork *candidate = networks.getptr(candidate_rid);
                    const Wire *candidate_wire = _get_wire(candidate_rid);
                    if (candidate == nullptr || candidate_wire == nullptr || candidate->section != end->section) {
                        continue;
                    }
                    const int endpoint = (candidate_wire->p2 - candidate_wire->p1).dot(end_vector) >= 0.0
                                                 ? direction ^ 1
                                                 : direction;
                    if (!candidate->power_near[endpoint].is_valid() || candidate->resistance[endpoint] < 0.0) {
                        continue;
                    }
                    // the original measures to the span's location, its midpoint (Traction.cpp:156)
                    const double distance =
                            ((candidate_wire->p1 + candidate_wire->p2) * 0.5).distance_squared_to(point);
                    if (distance < best_distance) {
                        best_distance = distance;
                        match_source = candidate->power_near[endpoint];
                        match_resistance = candidate->resistance[endpoint];
                    }
                }
                if (!match_source.is_valid()) {
                    continue;
                }
                // ResistanceCalc(d, r, ps): the end itself learns the source on its open side
                end->power_near[direction ^ 1] = match_source;
                _resistance_walk(ends[index], direction, match_resistance, match_source);
                ends.write[index] = RID();
                connected = true;
            }
        }
    }

    /* Port of TTraction::VoltageGet(u, i), Traction.cpp:470: the sources the span is fed from
     * and the resistance each of them sees, for a load of p_resistance on the span. */
    int MaszynaTractionServer::_wire_feeds(
            const WireNetwork &p_network, const double p_resistance, Feed (&p_feeds)[2]) const {
        if (_get_power_source(p_network.power_source) != nullptr) {
            // directly powered - taken straight from the substation, no star network
            if (Math::is_zero_approx(p_resistance)) {
                return 0;
            }
            p_feeds[0] = {p_network.power_source, p_resistance};
            return 1;
        }
        const bool near0 = _get_power_source(p_network.power_near[0]) != nullptr;
        const bool near1 = _get_power_source(p_network.power_near[1]) != nullptr;
        const SourceState *state0 = source_states.getptr(p_network.power_near[0]);
        const SourceState *state1 = source_states.getptr(p_network.power_near[1]);
        if ((state0 != nullptr && state0->fuse()) || (state1 != nullptr && state1->fuse())) {
            // if either supply is out, so is this span
            return 0;
        }
        const double r0 = p_network.resistance[0];
        const double r1 = p_network.resistance[1];
        if (near0 && near1) {
            if (r0 > 0.0 && r1 > 0.0) {
                // fed from both ends: the triangle res/r0/r1 read as a star
                p_feeds[0] = {p_network.power_near[0], p_resistance + r0 + ((p_resistance * r0) / r1)};
                p_feeds[1] = {p_network.power_near[1], p_resistance + r1 + ((p_resistance * r1) / r0)};
                return 2;
            }
            if (r0 >= 0.0) {
                p_feeds[0] = {p_network.power_near[0], p_resistance + r0};
                return 1;
            }
            if (r1 >= 0.0) {
                p_feeds[0] = {p_network.power_near[1], p_resistance + r1};
                return 1;
            }
            return 0;
        }
        if (near0 && r0 >= 0.0) {
            p_feeds[0] = {p_network.power_near[0], p_resistance + r0};
            return 1;
        }
        if (near1 && r1 >= 0.0) {
            p_feeds[0] = {p_network.power_near[1], p_resistance + r1};
            return 1;
        }
        // in a section, but nothing along it reaches this span
        return 0;
    }

    double MaszynaTractionServer::_load_resistance(const double p_assumed_voltage, const double p_current) {
        return !Math::is_zero_approx(p_current) ? p_assumed_voltage / p_current : NO_LOAD_RESISTANCE;
    }

    double MaszynaTractionServer::wire_get_voltage(
            const RID &p_wire, const double p_assumed_voltage, const double p_current) const {
        const Wire *wire = _get_wire(p_wire);
        if (wire == nullptr) {
            return 0.0;
        }
        const WireNetwork *network = networks.getptr(p_wire);
        if (network == nullptr || (!network->section.is_valid() && !network->power_source.is_valid())) {
            // belongs to no section and is fed by nothing: the span's own nominal voltage stands
            return wire->nominal_voltage;
        }
        const double res = _load_resistance(p_assumed_voltage, p_current);
        Feed feeds[2];
        const int count = _wire_feeds(*network, res, feeds);
        double current = 0.0;
        for (int index = 0; index < count; ++index) {
            const SourceState *state = source_states.getptr(feeds[index].source);
            current += _source_current_get(
                    *_get_power_source(feeds[index].source), state != nullptr ? *state : SourceState(),
                    feeds[index].resistance);
        }
        return current * res;
    }

    void MaszynaTractionServer::wire_draw_current(
            const RID &p_wire, const double p_assumed_voltage, const double p_current) {
        const WireNetwork *network = networks.getptr(p_wire);
        if (network == nullptr) {
            return;
        }
        Feed feeds[2];
        const int count = _wire_feeds(*network, _load_resistance(p_assumed_voltage, p_current), feeds);
        for (int index = 0; index < count; ++index) {
            _source_draw(
                    *_get_power_source(feeds[index].source), source_states[feeds[index].source],
                    feeds[index].resistance);
        }
    }

    Dictionary MaszynaTractionServer::wire_find_above_with_height(
            const Vector3 &p_position, const Vector3 &p_up, const Vector3 &p_forward, const Vector3 &p_left,
            const double p_width, const double p_horn_width) const {
        const Vector2 query_center(p_position.x, p_position.z);
        const Rect2 query_aabb(
                query_center - (Vector2(1.0, 1.0) * QUERY_MARGIN), Vector2(1.0, 1.0) * QUERY_MARGIN * 2.0);
        RID best_rid;
        double best_height = INFINITY;
        const TypedArray<RID> candidates = _query_wires(query_aabb);
        for (int index = 0; index < candidates.size(); ++index) {
            const RID wire_rid = candidates[index];
            const Wire *wire = _get_wire(wire_rid);
            if (wire == nullptr) {
                continue;
            }
            const double vertical =
                    _wire_height_above(*wire, p_position, p_up, p_forward, p_left, p_width, p_horn_width);
            if (vertical < best_height) {
                best_height = vertical;
                best_rid = wire_rid;
            }
        }
        Dictionary result;
        result["rid"] = best_rid;
        result["height"] = best_height;
        return result;
    }

    Dictionary MaszynaTractionServer::wire_follow_above(
            const RID &p_from_wire, const Vector3 &p_position, const Vector3 &p_up, const Vector3 &p_forward,
            const Vector3 &p_left, const double p_width, const double p_horn_width) const {
        Dictionary result;
        result["rid"] = RID();
        result["height"] = INFINITY;

        RID wire_rid = p_from_wire;
        for (int hop = 0; hop < MAX_WIRE_HOPS; ++hop) {
            const Wire *wire = _get_wire(wire_rid);
            const WireNetwork *network = networks.getptr(wire_rid);
            if (wire == nullptr || network == nullptr) {
                return result;
            }
            /* At the end of a section, and among spans sharing a running, the chain is not the
             * whole story - the wire actually overhead may be one this span does not point to.
             * The original gives up on the chain here and looks around, whether or not the
             * current span would still do (DynObj.cpp:8747-8757). */
            if ((network->last_flags & LAST_SPAN_FLAGS) != 0 || network->has_parallel) {
                return result;
            }
            const Vector3 parametric = wire->p2 - wire->p1;
            const double front_dot = parametric.dot(p_forward);
            if (Math::is_zero_approx(front_dot)) {
                return result;
            }
            /* Which end the pantograph ran off decides which neighbour it ran onto - the two
             * ends of a span are hvNext[0] and hvNext[1] of the original's own list. */
            const double t = -(wire->p1.dot(p_forward) - p_position.dot(p_forward)) / front_dot;
            const double t_tolerance = SPAN_END_TOLERANCE / Math::abs(front_dot);
            if (t < -t_tolerance) {
                wire_rid = network->next[0];
                continue;
            }
            if (t > 1.0 + t_tolerance) {
                wire_rid = network->next[1];
                continue;
            }
            const double vertical =
                    _wire_height_above(*wire, p_position, p_up, p_forward, p_left, p_width, p_horn_width);
            if (!Math::is_finite(vertical)) {
                /* Under the span but out of the collector's reach sideways. The original gives up
                 * on the chain here too and searches the area, because a parallel span it cannot
                 * see from this one may be the wire actually overhead (DynObj.cpp:8791). */
                return result;
            }
            result["rid"] = wire_rid;
            result["height"] = vertical;
            return result;
        }
        return result;
    }

    double MaszynaTractionServer::_wire_height_above(
            const Wire &p_wire, const Vector3 &p_position, const Vector3 &p_up, const Vector3 &p_forward,
            const Vector3 &p_left, const double p_width, const double p_horn_width) {
        const Vector3 parametric = p_wire.p2 - p_wire.p1;
        const double front_dot = parametric.dot(p_forward);
        if (Math::is_zero_approx(front_dot)) {
            return INFINITY;
        }
        const double t = -(p_wire.p1.dot(p_forward) - p_position.dot(p_forward)) / front_dot;
        // Confirmed live: a test caught EP07-424 crossing td.scn losing pantograph voltage for a
        // frame at one fixed point, which the Mover's NoVoltRelay read as a real loss of contact.
        const double t_tolerance = SPAN_END_TOLERANCE / Math::abs(front_dot);
        if (t < -t_tolerance || t > 1.0 + t_tolerance) {
            return INFINITY;
        }
        const Vector3 contact_point = p_wire.p1 + (parametric * static_cast<real_t>(t));
        const Vector3 to_contact = contact_point - p_position;
        const double vertical = to_contact.dot(p_up);
        if (vertical < 0.0) {
            return INFINITY;
        }
        /* Beyond the slider the wire is not lost: it runs onto the horn, which leads it back
         * (scene.cpp:105-112). A wire over the horn is counted as geometrically higher, rising to
         * HORN_CONTACT_RISE at the horn's tip, so a span properly overhead still wins against it. */
        const double lateral = Math::abs(to_contact.dot(p_left)) - p_width;
        if (lateral <= 0.0) {
            return vertical;
        }
        if (lateral >= p_horn_width) {
            return INFINITY;
        }
        return vertical + (HORN_CONTACT_RISE * lateral / p_horn_width);
    }
} // namespace godot
