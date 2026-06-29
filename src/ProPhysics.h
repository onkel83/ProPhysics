/* ==========================================================================
 * ProPhysics - Engine Kernel Interface
 * File: ProPhysics.h
 * Architecture: C99, Zero-Allocation, Cache-Line Aligned
 * ========================================================================== */

#ifndef PROPHYSICS_H
#define PROPHYSICS_H

#include "ProPhysics_Config.h"
#include "ProPhysics_Types.h"

#ifdef _MSC_VER
#include <intrin.h>
#define POPCOUNT64(x) __popcnt64(x) 
#else
#define POPCOUNT64(x) __builtin_popcountll(x)
#endif

PRO_EXPORT void   ProPhysics_Initialize(ProUniverse* pu);
PRO_EXPORT void   ProPhysics_Inject_Elements(ProUniverse* pu);
PRO_EXPORT double ProPhysics_Query_Anisotropy(const ProUniverse* pu, int32_t source_x, int32_t source_y, int32_t source_z);
PRO_EXPORT void   ProPhysics_Execute_Tick(ProUniverse* pu);
PRO_EXPORT bool   ProPhysics_Verify_Invariance(const ProUniverse* pu, uint64_t expected_initial_bits);
PRO_EXPORT void   ProPhysics_Reset(ProUniverse* pu, uint64_t* initial_bit_tracker);
PRO_EXPORT void ProPhysics_Update_Observer(ProUniverse* pu, uint64_t expected_initial_bits);

#endif // PROPHYSICS_H