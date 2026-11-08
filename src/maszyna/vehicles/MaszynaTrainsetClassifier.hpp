#pragma once
#include "vehicles/rail/RailVehicleTrainsetClassifier.hpp"

namespace godot {
    /* The original's kind of a train (TController::AutoRewident(), Driver.cpp:2146-2175): of the
     * wagons (Power < 1) - r with the R brake delay, g with G and without R, p the rest - a passenger
     * train while g < min(4, r + p), else a goods train; none without a wagon. MaSzyna's sceneries
     * are timed by it, so the plugin hands it to RailVehicleServer. */
    class MaszynaTrainsetClassifier : public RailVehicleTrainsetClassifier {
            GDCLASS(MaszynaTrainsetClassifier, RailVehicleTrainsetClassifier)

        protected:
            static void _bind_methods();

        public:
            /* Fewer goods wagons than this (and than the others) make a passenger train
             * (Driver.cpp:2175) */
            static constexpr int PASSENGER_GOODS_LIMIT = 4;
            RailVehicleServer::TrainsetType classify(const TypedArray<RID> &p_vehicles) const override;
    };
} // namespace godot
