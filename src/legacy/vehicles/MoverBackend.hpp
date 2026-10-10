#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"

/* The vendored Mover is the backend a component may happen to be implemented on. It is named
 * here and in the `Mover*` implementations that include this header - never in `VehicleComponent`
 * or in any `Vehicle<Domain>` interface, which describe a vehicle without saying what simulates
 * it. */

#define ASSERT_MOVER(mover_ptr)                                                                                        \
    if ((mover_ptr) == nullptr) {                                                                                      \
        return;                                                                                                        \
    }

/* A brake component's Mover work also needs the Mover's own brake (Hamulec). */
#define ASSERT_MOVER_BRAKE(mover_ptr, ...)                                                                             \
    if ((mover_ptr) == nullptr || mover_ptr->Hamulec == nullptr) {                                                     \
        return __VA_ARGS__;                                                                                            \
    }
