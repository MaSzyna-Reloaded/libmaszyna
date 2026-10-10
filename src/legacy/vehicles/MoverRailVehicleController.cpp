#include "MaszynaMoverVehicleServer.hpp"
#include "MoverRailVehicleController.hpp"
#include "legacy/maszyna-mover/utilities.h"
#include "legacy/vehicles/MoverBackend.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include <algorithm>
#include <cmath>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <tuple>

namespace godot {
    // the wrapper's coupling vocabulary is the original's, value for value - the Mover takes it as is
    static_assert(static_cast<int>(RailVehicleController::COUPLER_END_FRONT) == static_cast<int>(end::front));
    static_assert(static_cast<int>(RailVehicleController::COUPLER_END_REAR) == static_cast<int>(end::rear));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_COUPLER) == static_cast<int>(coupling::coupler));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_BRAKEHOSE) == static_cast<int>(coupling::brakehose));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_CONTROL) == static_cast<int>(coupling::control));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_HIGHVOLTAGE) ==
            static_cast<int>(coupling::highvoltage));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_GANGWAY) == static_cast<int>(coupling::gangway));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_MAINHOSE) == static_cast<int>(coupling::mainhose));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_HEATING) == static_cast<int>(coupling::heating));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_PERMANENT) == static_cast<int>(coupling::permanent));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_POWER_24V) == static_cast<int>(coupling::power24v));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_POWER_110V) == static_cast<int>(coupling::power110v));
    static_assert(
            static_cast<int>(RailVehicleController::COUPLING_FLAG_POWER_3X400V) ==
            static_cast<int>(coupling::power3x400v));

    void MoverRailVehicleController::_bind_methods() {}

    /* release() runs from VehicleController's NOTIFICATION_PREDELETE, which the editor skips; the
     * Mover must not outlive the vehicle either way. */
    MoverRailVehicleController::MoverRailVehicleController() {
        set_implementation(MaszynaMoverVehicleServer::IMPLEMENTATION_NAME);
    }

    MoverRailVehicleController::~MoverRailVehicleController() {
        if (MaszynaMoverVehicleServer *implementation = _mover_implementation(); implementation != nullptr) {
            implementation->mover_free(mover_vehicle);
        }
    }

    MaszynaMoverVehicleServer *MoverRailVehicleController::_mover_implementation() const {
        return Object::cast_to<MaszynaMoverVehicleServer>(ObjectDB::get_instance(mover_implementation));
    }

    Ref<RailVehicleController> MoverRailVehicleController::_controller_of(const TMoverParameters *p_mover) const {
        const MaszynaMoverVehicleServer *implementation = _mover_implementation();
        const VehicleServer *vehicles = VehicleServer::get_instance();
        if (implementation == nullptr || vehicles == nullptr || p_mover == nullptr) {
            return Ref<RailVehicleController>();
        }
        return Object::cast_to<RailVehicleController>(ObjectDB::get_instance(
                ObjectID(vehicles->vehicle_get_controller_instance_id(implementation->mover_get_vehicle(p_mover)))));
    }

    TMoverParameters *MoverRailVehicleController::get_mover() const {
        return mover;
    }

    RailVehicleController::CouplerEnd MoverRailVehicleController::get_coupled_end(const CouplerEnd p_end) const {
        ERR_FAIL_COND_V(mover == nullptr || mover->Couplers[p_end].Connected == nullptr, COUPLER_END_FRONT);
        return static_cast<CouplerEnd>(mover->Couplers[p_end].ConnectedNr);
    }

    Ref<RailVehicleController> MoverRailVehicleController::get_coupled_controller(const CouplerEnd p_end) const {
        if (mover == nullptr) {
            return Ref<RailVehicleController>();
        }
        return _controller_of(mover->Couplers[p_end].Connected);
    }

    void MoverRailVehicleController::initialize_mover_state() {
        const bool driver_active = get_initial_velocity() != 0.0;

        mover->MainCtrlPos = mover->MainCtrlNoPowerPos();
        mover->LocalBrakePosA = 0.0;
        // CheckLocomotiveParameters() puts the handle (BrakeCtrlPos, BrakeCtrlPosR) but not
        // fBrakeCtrlPos, which BrakeLevelSet() compares with: set up a second time, it would find
        // the position unchanged and leave the handle at lap (FINDINGS.md, 2026-09-26)
        mover->fBrakeCtrlPos = mover->BrakeCtrlPosR;
        mover->BrakeLevelSet(
                std::floor(mover->Handle->GetPos(
                        driver_active && get_driver_cabin_kind() != RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE
                                ? bh_RP
                                : bh_NP)));
    }

    void MoverRailVehicleController::_initialize_simulation() {
        MaszynaMoverVehicleServer *implementation = MaszynaMoverVehicleServer::get_instance();
        ERR_FAIL_NULL(implementation);
        // the vehicle used to be named by its node; what identifies one now is its train id
        mover_vehicle = get_rid();
        mover_implementation = ObjectID(implementation->get_instance_id());
        mover = implementation->mover_create(
                mover_vehicle, get_initial_velocity(), get_type_name(), get_vehicle_id(),
                cab_occupied(get_driver_cabin_kind())); // the cab as TMoverParameters::CabActivisation counts it
        ERR_FAIL_NULL(mover);
        // every component takes the Mover before the configuration is written into it
        _attach_implementation(mover_implementation);

        // Original engine: TDynamicObject::Init() (DynObj.cpp:2020-2075) - every value of the FIZ
        // (LoadFIZ()), the load and the mass, then CheckLocomotiveParameters() once, then the
        // controller and the brake handle; nothing of the configuration is written after it, or it
        // undoes what CheckLocomotiveParameters() derived (the spring brake of a standing vehicle,
        // the brake's load flag and delays)
        apply_configuration();
        /* What the scenery loaded the vehicle with. The backend takes the cargo's name and its
         * amount together and reads more than cargo out of them - `pantstate` is how a scenery
         * starts a locomotive with raised pantographs (Mover.cpp:7647). */
        if (!get_load_name().is_empty()) {
            mover->AssignLoad(std::string(get_load_name().utf8().ptr()), static_cast<float>(get_load_amount()));
        }
        mover->ComputeMass();
        // the original's Dir (velocity sign x cab x direction, DynObj.cpp:2036-2039) sets DirActive
        // of a vehicle moving at load; here no cab is active yet (see below), so it is 0
        mover->CheckLocomotiveParameters(get_initial_velocity() != 0.0, 0);
        initialize_mover_state();

        // Original engine: Load() (Mover.cpp:11692) calls ComputeConstans() once, after every
        // physical parameter (TotalMass, Dim, Cx, BearingType, NPoweredAxles, TrackW - all
        // already applied above by apply_configuration()) is settled -
        // it derives FrictConst1/FrictConst2s/FrictConst2d, the per-vehicle rolling/air-drag
        // resistance coefficients FrictionForce() (called every tick from ComputeTotalForce())
        // actually uses. Never called anywhere else in the original either (a single call at
        // load time is correct - the original itself never updates curve-dependent resistance
        // terms after that point). Without this, every one of this wrapper's vehicles ran with
        // zero rolling/air resistance: free acceleration to unrealistic speeds and near-zero
        // coasting deceleration, since FrictConst1/2s/2d all silently stayed at their
        // compiled-zero defaults.
        mover->ComputeConstans();

        // the cab a driver sits in is the one RailVehicleServer handed down, and nobody's is 0 -
        // mover_create() above took it (DynObj.cpp:1948-1964): an unmanned car of a unit is driven over its couplers
        // and its own brake valve leaves the pipe alone (bom_PS, Mover.cpp:4548)
        // no cab is active yet (CabActive = 0, MOVER.h:2090): the driver switches it on once the
        // trainset is coupled - the AI by its hint (driverhints.cpp:108), the player on entering
        // (Train.cpp:9147) - so the activation reaches every cab of the unit (SendCtrlToNext)

        /* switch_physics() raczej trzeba zostawic */
        mover->switch_physics(true);

        DEBUG("[MaSzyna::TMoverParameters] Mover initialized successfully");
        emit_signal(simulation_initialized_signal);
    }

    /* The base lets go of the components and the registration; the Mover goes last. A neighbour
     * still coupled to it is let go first: its coupler would point at a Mover that is gone. The
     * original never deletes a vehicle - it takes the whole trainset out of the simulation
     * (vehicle_table::erase_disabled(), DynObj.cpp:8837) - so it has no counterpart; the coupler is
     * cleared the way Dettach() clears a coupling at pressed buffers (Mover.cpp:634). */
    void MoverRailVehicleController::release() {
        VehicleController::release();
        MaszynaMoverVehicleServer *implementation = _mover_implementation();
        // freed at shutdown before this controller, the server took every Mover with it
        if (implementation == nullptr) {
            mover = nullptr;
            mover_implementation = ObjectID();
            return;
        }
        if (mover == nullptr) {
            return;
        }
        for (TCoupling &coupler: mover->Couplers) {
            if (coupler.Connected == nullptr) {
                continue;
            }
            TCoupling &other_coupler = coupler.Connected->Couplers[coupler.ConnectedNr];
            std::tie(other_coupler.Connected, other_coupler.ConnectedNr, other_coupler.CouplingFlag) =
                    std::make_tuple(nullptr, -1, coupling::faux);
            if (const Ref<RailVehicleController> neighbour = _controller_of(coupler.Connected); neighbour.is_valid()) {
                neighbour->emit_signal(trainset_changed_signal);
            }
        }
        implementation->mover_free(mover_vehicle);
        mover = nullptr;
        mover_vehicle = RID();
        mover_implementation = ObjectID();
    }

    bool MoverRailVehicleController::is_simulation_ready() const {
        return mover != nullptr;
    }

    /* Whether the vehicle still has anything to integrate. A braked standing vehicle stays active
     * in the original too (Mover.cpp:4603). */
    void MoverRailVehicleController::wake() {
        ERR_FAIL_NULL(mover);
        mover->switch_physics(true);
    }

    bool MoverRailVehicleController::is_physics_active() const {
        return mover != nullptr && mover->PhysicActivation;
    }

    // Original engine: TDynamicObject::Move sets Loc = {-x, z, y} (DynObj.cpp:2334); dMoveLen collects the
    // movement of one simulation frame and is reset after it (ResetdMoveLen, DynObj.cpp:3473)
    void MoverRailVehicleController::update_location() {
        if (mover == nullptr) {
            return;
        }
        const Vector3 position = get_world_position();
        mover->Loc = {-position.x, position.z, position.y};
        mover->dMoveLen = 0.0;
    }

    // Original engine: TDynamicObject::update_neighbours() (DynObj.cpp:7544) - a physical connection
    // with another vehicle locks down the collision source on this end
    bool MoverRailVehicleController::_neighbour_from_coupler(const CouplerEnd p_end) {
        const TCoupling &coupler = mover->Couplers[p_end];
        if (coupler.Connected == nullptr) {
            return false;
        }
        neighbour_data &neighbour = mover->Neighbours[p_end];
        neighbour.vehicle = coupler.Connected;
        neighbour.vehicle_end = coupler.ConnectedNr;
        neighbour.distance = static_cast<float>(
                TMoverParameters::CouplerDist(mover, coupler.Connected) - coupler.adapter_length -
                coupler.Connected->Couplers[coupler.ConnectedNr].adapter_length);
        return true;
    }

    // Original engine: TDynamicObject::update_neighbours() (DynObj.cpp:7544); the track scan itself
    // (find_vehicle) is done by RailVehicleServer, which passes the center to center track distance
    void MoverRailVehicleController::update_neighbour(
            const CouplerEnd p_end, const Ref<RailVehicleController> &p_other, const CouplerEnd p_other_end,
            const double p_track_distance) {
        // below this distance [m] the range between the couplers is measured directly (DynObj.cpp:7577)
        static constexpr double COUPLER_MEASURE_RANGE = 100.0;
        static constexpr double COUPLER_MEASURE_RANGE_ROAD = 50.0;
        // CategoryFlag of a road vehicle (MOVER.h:1102)
        static constexpr int CATEGORY_ROAD = 2;
        const MoverRailVehicleController *other = Object::cast_to<MoverRailVehicleController>(p_other.ptr());
        if (mover == nullptr || _neighbour_from_coupler(p_end)) {
            return;
        }
        neighbour_data &neighbour = mover->Neighbours[p_end];
        neighbour = neighbour_data();
        if (other == nullptr || other->mover == nullptr) {
            return;
        }
        TMoverParameters *other_mover = other->mover;
        const TCoupling &coupler = mover->Couplers[p_end];
        const TCoupling &other_coupler = other_mover->Couplers[p_other_end];
        neighbour.vehicle = other_mover;
        neighbour.vehicle_end = p_other_end;
        neighbour.distance = static_cast<float>(p_track_distance - (0.5 * (mover->Dim.L + other_mover->Dim.L)));
        const double measure_range =
                other_mover->CategoryFlag == CATEGORY_ROAD ? COUPLER_MEASURE_RANGE_ROAD : COUPLER_MEASURE_RANGE;
        if (neighbour.distance < static_cast<float>(measure_range)) {
            neighbour.distance = static_cast<float>(
                    TMoverParameters::CouplerDist(mover, other_mover) - coupler.adapter_length -
                    other_coupler.adapter_length);
        }
    }

    // Original engine: update_neighbours() with nothing found (DynObj.cpp:7544) - a coupled vehicle
    // stays the neighbour, anything else is forgotten
    void MoverRailVehicleController::clear_neighbour(const CouplerEnd p_end) {
        if (mover == nullptr || _neighbour_from_coupler(p_end)) {
            return;
        }
        mover->Neighbours[p_end] = neighbour_data();
    }

    void MoverRailVehicleController::compute_forces(const double p_delta) {
        if (mover == nullptr) {
            return;
        }
        mover->ComputeTotalForce(p_delta);
    }

    void MoverRailVehicleController::compute_movement(const double p_delta) {
        _integrate(p_delta, Integration::FULL);
    }

    /// The cheap movement of the intermediate physics iterations: the original runs UpdateForce +
    /// FastUpdate for every sub-iteration and the full Update() only once per frame
    /// (DynObj.cpp:8195-8210), where FastUpdate calls Mover::FastComputeMovement()
    /// (DynObj.cpp:4086) instead of the full ComputeMovement().
    void MoverRailVehicleController::compute_fast_movement(const double p_delta) {
        _integrate(p_delta, Integration::FAST);
    }

    void MoverRailVehicleController::_integrate(const double p_delta, const Integration p_integration) {
        // a standing vehicle switched off by ComputeTotalForce() is not moved at all
        // (DynObj.cpp:4059 FastUpdate, DynObj.cpp:2940 Update)
        if (mover == nullptr || !mover->PhysicActivation) {
            return;
        }
        TRotation rotation;
        if (p_integration == Integration::FULL) {
            mover->ComputeMovement(
                    p_delta, p_delta, mover->RunningShape, mover->RunningTrack, mover->RunningTraction, mover->Loc,
                    rotation);
        } else {
            mover->FastComputeMovement(p_delta, mover->RunningShape, mover->RunningTrack, mover->Loc, rotation);
        }
        // the vehicle is moved by this distance (DynObj.cpp:2439), front-relative
        mover->dMoveLen += mover->V * p_delta;
    }

    // Original engine: TDynamicObject::AttachNext() couples with Enforce, without sound (DynObj.cpp:2590)
    void MoverRailVehicleController::couple(
            const Ref<RailVehicleController> &p_other, const CouplerEnd p_end, const CouplerEnd p_other_end,
            const BitField<CouplingFlags> p_coupling) {
        MoverRailVehicleController *other = Object::cast_to<MoverRailVehicleController>(p_other.ptr());
        if (mover == nullptr || other == nullptr || other->mover == nullptr) {
            UtilityFunctions::push_error("Cannot couple vehicles without initialized movers.");
            return;
        }
        int coupling_type = static_cast<int>(p_coupling);
        // a coupler allowing only permanent coupling keeps it permanent (simulationstateserializer.cpp:990)
        if (coupling_type != coupling::faux && (mover->Couplers[p_end].AllowedFlag & coupling::permanent) != 0) {
            coupling_type |= coupling::permanent;
        }
        mover->Attach(p_end, p_other_end, other->mover, coupling_type, true, false);
        // the original re-inspects the trainset on a coupling change (CheckVehicles(), Driver.cpp:2622)
        emit_signal(trainset_changed_signal);
        other->emit_signal(trainset_changed_signal);
    }

    void MoverRailVehicleController::uncouple(const CouplerEnd p_end) {
        if (mover == nullptr || mover->Couplers[p_end].Connected == nullptr) {
            return;
        }
        const Ref<RailVehicleController> neighbour = get_coupled_controller(p_end);
        const bool detached = mover->Dettach(p_end);
        _consume_coupler_events();
        // Dettach() clears Connected on both ends before its event is consumed. Announce the other
        // end here, while its controller is still known (Mover.cpp:616-646).
        if (detached && neighbour.is_valid()) {
            neighbour->emit_signal(trainset_changed_signal);
        }
    }

    // TDynamicObject::attach_coupler_adapter() (DynObj.cpp:1754-1789): the vehicle beyond the end -
    // the one coupled at it for the scenery, the nearest one within reach otherwise - hands its
    // adapter; with room asked for, some has to be left; the end then couples as an automatic one
    bool MoverRailVehicleController::_fit_coupler_adapter(const CouplerEnd p_end, const bool p_with_room) {
        if (mover == nullptr) {
            return false;
        }
        const neighbour_data &neighbour = mover->Neighbours[p_end];
        const Ref<RailVehicleController> other =
                _controller_of(p_with_room ? neighbour.vehicle : mover->Couplers[p_end].Connected);
        if (other.is_null() || (p_with_room && neighbour.distance > COUPLER_ADAPTER_REACH)) {
            return false;
        }
        const double length = other->get_coupler_adapter_length();
        if (p_with_room && neighbour.distance - length < COUPLER_ADAPTER_ROOM) {
            return false;
        }
        TCoupling &coupler = mover->Couplers[p_end];
        coupler.adapter_type = TCouplerType::Automatic;
        coupler.adapter_length = length;
        coupler.adapter_height = other->get_coupler_adapter_height();
        fitted_adapter_models[p_end] = other->get_coupler_adapter_model();
        emit_signal(coupler_adapter_attached_signal, p_end);
        return true;
    }

    bool MoverRailVehicleController::is_coupler_automatic(const CouplerEnd p_end) const {
        return mover != nullptr && mover->Couplers[p_end].type() == TCouplerType::Automatic;
    }

    BitField<RailVehicleController::CouplingFlags>
    MoverRailVehicleController::get_coupler_joinable_flags(const CouplerEnd p_end) const {
        if (mover == nullptr || mover->Neighbours[p_end].vehicle == nullptr) {
            return COUPLING_FLAG_NONE;
        }
        const neighbour_data &neighbour = mover->Neighbours[p_end];
        const TCoupling &coupler = mover->Couplers[p_end];
        const TCoupling &other_coupler = neighbour.vehicle->Couplers[neighbour.vehicle_end];
        int64_t joinable = coupler.AllowedFlag & other_coupler.AllowedFlag;
        if (coupler.control_type != other_coupler.control_type) {
            joinable &= ~static_cast<int64_t>(coupling::control);
        }
        return joinable;
    }

    bool MoverRailVehicleController::coupler_adapter_attach(const Variant &p_where) {
        return mover != nullptr && _fit_coupler_adapter(_resolve_coupler_end(p_where), true);
    }

    bool MoverRailVehicleController::coupler_adapter_fit(const CouplerEnd p_end) {
        return _fit_coupler_adapter(p_end, false);
    }

    // TDynamicObject::remove_coupler_adapter() (DynObj.cpp:1791-1810)
    bool MoverRailVehicleController::coupler_adapter_remove(const Variant &p_where) {
        if (mover == nullptr) {
            return false;
        }
        const CouplerEnd end = _resolve_coupler_end(p_where);
        TCoupling &coupler = mover->Couplers[end];
        if (coupler.adapter_type == TCouplerType::NoCoupler) {
            return false;
        }
        if (coupler.Connected != nullptr) {
            uncouple(end);
        }
        coupler.adapter_type = TCouplerType::NoCoupler;
        coupler.adapter_length = 0.0;
        coupler.adapter_height = 0.0;
        fitted_adapter_models[end] = String();
        emit_signal(coupler_adapter_removed_signal, end);
        return true;
    }

    String MoverRailVehicleController::get_coupler_adapter_fitted_model(const CouplerEnd p_end) const {
        return fitted_adapter_models[p_end];
    }

    double MoverRailVehicleController::get_coupler_adapter_fitted_length(const CouplerEnd p_end) const {
        return mover != nullptr ? mover->Couplers[p_end].adapter_length : 0.0;
    }

    double MoverRailVehicleController::get_coupler_adapter_fitted_height(const CouplerEnd p_end) const {
        return mover != nullptr ? mover->Couplers[p_end].adapter_height : 0.0;
    }

    bool MoverRailVehicleController::is_coupled(const CouplerEnd p_end) const {
        return mover != nullptr && mover->Couplers[p_end].Connected != nullptr;
    }

    bool
    MoverRailVehicleController::is_coupled_by(const CouplerEnd p_end, const BitField<CouplingFlags> p_flags) const {
        return mover != nullptr && TestFlag(mover->Couplers[p_end].CouplingFlag, static_cast<int>(p_flags));
    }

    // p_where is a coupler end (0 front, 1 rear) or a world position - then the vehicle end nearest to
    // it is used, like the walk mode commands of the original (ABuScanNearestObject, Train.cpp:6213)
    RailVehicleController::CouplerEnd MoverRailVehicleController::_resolve_coupler_end(const Variant &p_where) const {
        if (p_where.get_type() != Variant::VECTOR3) {
            return static_cast<CouplerEnd>(CLAMP(static_cast<int>(p_where), COUPLER_END_FRONT, COUPLER_END_REAR));
        }
        const Transform3D transform = get_world_transform();
        // vehicles face -Z; the front coupler (end 0) is half the length ahead of the center
        const Vector3 front =
                transform.origin - transform.basis.get_column(2).normalized() * static_cast<real_t>(0.5 * mover->Dim.L);
        const Vector3 rear =
                transform.origin + transform.basis.get_column(2).normalized() * static_cast<real_t>(0.5 * mover->Dim.L);
        const Vector3 position = p_where;
        return position.distance_squared_to(front) <= position.distance_squared_to(rear) ? COUPLER_END_FRONT
                                                                                         : COUPLER_END_REAR;
    }

    // Original engine: TDynamicObject::couple() (DynObj.cpp:1509) - one more coupling type per call,
    // with the vehicle detected at that end
    void MoverRailVehicleController::coupler_connect(const Variant &p_where) {
        if (mover == nullptr) {
            return;
        }
        const CouplerEnd side = _resolve_coupler_end(p_where);
        const neighbour_data &neighbour = mover->Neighbours[side];
        if (neighbour.vehicle == nullptr) {
            return;
        }
        const TCoupling &coupler = mover->Couplers[side];
        const int64_t joinable = get_coupler_joinable_flags(side);

        if (coupler.CouplingFlag == coupling::faux && (joinable & coupling::coupler) == coupling::coupler &&
            mover->Attach(side, neighbour.vehicle_end, neighbour.vehicle, coupling::coupler)) {
            return;
        }
        for (const int flag:
             {coupling::brakehose, coupling::mainhose, coupling::control, coupling::gangway, coupling::heating}) {
            if ((coupler.CouplingFlag & flag) == flag || (joinable & flag) != flag) {
                continue;
            }
            if (mover->Attach(side, neighbour.vehicle_end, neighbour.vehicle, coupler.CouplingFlag | flag)) {
                return;
            }
        }
    }

    // Original engine: TDynamicObject::uncouple() (DynObj.cpp:1614)
    void MoverRailVehicleController::coupler_disconnect(const Variant &p_where) {
        if (mover == nullptr) {
            return;
        }
        const CouplerEnd side = _resolve_coupler_end(p_where);
        if (mover->DettachStatus(side) >= 0 || (mover->Couplers[side].CouplingFlag & coupling::permanent) != 0) {
            return;
        }
        uncouple(side);
    }

    void MoverRailVehicleController::update_state() {
        if (mover != nullptr) {
            _consume_coupler_events();
        }
    }

    // The flags are the original's coupling:: flags (Mover.cpp:590).
    // Original engine: coupler attach/detach sounds (DynObj.cpp:4855-4905) - each request of the mover
    // (TCoupling::sounds) bumps a counter the sound triggers play on; the flags are consumed as there.
    //
    // Consuming is a tick job, not a read job: this clears the mover's flags, so doing it while
    // filling the state dictionary made the events belong to whoever happened to read first.
    //
    // The coupler itself joining or parting is a trainset change, whichever way it came - a command,
    // an automatic coupler meeting another (Mover.cpp:4894), Dettach(). The flag is set on the
    // coupler that coupled only (Mover.cpp:593), so the vehicle it coupled to is told as well; a
    // parted one is no longer known here, but it was in the same trainset as this one.
    void MoverRailVehicleController::_consume_coupler_events() {
        // the coupling each attach sound of the original stands for (DynObj.cpp:4855-4905)
        static const std::pair<int, CouplingFlags> events[] = {
                {sound::attachcoupler, COUPLING_FLAG_COUPLER},   {sound::attachbrakehose, COUPLING_FLAG_BRAKEHOSE},
                {sound::attachmainhose, COUPLING_FLAG_MAINHOSE}, {sound::attachcontrol, COUPLING_FLAG_CONTROL},
                {sound::attachgangway, COUPLING_FLAG_GANGWAY},   {sound::attachheating, COUPLING_FLAG_HEATING}};
        bool trainset_changed = false;
        for (TCoupling &coupler: mover->Couplers) {
            if (coupler.sounds == sound::none) {
                continue;
            }
            const bool detaching = (coupler.sounds & sound::detach) != 0;
            if ((coupler.sounds & sound::attachcoupler) != 0) {
                trainset_changed = true;
                if (const Ref<RailVehicleController> neighbour = _controller_of(coupler.Connected);
                    !detaching && neighbour.is_valid()) {
                    neighbour->emit_signal(trainset_changed_signal);
                }
            }
            for (const auto &[event, flag]: events) {
                if ((coupler.sounds & event) != 0) {
                    emit_signal(detaching ? coupler_detached_signal : coupler_attached_signal, flag);
                }
            }
            coupler.sounds = sound::none;
        }
        if (trainset_changed) {
            emit_signal(trainset_changed_signal);
        }
    }

    double MoverRailVehicleController::process_movement(const double p_delta) {
        return mover != nullptr ? mover->V * p_delta : 0.0;
    }

    void MoverRailVehicleController::apply_config() {
        if (mover == nullptr) {
            UtilityFunctions::push_warning("VehicleController::apply_config() failed: internal mover not initialized");
            return;
        }
        mover->Mass = get_mass();
        mover->Power = get_power();
        mover->Vmax = get_max_velocity();
        mover->Mred = get_reduced_mass();

        mover->ComputeMass();

        mover->CategoryFlag = get_category();
        mover->TrainType = get_train_type();
        mover->SandCapacity = static_cast<int>(get_sand_capacity());
        mover->HeatingPower = get_heating_power();
        mover->LightPower = get_light_power();

        mover->Dim.L = get_dimensions_length();
        mover->Dim.H = get_dimensions_height();
        mover->Dim.W = get_dimensions_width();
        mover->Cx = get_dimensions_drag_coefficient();
        mover->Floor = static_cast<float>(get_dimensions_floor_height());

        mover->GroundRelayStart = MaszynaMoverVehicleServer::start_mode_to_mover(get_cntrl_ground_relay_start_mode());
        mover->CompartmentLights.start_type =
                MaszynaMoverVehicleServer::start_mode_to_mover(get_cntrl_compartment_lights_start_mode());
        mover->AutomaticCabActivation = get_cntrl_automatic_cab_activation();
        mover->InactiveCabFlag = get_cntrl_inactive_cab_flag();
        emit_config_changed();
    }

    /* An EZT's automatic start thresholds where the engine's Circuit: has not written its own
     * (LoadFIZ_Param, Mover.cpp:10300-10305) - a cab car has no engine to write them, and with
     * Imin == IminHi == 0 DirectionBackward() switches the high start off forever (Mover.cpp:3250).
     * Read after every component has applied its configuration, the engine's among them. */
    void MoverRailVehicleController::apply_vehicle_config() {
        if (mover == nullptr) {
            return;
        }
        if (mover->TrainType == Maszyna::dt_EZT && mover->IminLo == 0 && mover->IminHi == 0) {
            mover->IminLo = EZT_IMIN_LOW;
            mover->IminHi = EZT_IMIN_HIGH;
            mover->Imin = mover->IminLo;
        }
    }

    void MoverRailVehicleController::_fill_config_dictionary(Dictionary &p_config) const {
        if (mover == nullptr) {
            return;
        }
        // Vehicle-wide, not brake-specific - mover->Vmax is set from this same max_velocity
        // property (see apply_config() below), so this is a thin alias, not new derivation.
        p_config["max_speed"] = get_max_velocity();
        p_config["power"] = mover->Power;
        p_config["length"] = mover->Dim.L;
        p_config["train_type"] = get_train_type();
    }

    VehicleController::Direction MoverRailVehicleController::get_direction_absolute() const {
        return mover != nullptr ? static_cast<Direction>(mover->DirAbsolute) : DIRECTION_NEUTRAL;
    }

    int MoverRailVehicleController::get_train_damage() const {
        return mover != nullptr ? mover->DamageFlag : 0;
    }

    double MoverRailVehicleController::get_mass_reduced() const {
        return mover != nullptr ? mover->Mred : 0.0;
    }

    bool MoverRailVehicleController::get_coupler_stretched() const {
        return mover != nullptr && (mover->Couplers[end::front].stretch_duration > 0.0f ||
                                    mover->Couplers[end::rear].stretch_duration > 0.0f);
    }

    double MoverRailVehicleController::get_velocity() const {
        return mover != nullptr ? mover->V : 0.0;
    }

    double MoverRailVehicleController::get_speed() const {
        return mover != nullptr ? mover->Vel : 0.0;
    }

    double MoverRailVehicleController::get_acceleration() const {
        return mover != nullptr ? mover->AccS : 0.0;
    }

    double MoverRailVehicleController::get_mass_total() const {
        return mover != nullptr ? mover->TotalMass : 0.0;
    }

    double MoverRailVehicleController::get_total_distance() const {
        return mover != nullptr ? mover->DistCounter : 0.0;
    }

    VehicleController::Direction MoverRailVehicleController::get_direction() const {
        return mover != nullptr ? static_cast<Direction>(mover->DirActive) : DIRECTION_NEUTRAL;
    }

    void MoverRailVehicleController::compartment_lights(const bool p_enabled) const {
        ASSERT_MOVER(mover);
        mover->CompartmentLightsSwitch(p_enabled);
    }

    void MoverRailVehicleController::compartment_lights_switch_off(const bool p_enabled) const {
        ASSERT_MOVER(mover);
        mover->CompartmentLightsSwitchOff(p_enabled);
    }

    bool MoverRailVehicleController::get_compartment_lights_enabled() const {
        return mover != nullptr && mover->CompartmentLights.is_enabled;
    }

    bool MoverRailVehicleController::get_compartment_lights_active() const {
        return mover != nullptr && mover->CompartmentLights.is_active;
    }

    // Original engine: OnCommand_cabactivationenable/disable (Train.cpp:2430-2472)
    void MoverRailVehicleController::cab_activation(const bool p_enabled) const {
        ASSERT_MOVER(mover);
        if (p_enabled) {
            mover->CabActivisation();
            return;
        }
        mover->CabDeactivisation();
    }

    // Original engine: taking over a vehicle activates its cab if the FIZ allows automatic
    // activation (Train.cpp:9086, 9147); otherwise the driver uses cab_activation
    void MoverRailVehicleController::cab_activation_auto() const {
        ASSERT_MOVER(mover);
        mover->CabActivisationAuto(true);
    }

    // Original engine: TTrain::CabChange() (Train.cpp:10336) switches the cab off before the change
    void MoverRailVehicleController::cab_deactivation_auto() const {
        ASSERT_MOVER(mover);
        mover->CabDeactivisationAuto();
    }

    // Original engine: what TMoverParameters::ChangeCab() resets besides the cab (Mover.cpp:735-749);
    // the cab itself is the driver's, handed down by set_driver_cabin_kind()
    void MoverRailVehicleController::cab_controls_reset() const {
        ASSERT_MOVER(mover);
        if ((mover->BrakeCtrlPosNo > 0) && ((mover->BrakeSystem == TBrakeSystem::Pneumatic) ||
                                            (mover->BrakeSystem == TBrakeSystem::ElectroPneumatic))) {
            mover->BrakeLevelSet(mover->Handle->GetPos(bh_NP));
            mover->LimPipePress = mover->PipePress;
            mover->ActFlowSpeed = 0;
        } else {
            mover->BrakeLevelSet(mover->Handle->GetPos(bh_NP));
        }
        mover->MainCtrlPos = mover->MainCtrlNoPowerPos();
        mover->ScndCtrlPos = 0;
    }

    /* The Mover counts the cab its driver sits in as +1 for the front one and -1 for the rear,
     * nobody or the machine room 0 (TTrain::InitializeCab(), Train.cpp:8684; DynObj.cpp:1948-1964) */
    int MoverRailVehicleController::cab_occupied(const RailVehicleCabinKind::Kind p_kind) {
        switch (p_kind) {
            case RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT:
                return 1;
            case RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR:
                return -1;
            default:
                return 0;
        }
    }

    /* Entering, leaving and changing the cab write CabOccupied as they are, with nothing reset
     * (Train.cpp:8305, 10894; Driver.cpp:2124) - the resets of a cab change are cab_controls_reset() */
    void MoverRailVehicleController::set_driver_cabin_kind(const RailVehicleCabinKind::Kind p_kind) {
        RailVehicleController::set_driver_cabin_kind(p_kind);
        if (mover != nullptr) {
            mover->CabOccupied = cab_occupied(p_kind);
        }
    }

    // Original engine: TTrain::MoveToVehicle() (Train.cpp:10950-10954) - CabOccupied follows the driver
    // (set_driver_cabin_kind())
    void MoverRailVehicleController::cabin_leave() const {
        ASSERT_MOVER(mover);
        mover->CabDeactivisation();
        mover->BrakeLevelSet(mover->Handle->GetPos(bh_NP));
        mover->MainCtrlPos = mover->MainCtrlNoPowerPos();
        mover->ScndCtrlPos = 0;
    }

    // Original engine: TTrain::MoveToVehicle() (Train.cpp:10976-10977)
    void MoverRailVehicleController::cabin_enter() const {
        ASSERT_MOVER(mover);
        mover->LimPipePress = mover->PipePress;
        mover->CabActivisationAuto(true);
    }

    /* CabActive counts as CabOccupied does: +1 the front cab, -1 the rear one, 0 none (Mover.cpp:2654) */
    RailVehicleCabinKind::Kind MoverRailVehicleController::get_active_cabin_kind() const {
        if (mover == nullptr) {
            return RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE;
        }
        switch (mover->CabActive) {
            case 1:
                return RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT;
            case -1:
                return RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR;
            default:
                return RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE;
        }
    }

    void MoverRailVehicleController::ground_relay_reset() const {
        ASSERT_MOVER(mover);
        mover->RelayReset(Maszyna::maincircuitground);
    }

    void MoverRailVehicleController::antislip() const {
        ASSERT_MOVER(mover);
        mover->AntiSlippingButton();
    }

    void MoverRailVehicleController::main_controller_increase(const int p_step) const {
        ASSERT_MOVER(mover);
        const int step = p_step > 0 ? p_step : 1;
        mover->IncMainCtrl(step);
    }

    void MoverRailVehicleController::main_controller_decrease(const int p_step) const {
        ASSERT_MOVER(mover);
        const int step = p_step > 0 ? p_step : 1;
        mover->DecMainCtrl(step);
    }

    void MoverRailVehicleController::main_controller_set_position(const int p_position) const {
        ASSERT_MOVER(mover);
        mover->MainCtrlPos = std::clamp(p_position, 0, mover->MainCtrlPosNo);
    }

    // Original engine: OnCommand_secondcontrollerincrease/decrease (Train.cpp:1188, 1349), regular mode
    void MoverRailVehicleController::second_controller_increase(const int p_step) const {
        ASSERT_MOVER(mover);
        const int step = p_step > 0 ? p_step : 1;
        mover->IncScndCtrl(step);
    }

    void MoverRailVehicleController::second_controller_decrease(const int p_step) const {
        ASSERT_MOVER(mover);
        const int step = p_step > 0 ? p_step : 1;
        mover->DecScndCtrl(step);
    }

    void MoverRailVehicleController::direction_increase() const {
        ASSERT_MOVER(mover);
        mover->DirectionForward();
    }

    void MoverRailVehicleController::direction_decrease() const {
        ASSERT_MOVER(mover);
        mover->DirectionBackward();
    }
} // namespace godot
