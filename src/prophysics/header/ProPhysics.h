/**
 * @file ProPhysics.h
 * @brief Rein topologischer C99-Physics-Kernel (Graph-basiert).
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 *
 * Dieser Header ist die oeffentliche API des Kernels. Interne Helfer
 * (Prefix `pro_`) sind in ProPhysics_Internal.h und nicht Teil dieser
 * Schnittstelle.
 *
 * Include-Reihenfolge:
 *   1. ProPhysics_Config.h  (self-contained Konstanten)
 *   2. ProPhysics_Types.h   (Structs, Enums, inline Helfer)
 *   3. ProPhysics_Exports.h (DLL-Gates)
 *   4. ProPhysics.h         (diese Datei, oeffentliche API)
 */

#ifndef PROPHYSICS_H
#define PROPHYSICS_H

#include <stdint.h>
#include <stdbool.h>

#include "ProPhysics_Config.h"
#include "ProPhysics_Types.h"
#include "ProPhysics_Exports.h"

#ifdef __cplusplus
extern "C" {
#endif

	/* ==========================================================================
	 * Kernel-Tick-Callback.
	 *
	 * Der Kernel ruft pro Tick fuer jeden Knoten mit nicht-NEUTRAL type_state
	 * diesen Callback auf. Der Callback darf die Kanaele des aktuellen und
	 * des Ziel-Knotens permutieren. Typischerweise definiert der Test-Harness
	 * hier eine Topologie-Plastizitaet.
	 *
	 * Signatur entspricht 1:1 der alten ProPhysics_ScientificRuleCallback
	 * aus pro_sdk_interface.h. Der Callback wird in ProPhysics_Tick
	 * aufgerufen; siehe ProPhysics_Core.c.
	 *
	 * Parameter:
	 *   current_state, target_state : type_state der beiden Knoten
	 *   current_channels            : Kanaele des aktuellen Knotens (mutierbar)
	 *   target_channels             : Kanaele des Ziel-Knotens (mutierbar)
	 *   out_next_state              : optional; neuer type_state (aktueller Knoten)
	 *   out_next_target_state       : optional; neuer type_state (Ziel-Knoten)
	 *   current_idx, total_nodes    : Index und Gesamtzahl (Diagnose)
	 * ========================================================================== */

	typedef void (*ProPhysics_RuleCallback)(
		uint8_t current_state,
		uint8_t target_state,
		uint64_t* current_channels,
		uint64_t* target_channels,
		uint8_t* out_next_state,
		uint8_t* out_next_target_state,
		uint64_t current_idx,
		uint64_t total_nodes);

	/* --------------------------------------------------------------------------
	 * Lifecycle
	 * -------------------------------------------------------------------------- */

	PROPHYSICS_API void ProPhysics_Initialize(ProUniverse* pu, uint64_t node_count);
	PROPHYSICS_API void ProPhysics_Free(ProUniverse* pu);

	/* --------------------------------------------------------------------------
	 * Topologie
	 * -------------------------------------------------------------------------- */

	PROPHYSICS_API void ProPhysics_Link_Nodes(ProUniverse* pu,
		uint64_t src, uint64_t target,
		uint8_t channel_idx);
	PROPHYSICS_API void ProPhysics_Unlink_Node(ProUniverse* pu,
		uint64_t src, uint8_t channel_idx);

	/* --------------------------------------------------------------------------
	 * Injektion
	 * -------------------------------------------------------------------------- */

	PROPHYSICS_API void ProPhysics_Inject_Momentum(ProUniverse* pu,
		uint64_t node_idx,
		double velocity_ratio);
	PROPHYSICS_API void ProPhysics_Spawn_Body(ProUniverse* pu,
		uint64_t node_idx,
		uint8_t state, uint8_t helicity);

	PROPHYSICS_API void ProPhysics_Set_Edge_Phase(ProUniverse* pu,
		uint64_t src, uint8_t ch,
		double phase_radians,
		uint8_t edge_type);

	/* --------------------------------------------------------------------------
	 * EPR-Messungen
	 * -------------------------------------------------------------------------- */

	PROPHYSICS_API int  ProPhysics_Measure_EPR_Pair(ProUniverse* pu,
		uint64_t node_a,
		double theta_a,
		double theta_b,
		uint64_t rng[4],
		int* out_a, int* out_b);

	PROPHYSICS_API int ProPhysics_Measure_EPR_Pair_Amp(
		ProUniverse* pu,
		uint64_t node_a,
		double   theta_a,
		double   theta_b,
		int* out_a, int* out_b);

	PROPHYSICS_API int ProPhysics_Measure_EPR_Pair_Collapse(
		ProUniverse* pu,
		uint64_t node_a,
		double   theta_a,
		double   theta_b,
		uint64_t rng[4],
		int* out_a, int* out_b);

	PROPHYSICS_API int ProPhysics_Measure_EPR_Pair_Graph(
		ProUniverse* pu,
		uint64_t node_a,
		double   theta_a,
		double   theta_b,
		uint64_t rng[4],
		int* out_a, int* out_b);

	PROPHYSICS_API void     ProPhysics_Set_EPR_Delay(ProUniverse* pu, uint32_t delay_ticks);
	PROPHYSICS_API uint32_t ProPhysics_Get_EPR_Delay(const ProUniverse* pu);
	PROPHYSICS_API void     ProPhysics_Set_EPR_Debug(ProUniverse* pu, uint32_t level);

	/* --------------------------------------------------------------------------
	 * Amp-Grid-Zugriff
	 * -------------------------------------------------------------------------- */

	PROPHYSICS_API ProAmpVector* ProPhysics_Get_Amp_Grid(ProUniverse* pu);

	PROPHYSICS_API bool ProPhysics_Set_Node_Amplitude(ProUniverse* pu,
		uint64_t node_idx,
		uint8_t  basis_idx,
		int32_t  re_q31,
		int32_t  im_q31);

	PROPHYSICS_API bool ProPhysics_Get_Node_Amplitude(const ProUniverse* pu,
		uint64_t node_idx,
		uint8_t  basis_idx,
		int32_t* out_re_q31,
		int32_t* out_im_q31);

	PROPHYSICS_API void ProPhysics_Sync_Amp_From_Type(ProUniverse* pu);

	/* --------------------------------------------------------------------------
	 * Unitare Dynamik
	 * -------------------------------------------------------------------------- */

	PROPHYSICS_API void ProPhysics_Apply_Unitary_Tick(ProUniverse* pu,
		const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE]);

	PROPHYSICS_API double ProPhysics_Verify_Unitarity(
		const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE]);

	PROPHYSICS_API double ProPhysics_Verify_U5_Commutator(
		const ProAmpQ31 U[PRO_AMP_BASIS_SIZE][PRO_AMP_BASIS_SIZE]);

	PROPHYSICS_API ProU128 ProPhysics_Weighted_Norm(const ProUniverse* pu,
		uint64_t node_idx);

	PROPHYSICS_API void ProPhysics_Apply_Signed_Permutation(ProUniverse* pu,
		const uint8_t perm[PRO_AMP_BASIS_SIZE],
		const int8_t  sign[PRO_AMP_BASIS_SIZE]);

	PROPHYSICS_API bool ProPhysics_Verify_Signed_Perm_Unitarity(
		const uint8_t perm[PRO_AMP_BASIS_SIZE],
		const int8_t  sign[PRO_AMP_BASIS_SIZE]);

	PROPHYSICS_API bool ProPhysics_Verify_Signed_Perm_U5(
		const uint8_t perm[PRO_AMP_BASIS_SIZE],
		const int8_t  sign[PRO_AMP_BASIS_SIZE]);

	PROPHYSICS_API void ProPhysics_Compute_Context_Perm(
		const ProUniverse* pu,
		uint64_t node_idx,
		uint8_t  perm_out[PRO_AMP_BASIS_SIZE],
		int8_t   sign_out[PRO_AMP_BASIS_SIZE]);

	PROPHYSICS_API void ProPhysics_Apply_Context_Tick(ProUniverse* pu);

	PROPHYSICS_API void ProPhysics_Apply_Wave_Step(ProUniverse* pu,
		uint32_t phase_step_q15);

	PROPHYSICS_API void ProPhysics_Apply_Edge_Transport(ProUniverse* pu,
		uint32_t theta_q15);

	PROPHYSICS_API void ProPhysics_Apply_Edge_Transport_Colored(
		ProUniverse* pu,
		uint32_t theta_q15,
		uint32_t grid_dim);

	PROPHYSICS_API void ProPhysics_Apply_Edge_Transport_Colored_3D(
		ProUniverse* pu,
		uint32_t theta_q15,
		uint32_t grid_dim);

	PROPHYSICS_API void ProPhysics_Apply_Guiding_Equation(ProUniverse* pu);

	PROPHYSICS_API void ProPhysics_Apply_Amp_Step(ProUniverse* pu,
		uint32_t phase_step_q15);

	PROPHYSICS_API ProU128 ProPhysics_Measure_Amp_Invariant(const ProUniverse* pu);

	PROPHYSICS_API const uint16_t* ProPhysics_Get_U_Field(const ProUniverse* pu);

	/* --------------------------------------------------------------------------
	 * Nichtlineare Phase-Steps (GP-Selbstkopplung)
	 * -------------------------------------------------------------------------- */

	PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step(
		ProUniverse* pu,
		int32_t  g_q15_signed,
		uint32_t phase_step_q15);

	PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step_Dilated(
		ProUniverse* pu,
		int32_t  g_q15_signed,
		uint32_t phase_step_q15);

	/* --------------------------------------------------------------------------
	 * U(1)-Eichstruktur
	 * -------------------------------------------------------------------------- */

	PROPHYSICS_API void ProPhysics_Wilson_Loop(const ProUniverse* pu,
		const uint64_t* path_nodes,
		const uint8_t* path_channels,
		uint32_t        path_len,
		uint16_t* out_phase_fx);

	PROPHYSICS_API void ProPhysics_Global_Phase(ProUniverse* pu,
		uint16_t phase_fx);

	PROPHYSICS_API void ProPhysics_Apply_Local_Gauge(ProUniverse* pu,
		const uint16_t* lambda_fx);

	PROPHYSICS_API void ProPhysics_Make_Lambda_Field(uint64_t seed,
		uint16_t* lambda_fx,
		uint64_t  total_nodes);

	PROPHYSICS_API void ProPhysics_Apply_Local_Phase_Plate(
		ProUniverse* pu,
		uint32_t x0, uint32_t y0,
		uint32_t w, uint32_t h,
		uint16_t phase_q15);

	PROPHYSICS_API void ProPhysics_Apply_Coulomb_Phase_Field_3D(
		ProUniverse* pu,
		uint32_t cx, uint32_t cy, uint32_t cz,
		int32_t  strength_q15,
		uint32_t softening_q8);

	PROPHYSICS_API double ProPhysics_Get_Born_Probability(const ProUniverse* pu,
		uint64_t node_idx,
		uint8_t  basis_idx);

	PROPHYSICS_API double ProPhysics_Compute_Lambda(const ProUniverse* pu,
		uint64_t node_idx);

	PROPHYSICS_API int ProPhysics_Sharp_Measure(double theta, double lambda);

	PROPHYSICS_API int ProPhysics_Type_State_Measure(double theta, uint8_t type_state);

	/* --------------------------------------------------------------------------
	 * Apparat-Subgraph
	 * -------------------------------------------------------------------------- */

	PROPHYSICS_API void ProPhysics_Init_Apparatus(ProUniverse* pu,
		uint64_t base_node,
		uint32_t dim);

	PROPHYSICS_API void ProPhysics_Set_Apparatus_Phase(ProUniverse* pu,
		uint64_t base_node,
		uint32_t dim,
		uint16_t phase_fx);

	PROPHYSICS_API double ProPhysics_Read_Apparatus_Theta(const ProUniverse* pu,
		uint64_t base_node,
		uint32_t dim);

	/* --------------------------------------------------------------------------
	 * Observer (Test-spezifische Zusatzdynamik, nicht Teil des SDK-Ticks)
	 * -------------------------------------------------------------------------- */

	PROPHYSICS_API void ProPhysics_Init_Observer(ProUniverse* pu,
		ProObserver* obs,
		uint64_t base_node,
		uint32_t size);

	PROPHYSICS_API bool ProPhysics_Observer_Read_Local(const ProUniverse* pu,
		const ProObserver* obs,
		ProAmpVector* out_amp);

	PROPHYSICS_API ProU128 ProPhysics_Get_Environment_Trace(
		const ProUniverse* pu, const ProObserver* obs);

	PROPHYSICS_API void ProPhysics_Apply_Local_Amplitude_Diffusion(ProUniverse* pu,
		uint32_t rate_percent);

	PROPHYSICS_API bool ProPhysics_Observer_Measure_CHSH(const ProUniverse* pu,
		const ProObserver* obs_A,
		const ProObserver* obs_B,
		double theta_a,
		double theta_b,
		int* out_a,
		int* out_b);

	PROPHYSICS_API void ProPhysics_Init_Chaotic_Source(ProUniverse* pu,
		uint64_t base_node,
		uint32_t dim,
		uint64_t seed);

	PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Diffusion_Tick(ProUniverse* pu,
		uint32_t rate_percent,
		uint64_t saturation_threshold_q62);

	PROPHYSICS_API bool ProPhysics_Observer_Measure_CHSH_Projected(
		const ProUniverse* pu,
		const ProObserver* obs_A,
		const ProObserver* obs_B,
		double theta_a,
		double theta_b,
		uint64_t rng[4],
		int* out_a,
		int* out_b,
		uint64_t* out_node_a,
		uint64_t* out_node_b);

	PROPHYSICS_API void ProPhysics_Apply_Local_Dephasing_Tick(ProUniverse* pu,
		uint32_t strength_percent);

	/* ==========================================================================
	 * Kernel-Tick-Orchestrierung.
	 *
	 * ProPhysics_Tick ist der top-level-Tick des Kernels. Er ist die
	 * einzige oeffentliche Funktion, die einen vollstaendigen Tick des
	 * Universums ausfuehrt. Alle Einzelschritte (Transport, Wave,
	 * Context, EPR-Propagation, Guiding Equation, Graph-Plastizitaet)
	 * werden hier in der korrekten Reihenfolge aufgerufen.
	 *
	 * Callback: wird pro Knoten mit nicht-NEUTRAL type_state aufgerufen,
	 * um Kanaele zu permutieren (Plastizitaet). NULL ist erlaubt; dann
	 * laeuft die Tick-Schleife ohne Graph-Modifikation.
	 *
	 * Verhalten bei callback == NULL ist bit-identisch zu einem
	 * callback, der seine Argumente unveraendert laesst.
	 *
	 * Ablauf:
	 *   1. Lazy-Init von reg_target (falls NULL).
	 *   2. current_cpu_tick++.
	 *   3. Advance_Internal_Clocks: modifiziert type_state-lose Knoten.
	 *   4. EPR-Propagation: verarbeitet pending_ticks auf EPR-Kanten.
	 *   5. Apply_Amp_Step: U1-U5-Dynamik (Context + Transport + Wave).
	 *   6. Apply_Guiding_Equation: U6-Anzeige (type_state aus amp_grid).
	 *   7. Graph-Plastizitaet: callback + swap reg_source <-> reg_target.
	 *   8. global_entropy_index = Anzahl aktiver Interaktionen.
	 *
	 * Siehe ProPhysics_Core.c fuer die Implementierung.
	 * ========================================================================== */

	PROPHYSICS_API void ProPhysics_Tick(ProUniverse* pu,
		ProPhysics_RuleCallback callback);

	PROPHYSICS_API void ProPhysics_Advance_Internal_Clocks(ProUniverse* pu);

	/* ==========================================================================
	 * Shared Reference (U4).
	 * ========================================================================== */

	PROPHYSICS_API bool ProPhysics_Entangle_Nodes(
		ProUniverse* pu, uint64_t a, uint64_t b);

	PROPHYSICS_API bool ProPhysics_Dissociate_Node(
		ProUniverse* pu, uint64_t a);

	PROPHYSICS_API bool ProPhysics_Is_Entangled(
		const ProUniverse* pu, uint64_t a, uint64_t b);

	PROPHYSICS_API uint64_t ProPhysics_Get_Representative(
		const ProUniverse* pu, uint64_t a);

	PROPHYSICS_API void ProPhysics_Shared_Sync(ProUniverse* pu);

	PROPHYSICS_API uint64_t ProPhysics_Shared_Class_Count(
		const ProUniverse* pu);

	/* Klassen-Tick. Nur aktiv bei pu->shared.active == 1.
	 * Wird intern von ProPhysics_Apply_Amp_Step aufgerufen. Kein
	 * direkter Aufruf durch den Test-Harness noetig. */
	PROPHYSICS_API void ProPhysics_Shared_Tick_Reps(
		ProUniverse* pu, uint32_t phase_step_q15);

	/* ==========================================================================
	 * Spin-1/2.
	 *
	 * Singlet-Entanglement markiert b als spin-flipped gegenueber a.
	 * Get_Node_Spin_View liefert den Spin-Zustand mit angewendetem Flip.
	 * Apply_Nonlinear_Phase_Step_Spin appliziert spin-abhaengige Phase.
	 *
	 * Spin-1/2-Komponenten: {UR_POSITRON_CW (up), UR_POSITRON_CCW (down)}.
	 * ========================================================================== */

	PROPHYSICS_API bool ProPhysics_Entangle_Nodes_Singlet(
		ProUniverse* pu, uint64_t a, uint64_t b);

	PROPHYSICS_API bool ProPhysics_Is_Spin_Flipped(
		const ProUniverse* pu, uint64_t k);

	PROPHYSICS_API bool ProPhysics_Get_Node_Spin_View(
		const ProUniverse* pu, uint64_t k,
		int32_t* out_re_up, int32_t* out_re_down);

	PROPHYSICS_API void ProPhysics_Apply_Nonlinear_Phase_Step_Spin(
		ProUniverse* pu,
		int32_t  g_spin_q15_signed,
		uint32_t phase_step_q15);

	/* ==========================================================================
	 * Dirac-Struktur.
	 *
	 * 4-Komponenten-Spinor {psi_L_up, psi_L_dn, psi_R_up, psi_R_dn}
	 * in Basis 1..4. Implementierung als signed permutations auf
	 * 4 Komponenten.
	 * ========================================================================== */

	PROPHYSICS_API double ProPhysics_Verify_Gamma_Algebra(void);

	PROPHYSICS_API void ProPhysics_Apply_Dirac_Mass_Term(
		ProUniverse* pu,
		int32_t mass_q15);

	PROPHYSICS_API void ProPhysics_Apply_Dirac_Step(
		ProUniverse* pu,
		int32_t  mass_q15,
		uint32_t theta_q15);

	/* ==========================================================================
	 * SU(2)-Eichfeld.
	 *
	 * Die ProEdge-Struktur traegt zusaetzlich ein SU(2)-Link-Element in
	 * Quaternion-Form (a, b) mit |a|^2 + |b|^2 = PRO_SU2_NORM. Skala 2^30.
	 *
	 * Aktivierung:
	 *   Der erste ProPhysics_Set_Edge_SU2-Aufruf setzt pu->su2_active = 1.
	 *   Direktes Setzen von pu->su2_active durch den Test-Setup ist
	 *   ebenfalls moeglich (wie bei dirac_active).
	 *
	 * Wilson-Loop-Konvention (Standard-Lattice-QCD, Vorwaerts):
	 *   W(C) = U(p[0]->p[1]) * U(p[1]->p[2]) * ... * U(p[n-1]->p[0])
	 *   mit U_k = Link(path_nodes[k] -> path_nodes[k+1 mod n]).
	 *
	 * Eichinvarianz:
	 *   U(x->y) -> g(x) * U(x->y) * g(y)^dagger
	 *   W(C)    -> g(p[0]) * W(C) * g(p[0])^dagger
	 *   Tr(W)   -> Tr(W)   (unveraendert, weil Tr zyklisch ist)
	 *
	 * Wilson-Loop-Average (Etappe 23b):
	 *   <Re Tr(W_C)/2> gemittelt ueber alle m x n-Loops der drei
	 *   Ebenen (xy, xz, yz) eines 3D-Torus. Read-only; dient dem
	 *   Creutz-Ratio-Konsistenz-Test in alpha_test_creutz_ratio.c.
	 *   Rueckgabe 0.0, wenn grid_ndim != 3, su2_active == 0, oder
	 *   m/n ausserhalb [1, grid_dim/2].
	 * ========================================================================== */

	PROPHYSICS_API bool ProPhysics_Set_Edge_SU2(
		ProUniverse* pu, uint64_t src, uint8_t ch,
		int32_t a_re_q30, int32_t a_im_q30,
		int32_t b_re_q30, int32_t b_im_q30);

	PROPHYSICS_API bool ProPhysics_Set_Edge_SU2_AxisAngle(
		ProUniverse* pu, uint64_t src, uint8_t ch,
		double nx, double ny, double nz, double alpha);

	PROPHYSICS_API bool ProPhysics_Get_Edge_SU2(
		const ProUniverse* pu, uint64_t src, uint8_t ch,
		int32_t* out_a_re_q30, int32_t* out_a_im_q30,
		int32_t* out_b_re_q30, int32_t* out_b_im_q30);

	PROPHYSICS_API bool ProPhysics_Wilson_Loop_SU2(
		const ProUniverse* pu,
		const uint64_t* path_nodes,
		const uint8_t* path_channels,
		uint32_t        path_len,
		int32_t* out_a_re_q30, int32_t* out_a_im_q30,
		int32_t* out_b_re_q30, int32_t* out_b_im_q30);

	PROPHYSICS_API double ProPhysics_Wilson_Loop_SU2_Trace(
		const ProUniverse* pu,
		const uint64_t* path_nodes,
		const uint8_t* path_channels,
		uint32_t        path_len);

	PROPHYSICS_API double ProPhysics_Wilson_Loop_Average(
		const ProUniverse* pu, uint32_t m, uint32_t n);

	PROPHYSICS_API void ProPhysics_Apply_Local_SU2_Gauge(
		ProUniverse* pu,
		const int32_t* lambda_q30);

	PROPHYSICS_API double ProPhysics_Verify_SU2_Quaternion(void);

	/* ==========================================================================
	 * SU(2)-Link-Dynamik (Leapfrog, Yang-Mills).
	 *
	 * Klassisches Leapfrog-Schema fuer die Link-Felder und deren
	 * chromoelektrisches Feld E (pro Kante in ProEdge.su2_E_*).
	 *
	 * Aktivierung:
	 *   ProPhysics_Enable_SU2_Dynamics(pu)  -- schaltet Dynamik ein.
	 *   ProPhysics_Disable_SU2_Dynamics(pu) -- schaltet sie ab (R7).
	 *   ProPhysics_Is_SU2_Dynamics_Active(pu) -- 0/1.
	 *
	 * Kopplung:
	 *   ProPhysics_Set_SU2_Yang_Mills(pu, g2_q15)  -- g^2 in Q15.
	 *
	 * Direkter Tick (normalerweise nicht noetig; ProPhysics_Tick ruft
	 * ihn intern, wenn die Dynamik aktiv ist):
	 *   ProPhysics_Apply_SU2_Tick(pu, dt_q15)
	 *
	 * Diagnose:
	 *   ProPhysics_SU2_Plaquette_Action(pu) -- Summe (1 - 0.5*Tr(W)).
	 *   ProPhysics_SU2_Total_Energy(pu)     -- S_plaq + 0.5*Sum|E|^2.
	 *
	 * Lokale Plaquette-Summe (read-only):
	 *   ProPhysics_SU2_Link_Plaquette_Sum(pu, x, mu) liefert die Summe
	 *   der Wilson-Aktionen aller Plaquettes, die den Link (x, mu)
	 *   enthalten. In 3D: 4 Plaquettes. Nuetzlich fuer Metropolis / HMC:
	 *   dS = (neue Summe) - (alte Summe) ist der Delta-S-Beitrag eines
	 *   Link-Updates.
	 * ========================================================================== */

	PROPHYSICS_API void ProPhysics_Enable_SU2_Dynamics(ProUniverse* pu);

	PROPHYSICS_API void ProPhysics_Disable_SU2_Dynamics(ProUniverse* pu);

	PROPHYSICS_API int ProPhysics_Is_SU2_Dynamics_Active(const ProUniverse* pu);

	PROPHYSICS_API void ProPhysics_Set_SU2_Yang_Mills(ProUniverse* pu,
		int32_t g2_q15);

	PROPHYSICS_API void ProPhysics_Apply_SU2_Tick(ProUniverse* pu,
		uint32_t dt_q15);

	PROPHYSICS_API double ProPhysics_SU2_Plaquette_Action(const ProUniverse* pu);

	PROPHYSICS_API double ProPhysics_SU2_Total_Energy(const ProUniverse* pu);

	PROPHYSICS_API double ProPhysics_SU2_Link_Plaquette_Sum(
		const ProUniverse* pu, uint64_t x, uint8_t mu);

	/* ==========================================================================
	 * Tensor-Paare (2-Knoten-Verschaenkung).
	 * ========================================================================== */

	PROPHYSICS_API bool ProPhysics_Tensor_Create_Pair(
		ProUniverse* pu, uint64_t node_a, uint64_t node_b,
		uint32_t* out_pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Destroy_Pair(ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Set_State(ProUniverse* pu, uint32_t pair_id,
		const ProAmpQ31 coeff[PRO_TENSOR_DIM]);
	PROPHYSICS_API bool ProPhysics_Tensor_Get_State(const ProUniverse* pu, uint32_t pair_id,
		ProAmpQ31 coeff_out[PRO_TENSOR_DIM]);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_Local_Op(ProUniverse* pu, uint32_t pair_id,
		int side,
		const ProAmpQ31 U[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);
	PROPHYSICS_API bool ProPhysics_Tensor_Partial_Trace_A(const ProUniverse* pu,
		uint32_t pair_id,
		ProAmpQ31 out_rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);
	PROPHYSICS_API bool ProPhysics_Tensor_Partial_Trace_B(const ProUniverse* pu,
		uint32_t pair_id,
		ProAmpQ31 out_rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);
	PROPHYSICS_API double ProPhysics_Von_Neumann_Entropy(
		const ProAmpQ31 rho[PRO_TENSOR_RHO_DIM][PRO_TENSOR_RHO_DIM]);
	PROPHYSICS_API double ProPhysics_Tensor_Entanglement_Entropy(
		const ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Measure_Projective(
		const ProUniverse* pu, uint32_t pair_id,
		double theta_a, double theta_b,
		uint64_t rng[4], int* out_a, int* out_b);
	PROPHYSICS_API bool     ProPhysics_Tensor_Sync_To_Amp(ProUniverse* pu);
	PROPHYSICS_API bool     ProPhysics_Tensor_Sync_From_Amp(ProUniverse* pu,
		uint32_t pair_id);
	PROPHYSICS_API uint32_t ProPhysics_Tensor_Find_Pair_By_Node(
		const ProUniverse* pu, uint64_t node);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_Single_Qubit_Gate(
		ProUniverse* pu, uint32_t pair_id, int side,
		const ProAmpQ31 U[2][2]);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_Two_Qubit_Gate(
		ProUniverse* pu, uint32_t pair_id,
		const ProAmpQ31 G[4][4]);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_Hadamard(
		ProUniverse* pu, uint32_t pair_id, int side);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_X(
		ProUniverse* pu, uint32_t pair_id, int side);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_Y(
		ProUniverse* pu, uint32_t pair_id, int side);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_Z(
		ProUniverse* pu, uint32_t pair_id, int side);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_CNOT(
		ProUniverse* pu, uint32_t pair_id, int side_control);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_CZ(
		ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_SWAP(
		ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_SqrtSwap(
		ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API double ProPhysics_Tensor_Concurrence(
		const ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Mark_Nodes(
		ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Unmark_Nodes(
		ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Is_Node_Marked(
		const ProUniverse* pu, uint64_t node);
	PROPHYSICS_API void ProPhysics_Tensor_Set_Auto_Sync(
		ProUniverse* pu, int enable);
	PROPHYSICS_API bool ProPhysics_Tensor_Sync_Marked_To_Amp(ProUniverse* pu);
	PROPHYSICS_API bool ProPhysics_Tensor_Set_Coupling(
		ProUniverse* pu, uint32_t pair_id, int32_t theta_q15);
	PROPHYSICS_API int32_t ProPhysics_Tensor_Get_Coupling(
		const ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_XX_Step(
		ProUniverse* pu, uint32_t pair_id);

	/* --- SU(2)-Rotation auf Spinor-Paaren --- */
	PROPHYSICS_API bool ProPhysics_Apply_SU2_Rotation(
		ProUniverse* pu, uint64_t node, int which_spinor,
		double nx, double ny, double nz, double alpha);
	PROPHYSICS_API bool ProPhysics_Measure_Spin(
		const ProUniverse* pu, uint64_t node, int which_spinor,
		double* out_sx, double* out_sy, double* out_sz);
	PROPHYSICS_API double ProPhysics_Verify_SU2_Algebra(void);

	/* --- Fermionen / Slater / Hopping --- */
	PROPHYSICS_API bool ProPhysics_Tensor_Fermionize(ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Is_Antisymmetric(
		const ProUniverse* pu, uint32_t pair_id, int32_t tol_q31);
	PROPHYSICS_API bool ProPhysics_Tensor_Set_Slater(
		ProUniverse* pu, uint32_t pair_id,
		uint8_t orbital_a, uint8_t orbital_b);
	PROPHYSICS_API double ProPhysics_Tensor_Pauli_Violation(
		const ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API double ProPhysics_Tensor_Antisymmetrize(
		ProUniverse* pu, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_Tensor_Apply_Hopping(
		ProUniverse* pu, uint32_t pair_id,
		uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15);
	PROPHYSICS_API bool ProPhysics_Tensor_Set_Hopping(
		ProUniverse* pu, uint32_t pair_id,
		uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15);
	PROPHYSICS_API bool ProPhysics_Tensor_Get_Hopping(
		const ProUniverse* pu, uint32_t pair_id,
		uint8_t* out_i, uint8_t* out_j, int32_t* out_theta_q15);

	/* ==========================================================================
	 * Fock-Raum (8 Moden, 256-dim Basis, Jordan-Wigner).
	 * ========================================================================== */

	PROPHYSICS_API uint32_t ProPhysics_Fock_Popcount(uint8_t bits);
	PROPHYSICS_API bool ProPhysics_Fock_Create(ProUniverse* pu, uint64_t* out_fock_id);
	PROPHYSICS_API bool ProPhysics_Fock_Destroy(ProUniverse* pu, uint64_t fock_id);
	PROPHYSICS_API bool ProPhysics_Fock_Set_Basis(ProUniverse* pu, uint64_t fock_id, uint8_t bits);
	PROPHYSICS_API bool ProPhysics_Fock_Get_Amplitude(const ProUniverse* pu, uint64_t fock_id,
		uint8_t bits, int32_t* out_re_q31, int32_t* out_im_q31);
	PROPHYSICS_API bool ProPhysics_Fock_Set_Amplitude(ProUniverse* pu, uint64_t fock_id,
		uint8_t bits, int32_t re_q31, int32_t im_q31);
	PROPHYSICS_API double ProPhysics_Fock_Norm(const ProUniverse* pu, uint64_t fock_id);
	PROPHYSICS_API double ProPhysics_Fock_Particle_Number(const ProUniverse* pu, uint64_t fock_id);
	PROPHYSICS_API uint64_t ProPhysics_Fock_Count(const ProUniverse* pu);
	PROPHYSICS_API bool ProPhysics_Fock_Apply_Create(ProUniverse* pu, uint64_t fock_id, uint8_t mode);
	PROPHYSICS_API bool ProPhysics_Fock_Apply_Annihilate(ProUniverse* pu, uint64_t fock_id, uint8_t mode);
	PROPHYSICS_API bool ProPhysics_Fock_Apply_Create_Plus_Annihilate(ProUniverse* pu, uint64_t fock_id, uint8_t mode);
	PROPHYSICS_API bool ProPhysics_Fock_Is_Zero(const ProUniverse* pu, uint64_t fock_id);
	PROPHYSICS_API bool ProPhysics_Fock_Clone(ProUniverse* pu, uint64_t src_fock_id, uint64_t* out_dst_fock_id);
	PROPHYSICS_API bool ProPhysics_Fock_Apply_Anticomm_CD(ProUniverse* pu, uint64_t fock_id, uint8_t i, uint8_t j);
	PROPHYSICS_API bool ProPhysics_Fock_Apply_Anticomm_CC(ProUniverse* pu, uint64_t fock_id, uint8_t i, uint8_t j);
	PROPHYSICS_API bool ProPhysics_Fock_Apply_Anticomm_DD(ProUniverse* pu, uint64_t fock_id, uint8_t i, uint8_t j);
	PROPHYSICS_API double ProPhysics_Fock_Compare(const ProUniverse* pu, uint64_t fid_a, uint64_t fid_b);
	PROPHYSICS_API bool ProPhysics_Fock_Scale(ProUniverse* pu, uint64_t fock_id, int32_t scale_q31);
	PROPHYSICS_API bool ProPhysics_Fock_Apply_Hopping(ProUniverse* pu, uint64_t fock_id,
		uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15);
	PROPHYSICS_API bool ProPhysics_Tensor_To_Fock(ProUniverse* pu, uint32_t pair_id, uint64_t* out_fock_id);
	PROPHYSICS_API bool ProPhysics_Fock_To_Tensor(const ProUniverse* pu, uint64_t fock_id, uint32_t pair_id);

	/* ==========================================================================
	 * Dichte-Matrizen (8x8 Knoten, 64x64 Tensor, 256x256 Fock).
	 * ========================================================================== */

	PROPHYSICS_API bool ProPhysics_Density_Create(ProUniverse* pu, uint32_t* out_rho_id);
	PROPHYSICS_API bool ProPhysics_Density_Destroy(ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API uint64_t ProPhysics_Density_Count(const ProUniverse* pu);
	PROPHYSICS_API bool ProPhysics_Density_From_Pure(ProUniverse* pu, uint32_t rho_id, const ProAmpVector* psi);
	PROPHYSICS_API bool ProPhysics_Density_From_Mixture(ProUniverse* pu, uint32_t rho_id,
		const ProAmpVector* states, const double* weights, uint32_t n);
	PROPHYSICS_API bool ProPhysics_Density_Get_Element(const ProUniverse* pu, uint32_t rho_id,
		uint32_t i, uint32_t j, int32_t* out_re, int32_t* out_im);
	PROPHYSICS_API bool ProPhysics_Density_Set_Element(ProUniverse* pu, uint32_t rho_id,
		uint32_t i, uint32_t j, int32_t re, int32_t im);
	PROPHYSICS_API double ProPhysics_Density_Trace(const ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API double ProPhysics_Density_Purity(const ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API double ProPhysics_Density_Von_Neumann_Entropy(const ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API double ProPhysics_Density_Offdiag_Magnitude(const ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API bool ProPhysics_Density_Apply_Unitary(ProUniverse* pu, uint32_t rho_id,
		const ProAmpQ31 U[PRO_DENSITY_DIM][PRO_DENSITY_DIM]);
	PROPHYSICS_API bool ProPhysics_Density_Apply_Lindblad_Step(ProUniverse* pu, uint32_t rho_id,
		uint32_t lindblad_kind, double strength);
	PROPHYSICS_API bool ProPhysics_TensorDensity_Create(ProUniverse* pu, uint32_t pair_id, uint32_t* out_rho_id);
	PROPHYSICS_API bool ProPhysics_TensorDensity_Destroy(ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API uint64_t ProPhysics_TensorDensity_Count(const ProUniverse* pu);
	PROPHYSICS_API bool ProPhysics_TensorDensity_From_Pair(ProUniverse* pu, uint32_t rho_id, uint32_t pair_id);
	PROPHYSICS_API bool ProPhysics_TensorDensity_Get_Element(const ProUniverse* pu, uint32_t rho_id,
		uint32_t i, uint32_t j, int32_t* out_re, int32_t* out_im);
	PROPHYSICS_API double ProPhysics_TensorDensity_Trace(const ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API double ProPhysics_TensorDensity_Purity(const ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API bool ProPhysics_TensorDensity_Partial_Trace_A(const ProUniverse* pu, uint32_t rho_id, uint32_t out_rho_id);
	PROPHYSICS_API bool ProPhysics_TensorDensity_Partial_Trace_B(const ProUniverse* pu, uint32_t rho_id, uint32_t out_rho_id);
	PROPHYSICS_API bool ProPhysics_TensorDensity_Apply_Local_Lindblad(ProUniverse* pu, uint32_t rho_id, int side,
		uint32_t lindblad_kind, double strength);
	PROPHYSICS_API bool ProPhysics_FockDensity_Create(ProUniverse* pu, uint64_t fock_id, uint32_t* out_rho_id);
	PROPHYSICS_API bool ProPhysics_FockDensity_Destroy(ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API uint64_t ProPhysics_FockDensity_Count(const ProUniverse* pu);
	PROPHYSICS_API bool ProPhysics_FockDensity_From_Fock(ProUniverse* pu, uint32_t rho_id, uint64_t fock_id);
	PROPHYSICS_API bool ProPhysics_FockDensity_Get_Element(const ProUniverse* pu, uint32_t rho_id,
		uint32_t i, uint32_t j, int32_t* out_re, int32_t* out_im);
	PROPHYSICS_API bool ProPhysics_FockDensity_Set_Element(ProUniverse* pu, uint32_t rho_id,
		uint32_t i, uint32_t j, int32_t re, int32_t im);
	PROPHYSICS_API double ProPhysics_FockDensity_Trace(const ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API double ProPhysics_FockDensity_Purity(const ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API double ProPhysics_FockDensity_Mode_Occupation(const ProUniverse* pu, uint32_t rho_id, uint32_t mode);
	PROPHYSICS_API bool ProPhysics_FockDensity_Apply_Mode_Lindblad(ProUniverse* pu, uint32_t rho_id, uint32_t mode,
		uint32_t lindblad_kind, double strength);
	PROPHYSICS_API bool ProPhysics_FockDensity_Partial_Trace_Modes(const ProUniverse* pu, uint32_t rho_id,
		uint32_t n_keep, ProAmpQ31* out_rho, uint32_t* out_dim);
	PROPHYSICS_API void ProPhysics_Density_Set_Auto_Sync(ProUniverse* pu, int enable);
	PROPHYSICS_API bool ProPhysics_TensorDensity_Sync_From_Pair(ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API bool ProPhysics_FockDensity_Sync_From_Fock(ProUniverse* pu, uint32_t rho_id);
	PROPHYSICS_API bool ProPhysics_TensorDensity_Sync_All_From_Pairs(ProUniverse* pu);
	PROPHYSICS_API bool ProPhysics_FockDensity_Sync_All_From_Fock(ProUniverse* pu);

#ifndef ProPhysics_Init
#define ProPhysics_Init(pu, count) ProPhysics_Initialize((pu), (count))
#endif

#ifdef __cplusplus
}
#endif

#endif /* PROPHYSICS_H */