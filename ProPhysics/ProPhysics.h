/* ==========================================================================
 * ProPhysics - Engine Kernel Interface
 * File: ProPhysics.h
 * Architecture: C99, Pure Sequential Node Evaluation
 * ========================================================================== */

#ifndef PROPHYSICS_H
#define PROPHYSICS_H

#include "ProPhysics_Types.h"

#ifdef _MSC_VER
#define PROPHYSICS_API __declspec(dllexport)
#else
#define PROPHYSICS_API __attribute__((visibility("default")))
#endif

PROPHYSICS_API void ProPhysics_Initialize(ProUniverse* pu, uint64_t node_count);
PROPHYSICS_API void ProPhysics_Execute_Tick(ProUniverse* pu);
PROPHYSICS_API bool ProPhysics_Verify_Invariance(const ProUniverse* pu);
PROPHYSICS_API void ProPhysics_Free(ProUniverse* pu);

#endif // PROPHYSICS_H