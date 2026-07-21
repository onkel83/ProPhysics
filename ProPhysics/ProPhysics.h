/**
 * @file ProPhysics.h
 * @brief Rein topologischer C99-Physics-Kernel (Graph-basiert)
 */

#ifndef PROPHYSICS_H
#define PROPHYSICS_H

#include "ProPhysics_Config.h"
#include "ProPhysics_Types.h"
#include "ProPhysics_Exports.h"

#ifdef __cplusplus
extern "C" {
#endif

	// Kernel Lifecycle
	PROPHYSICS_API void ProPhysics_Initialize(ProUniverse* pu, uint64_t node_count);
	PROPHYSICS_API void ProPhysics_Free(ProUniverse* pu);

	// Core Physics Execution Step
	PROPHYSICS_API void ProPhysics_Execute_Tick(ProUniverse* pu);

	// Invariance & Verification
	PROPHYSICS_API bool ProPhysics_Verify_Invariance(const ProUniverse* pu);

	// Topological Helpers (Durchgängig uint64_t für knotenweite Indizes)
	PROPHYSICS_API void ProPhysics_Link_Nodes(ProUniverse* pu, uint64_t src, uint64_t target, uint8_t channel_idx);
	PROPHYSICS_API void ProPhysics_Unlink_Node(ProUniverse* pu, uint64_t src, uint8_t channel_idx);

	/* --- LEGACY FUNCTION ALIASES --- */
#ifndef ProPhysics_Init
#define ProPhysics_Init(pu, count) ProPhysics_Initialize((pu), (count))
#endif

#ifndef ProPhysics_Tick
#define ProPhysics_Tick(pu) ProPhysics_Execute_Tick(pu)
#endif

#ifndef ProPhysics_Step
#define ProPhysics_Step(pu) ProPhysics_Execute_Tick(pu)
#endif

#ifdef __cplusplus
}
#endif

#endif // PROPHYSICS_H