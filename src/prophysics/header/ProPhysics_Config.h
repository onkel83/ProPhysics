/* ==========================================================================
 * ProPhysics - Unified Configuration
 * File: ProPhysics_Config.h
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * Zentrale Konfiguration des Kernels.
 * Architektur: C99, Static Compile-Time Constants (Zero-Allocation Core).
 *
 * Konvention:
 *   - Jede Konstante ist mit #ifndef geschuetzt, damit der Nutzer sie
 *     per -D<NAME>=<WERT> beim Build ueberschreiben kann.
 *   - Abgeleitete Konstanten (mit Klammern) werden NICHT geschuetzt;
 *     sie folgen aus den Basiswerten.
 *   - Diese Datei darf NICHTS aus anderen ProPhysics-Headern inkludieren.
 *     Sie ist self-contained und wird von allen anderen Headern zuerst
 *     eingebunden.
 *
 * Detaillierte Beschreibung aller Konstanten:
 *   siehe docs/project/CONFIG.md
 * ========================================================================== */

#ifndef PROPHYSICS_CONFIG_H
#define PROPHYSICS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

 /* ==========================================================================
  * 1. Topologie / Gittergroesse
  * ========================================================================== */

#ifndef NODE_COUNT
#define NODE_COUNT 1048576ULL   /* 2^20 Knoten (Default Standard-Gitter) */
#endif

#ifndef PRO_NODE_COUNT
#define PRO_NODE_COUNT NODE_COUNT
#endif

#ifndef MAX_NODES
#define MAX_NODES NODE_COUNT
#endif

#ifndef MAX_SPARSE_TRACKING_NODES
#define MAX_SPARSE_TRACKING_NODES 64000000ULL
#endif

  /* ==========================================================================
   * 2. Kanaele und Nachbarschaft
   *
   * Kanal-Belegung:
   *   0 = +x, 1 = -x, 2 = +y, 3 = -y, 4 = +z, 5 = Dephase,
   *   6 = -z, 7..14 = Reserve, 15 = EPR
   * ========================================================================== */

#ifndef CHANNELS_MAX
#define CHANNELS_MAX 16
#endif

#ifndef SYMMETRY_CHANNELS
#define SYMMETRY_CHANNELS CHANNELS_MAX
#endif

#define PRO_EPR_CHANNEL       (CHANNELS_MAX - 1)
#define PRO_DEPHASE_CHANNEL   5
#define PRO_PHYSICAL_CHANNELS PRO_EPR_CHANNEL
#define PRO_CH_MASK           (CHANNELS_MAX - 1u)

#define PRO_NEIGHBOR_X_PLUS   0u
#define PRO_NEIGHBOR_X_MINUS  1u
#define PRO_NEIGHBOR_Y_PLUS   2u
#define PRO_NEIGHBOR_Y_MINUS  3u
#define PRO_NEIGHBOR_Z_PLUS   4u
#define PRO_NEIGHBOR_Z_MINUS  6u

   /* ==========================================================================
	* 3. Amplituden-Basis
	* ========================================================================== */

#ifndef PRO_AMP_BASIS_SIZE
#define PRO_AMP_BASIS_SIZE 8u
#endif

	/* ==========================================================================
	 * 4. Tensor / Fock / Dichte
	 * ========================================================================== */

#ifndef PRO_TENSOR_DIM
#define PRO_TENSOR_DIM 64u
#endif

#ifndef PRO_TENSOR_RHO_DIM
#define PRO_TENSOR_RHO_DIM 8u
#endif

#ifndef PRO_TENSOR_MAX_PAIRS
#define PRO_TENSOR_MAX_PAIRS 256u
#endif

#ifndef PRO_FOCK_MODES
#define PRO_FOCK_MODES 8u
#endif

#ifndef PRO_FOCK_DIM
#define PRO_FOCK_DIM 256u
#endif

#ifndef PRO_FOCK_MAX_STATES
#define PRO_FOCK_MAX_STATES 64u
#endif

#ifndef PRO_DENSITY_DIM
#define PRO_DENSITY_DIM 8u
#endif

#ifndef PRO_DENSITY_MAX
#define PRO_DENSITY_MAX 64u
#endif

#ifndef PRO_TENSOR_DENSITY_DIM
#define PRO_TENSOR_DENSITY_DIM 64u
#endif

#ifndef PRO_TENSOR_DENSITY_MAX
#define PRO_TENSOR_DENSITY_MAX 32u
#endif

#ifndef PRO_FOCK_DENSITY_DIM
#define PRO_FOCK_DENSITY_DIM 256u
#endif

#ifndef PRO_FOCK_DENSITY_MAX
#define PRO_FOCK_DENSITY_MAX 8u
#endif

	 /* ==========================================================================
	  * 5. Dirac-Struktur
	  *
	  * Komponenten-Reihenfolge: {L_up, L_dn, R_up, R_dn}.
	  * Mapping zur Amplituden-Basis: Basis-Index = Komponenten-Index + 1.
	  * ========================================================================== */

#ifndef PRO_DIRAC_DIM
#define PRO_DIRAC_DIM 4u
#endif

#define PRO_DIRAC_COMP_L_UP   0u
#define PRO_DIRAC_COMP_L_DN   1u
#define PRO_DIRAC_COMP_R_UP   2u
#define PRO_DIRAC_COMP_R_DN   3u

#define PRO_DIRAC_TO_BASIS(ci)  ((uint8_t)((ci) + 1u))
#define PRO_DIRAC_FROM_BASIS(b) ((uint8_t)((b) - 1u))

	  /* ==========================================================================
	   * 6. SU(2)-Eichfeld: Kinematik
	   *
	   * Link-Elemente in Quaternion-Form (a, b) mit |a|^2 + |b|^2 = 2^60.
	   * Skala 2^30 (nicht 2^31 -- int64-Overflow-Schutz).
	   * ========================================================================== */

#ifndef PRO_SU2_SCALE
#define PRO_SU2_SCALE      1073741824            /* 2^30 */
#endif

#ifndef PRO_SU2_NORM
#define PRO_SU2_NORM       1152921504606846976LL /* 2^60 */
#endif

#ifndef PRO_SU2_IDENT_RE
#define PRO_SU2_IDENT_RE   1073741824            /* 1.0 in Skala 2^30 */
#endif

#ifndef PRO_SU2_IDENT_IM
#define PRO_SU2_IDENT_IM   0
#endif

	   /* ==========================================================================
		* 6b. SU(2)-Dynamik (Yang-Mills-Leapfrog)
		*
		* Hamilton-Funktion: H = S_plaq + (1/2) * Sum_links |E|^2
		*
		* Leapfrog-Integration pro Tick:
		*   1. E <- E - dt * dS/dU
		*   2. U <- exp(i * dt * E) * U
		*   3. E <- E - dt * dS/dU
		*
		* Nur aktiv bei su2_dynamics_active == 1.
		* ========================================================================== */

#ifndef PRO_SU2_YM_DEFAULT_Q15
#define PRO_SU2_YM_DEFAULT_Q15 1000
#endif

#ifndef PRO_SU2_LEAPFROG_DT_Q15
#define PRO_SU2_LEAPFROG_DT_Q15 1000u
#endif

		/* ==========================================================================
		 * 7. Node-Bits (reserved_gating in ProNode)
		 *
		 * Bit 0: Spin-Flip-Flag
		 * Bit 1: Dirac-Knoten
		 * Bit 2..7: Reserve
		 * ========================================================================== */

#define PRO_NODE_SPIN_FLIP_BIT  0x01u
#define PRO_NODE_DIRAC_BIT      0x02u

		 /* Komplement-Masken zum Loeschen der jeweiligen Bits. */
#define PRO_NODE_SPIN_FLIP_MASK  ((uint8_t)0xFEu)   /* ~0x01 */
#define PRO_NODE_DIRAC_MASK      ((uint8_t)0xFDu)   /* ~0x02 */
		 /* ==========================================================================
		  * 8. Physik-Konstanten
		  * ========================================================================== */

#ifndef PRO_2PI
#define PRO_2PI              6.283185307179586476925286766559
#endif

#ifndef PRO_INV_2PI
#define PRO_INV_2PI          0.15915494309189533576888376337251
#endif

#ifndef PRO_8_OVER_2PI
#define PRO_8_OVER_2PI       1.2732395447351626861510701069801
#endif

#ifndef PRO_2PI_INV_Q16
#define PRO_2PI_INV_Q16      (65536.0 / 6.283185307179586476925286766559)
#endif

#ifndef PRO_INV_127
#define PRO_INV_127          0.00787401574803149606299212598425
#endif

		  /* 1/sqrt(2) in Q31-Skala (INT32_MAX * 0.7071067811865476 = 1518500249).
		   * Verwendung: Hadamard-Gate, Superpositions-Aufbau. */
#ifndef PRO_Q31_HALF_SQRT2
#define PRO_Q31_HALF_SQRT2   1518500249
#endif

		  /* ==========================================================================
		   * 9. Physik-Defaults
		   * ========================================================================== */

#ifndef PRO_DEFAULT_PHASE_STEP_Q15
#define PRO_DEFAULT_PHASE_STEP_Q15 1000u
#endif

#ifndef PRO_DEFAULT_TRANSPORT_THETA_Q15
#define PRO_DEFAULT_TRANSPORT_THETA_Q15 500u
#endif

		   /* ==========================================================================
			* 10. Compiler-Attribute
			* ========================================================================== */

#ifndef PRO_INLINE
#if defined(_MSC_VER)
#define PRO_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define PRO_INLINE inline __attribute__((always_inline))
#else
#define PRO_INLINE inline
#endif
#endif

			/* ==========================================================================
			 * 11. Legacy-Typedefs
			 * ========================================================================== */

typedef double   Real;
typedef uint64_t Index;

/* ==========================================================================
 * 12. Cache- und Debug-Konfiguration
 * ========================================================================== */

#ifndef PRO_CACHE_LINE
#define PRO_CACHE_LINE 64u
#endif

#if (PRO_CACHE_LINE & (PRO_CACHE_LINE - 1u)) != 0
#error "PRO_CACHE_LINE muss Zweierpotenz sein"
#endif

#ifndef PRO_DEBUG_RING_SIZE
#define PRO_DEBUG_RING_SIZE 256u
#endif

#if (PRO_DEBUG_RING_SIZE & (PRO_DEBUG_RING_SIZE - 1u)) != 0
#error "PRO_DEBUG_RING_SIZE muss Zweierpotenz sein"
#endif

#endif /* PROPHYSICS_CONFIG_H */