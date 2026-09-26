/* example_test_density.c — Etappe 16a: Dichtematrix / Lindblad
 * 8x8-Q31-Dichtematrix auf dem Knoten-Hilbertraum.
 * Qubit-Basis fuer Lindblad: {UR_POSITRON_CW (=1), UR_NEGATRON_CCW (=4)}.
 *
 * Kompilieren (MSVC):
 *   cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. example_test_density.c ^
 *      /Fe:example_test_density.exe /link ProPhysics.lib
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ProPhysics.h"
#include "pro_sdk_interface.h"  

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define LN2 0.69314718055994530942

static int g_pass = 0;
static int g_fail = 0;

static void check(const char* name, int cond)
{
    printf("  [%s] %s\n", cond ? "PASS" : "FAIL", name);
    if (cond) g_pass++; else g_fail++;
}

static ProAmpQ31 q31(int32_t re, int32_t im) { return pro_amp_pack(re, im); }
static int32_t q31_one(void) { return INT32_MAX; }

/* Basis-Zustand |b> als ProAmpVector. */
static ProAmpVector pure_basis(uint8_t b)
{
    ProAmpVector v;
    for (int i = 0; i < 8; ++i) v.coeff[i] = 0;
    v.coeff[b] = q31(INT32_MAX, 0);
    return v;
}

/* Superposition (|b1> + |b2>) / sqrt(2). */
static ProAmpVector superposition(uint8_t b1, uint8_t b2)
{
    ProAmpVector v;
    for (int i = 0; i < 8; ++i) v.coeff[i] = 0;
    const int32_t a = (int32_t)lround(0.7071067811865476 * 2147483647.0);
    v.coeff[b1] = q31(a, 0);
    v.coeff[b2] = q31(a, 0);
    return v;
}

/* ---------------------------------------------------------------- D1
 * Reiner Basis-Zustand |1>: Tr(ρ)=1, Purity=1, S=0.
 */
static void test_D1_pure_basis(void)
{
    printf("\n--- D1: Reiner Zustand |1> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t rho_id = 0;
    ProPhysics_Density_Create(&pu, &rho_id);

    ProAmpVector psi = pure_basis(UR_POSITRON_CW);
    ProPhysics_Density_From_Pure(&pu, rho_id, &psi);

    const double tr = ProPhysics_Density_Trace(&pu, rho_id);
    const double pu_ = ProPhysics_Density_Purity(&pu, rho_id);
    const double S = ProPhysics_Density_Von_Neumann_Entropy(&pu, rho_id);

    printf("  Tr(ρ) = %.12f (erwartet 1)\n", tr);
    printf("  Tr(ρ²) = %.12f (erwartet 1)\n", pu_);
    printf("  S     = %.12e (erwartet 0)\n", S);

    check("Tr(ρ) = 1", fabs(tr - 1.0) < 1e-6);
    check("Purity = 1", fabs(pu_ - 1.0) < 1e-6);
    check("S = 0", S < 1e-9);

    ProPhysics_Density_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- D2
 * Superposition (|1> + |2>)/sqrt2: Purity=1, S=0.
 * (Trotz Superposition ist es ein reiner Zustand.)
 */
static void test_D2_pure_superposition(void)
{
    printf("\n--- D2: Superposition (|1> + |2>)/sqrt2 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t rho_id = 0;
    ProPhysics_Density_Create(&pu, &rho_id);

    ProAmpVector psi = superposition(UR_POSITRON_CW, UR_POSITRON_CCW);
    ProPhysics_Density_From_Pure(&pu, rho_id, &psi);

    const double tr = ProPhysics_Density_Trace(&pu, rho_id);
    const double pu_ = ProPhysics_Density_Purity(&pu, rho_id);
    const double S = ProPhysics_Density_Von_Neumann_Entropy(&pu, rho_id);
    const double off = ProPhysics_Density_Offdiag_Magnitude(&pu, rho_id);

    printf("  Tr(ρ) = %.12f\n", tr);
    printf("  Tr(ρ²) = %.12f (erwartet 1)\n", pu_);
    printf("  S     = %.12e (erwartet 0)\n", S);
    printf("  Σ_{i≠j}|ρ_ij|² = %.12f (erwartet 0.5)\n", off);

    check("Tr(ρ) = 1", fabs(tr - 1.0) < 1e-6);
    check("Purity = 1", fabs(pu_ - 1.0) < 1e-6);
    check("S = 0", S < 1e-9);
    check("Kohaerenz ρ_12 ≠ 0", off > 0.4);

    ProPhysics_Density_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- D3
 * Gemisch 0.5|1> + 0.5|2>. Purity = 0.5, S = ln 2.
 */
static void test_D3_mixture(void)
{
    printf("\n--- D3: Gemisch 0.5|1> + 0.5|2> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t rho_id = 0;
    ProPhysics_Density_Create(&pu, &rho_id);

    ProAmpVector states[2] = { pure_basis(UR_POSITRON_CW),
                               pure_basis(UR_POSITRON_CCW) };
    const double weights[2] = { 0.5, 0.5 };
    ProPhysics_Density_From_Mixture(&pu, rho_id, states, weights, 2u);

    const double tr = ProPhysics_Density_Trace(&pu, rho_id);
    const double pu_ = ProPhysics_Density_Purity(&pu, rho_id);
    const double S = ProPhysics_Density_Von_Neumann_Entropy(&pu, rho_id);

    printf("  Tr(ρ) = %.12f\n", tr);
    printf("  Tr(ρ²) = %.12f (erwartet 0.5)\n", pu_);
    printf("  S     = %.12f (erwartet ln2 = %.12f)\n", S, LN2);

    check("Tr(ρ) = 1", fabs(tr - 1.0) < 1e-6);
    check("Purity = 0.5", fabs(pu_ - 0.5) < 1e-4);
    check("S = ln2", fabs(S - LN2) < 1e-3);

    ProPhysics_Density_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- D4
 * Unitäre Evolution erhält Purity. R_x(pi) auf Basis |1>.
 */
static void test_D4_unitary(void)
{
    printf("\n--- D4: Unitäre Evolution erhält Purity ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t rho_id = 0;
    ProPhysics_Density_Create(&pu, &rho_id);

    ProAmpVector psi = superposition(UR_POSITRON_CW, UR_NEGATRON_CCW);
    ProPhysics_Density_From_Pure(&pu, rho_id, &psi);

    const double pu_before = ProPhysics_Density_Purity(&pu, rho_id);

    /* U = R_x(pi/2) auf {1,4}-Subspace, angewendet als volle 8x8. */
    ProAmpQ31 U[8][8] = { {0} };
    for (int i = 0; i < 8; ++i) U[i][i] = q31(q31_one(), 0);
    const int32_t h = (int32_t)lround(0.7071067811865476 * 2147483647.0);
    U[1][1] = q31(h, 0);  U[1][4] = q31(0, -h);
    U[4][1] = q31(0, -h); U[4][4] = q31(h, 0);

    ProPhysics_Density_Apply_Unitary(&pu, rho_id, U);

    const double pu_after = ProPhysics_Density_Purity(&pu, rho_id);
    const double tr_after = ProPhysics_Density_Trace(&pu, rho_id);

    printf("  Purity vor  = %.12f\n", pu_before);
    printf("  Purity nach = %.12f\n", pu_after);
    printf("  Tr nach     = %.12f\n", tr_after);
    printf("  |ΔPurity|   = %.4e\n", fabs(pu_after - pu_before));

    check("Purity erhalten", fabs(pu_after - pu_before) < 1e-4);
    check("Tr(ρ) = 1", fabs(tr_after - 1.0) < 1e-4);

    ProPhysics_Density_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- D5
 * Lindblad Amplitude-Damping: ρ_11(t) = ρ_11(0) · e^{-γt}.
 */
static void test_D5_amp_damp(void)
{
    printf("\n--- D5: Amplitude-Damping ρ_11 → e^{-γt} ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t rho_id = 0;
    ProPhysics_Density_Create(&pu, &rho_id);

    ProAmpVector psi = pure_basis(UR_POSITRON_CW);   /* |1> */
    ProPhysics_Density_From_Pure(&pu, rho_id, &psi);

    const int STEPS = 100;
    const double gamma_dt = 0.02;    /* γ · dt = 0.02, total γ·t = 2.0 */

    for (int t = 0; t < STEPS; ++t) {
        ProPhysics_Density_Apply_Lindblad_Step(
            &pu, rho_id, PRO_LINDBLAD_AMP_DAMP, gamma_dt);
    }

    int32_t r11 = 0, i11 = 0;
    ProPhysics_Density_Get_Element(&pu, rho_id,
        UR_POSITRON_CW, UR_POSITRON_CW, &r11, &i11);
    const double rho11 = (double)r11 / 2147483647.0;

    const double expected = exp(-(double)STEPS * gamma_dt);
    const double dev = fabs(rho11 - expected);

    printf("  ρ_11(0)   = 1.0\n");
    printf("  ρ_11(%d) = %.9f (erwartet %.9f)\n", STEPS, rho11, expected);
    printf("  Abweichung = %.4e\n", dev);

    check("ρ_11 nahe e^{-γt}", dev < 5e-3);

    ProPhysics_Density_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- D6
 * Lindblad Phase-Damping: ρ_14(t) = ρ_14(0) · e^{-γt}.
 */
static void test_D6_phase_damp(void)
{
    printf("\n--- D6: Phase-Damping ρ_14 → e^{-γt} ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t rho_id = 0;
    ProPhysics_Density_Create(&pu, &rho_id);

    ProAmpVector psi = superposition(UR_POSITRON_CW, UR_NEGATRON_CCW);
    ProPhysics_Density_From_Pure(&pu, rho_id, &psi);

    const int STEPS = 100;
    const double gamma_dt = 0.02;

    for (int t = 0; t < STEPS; ++t) {
        ProPhysics_Density_Apply_Lindblad_Step(
            &pu, rho_id, PRO_LINDBLAD_PHASE_DAMP, gamma_dt);
    }

    int32_t r14 = 0, i14 = 0;
    ProPhysics_Density_Get_Element(&pu, rho_id,
        UR_POSITRON_CW, UR_NEGATRON_CCW, &r14, &i14);
    const double rho14 = (double)r14 / 2147483647.0;

    const double expected = 0.5 * exp(-(double)STEPS * gamma_dt);
    const double dev = fabs(rho14 - expected);

    printf("  ρ_14(0)   = 0.5\n");
    printf("  ρ_14(%d) = %.9f (erwartet %.9f)\n", STEPS, rho14, expected);
    printf("  Abweichung = %.4e\n", dev);

    check("ρ_14 nahe 0.5·e^{-γt}", dev < 5e-3);

    ProPhysics_Density_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- D7
 * Depolarisierung: ρ → (1-p)ρ + p I/2, konvergiert zu I/2.
 */
static void test_D7_depolarize(void)
{
    printf("\n--- D7: Depolarisierung ρ → I/2 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t rho_id = 0;
    ProPhysics_Density_Create(&pu, &rho_id);

    ProAmpVector psi = pure_basis(UR_POSITRON_CW);
    ProPhysics_Density_From_Pure(&pu, rho_id, &psi);

    const int STEPS = 500;
    const double gamma_dt = 0.05;

    for (int t = 0; t < STEPS; ++t) {
        ProPhysics_Density_Apply_Lindblad_Step(
            &pu, rho_id, PRO_LINDBLAD_DEPOLARIZE, gamma_dt);
    }

    int32_t r11 = 0, i11 = 0, r44 = 0, i44 = 0;
    ProPhysics_Density_Get_Element(&pu, rho_id, 1, 1, &r11, &i11);
    ProPhysics_Density_Get_Element(&pu, rho_id, 4, 4, &r44, &i44);
    const double d11 = (double)r11 / 2147483647.0;
    const double d44 = (double)r44 / 2147483647.0;
    const double off = ProPhysics_Density_Offdiag_Magnitude(&pu, rho_id);
    const double tr = ProPhysics_Density_Trace(&pu, rho_id);

    printf("  ρ_11 = %.6f (Ziel 0.5)\n", d11);
    printf("  ρ_44 = %.6f (Ziel 0.5)\n", d44);
    printf("  Offdiag = %.4e (Ziel 0)\n", off);
    printf("  Tr = %.6f\n", tr);

    check("ρ_11 nahe 0.5", fabs(d11 - 0.5) < 1e-3);
    check("ρ_44 nahe 0.5", fabs(d44 - 0.5) < 1e-3);
    check("Off-Diagonalen ausgeloescht", off < 1e-6);
    check("Tr(ρ) = 1", fabs(tr - 1.0) < 1e-4);

    ProPhysics_Density_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- D8
 * Slater-Tensor(1,4) → Partial_Trace_A → ρ_A in ProDensityMatrix.
 * Konsistenzcheck mit Tensor-Pfad.
 */
static void test_D8_tensor_rho_consistency(void)
{
    printf("\n--- D8: Tensor Slater(1,4) → ρ_A Konsistenz ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    ProAmpQ31 rho_A[8][8];
    ProPhysics_Tensor_Partial_Trace_A(&pu, pid, rho_A);

    /* Kopiere die Tensor-ρ_A in eine ProDensityMatrix. */
    uint32_t rho_id = 0;
    ProPhysics_Density_Create(&pu, &rho_id);
    for (int a = 0; a < 8; ++a) {
        for (int b = 0; b < 8; ++b) {
            ProPhysics_Density_Set_Element(&pu, rho_id, (uint32_t)a, (uint32_t)b,
                pro_amp_real(rho_A[a][b]),
                pro_amp_imag(rho_A[a][b]));
        }
    }

    const double tr = ProPhysics_Density_Trace(&pu, rho_id);
    const double pu_ = ProPhysics_Density_Purity(&pu, rho_id);
    const double S = ProPhysics_Density_Von_Neumann_Entropy(&pu, rho_id);

    /* Tensor-Entropie als Referenz. */
    const double S_tensor = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    printf("  Tr(ρ_A)      = %.12f (erwartet 1)\n", tr);
    printf("  Tr(ρ_A²)     = %.12f (erwartet 0.5)\n", pu_);
    printf("  S(ρ_A)       = %.12f\n", S);
    printf("  S_tensor     = %.12f\n", S_tensor);
    printf("  |ΔS|         = %.4e\n", fabs(S - S_tensor));

    check("Tr(ρ_A) = 1", fabs(tr - 1.0) < 1e-3);
    check("Purity = 0.5", fabs(pu_ - 0.5) < 1e-3);
    check("S = ln2", fabs(S - LN2) < 1e-3);
    check("Konsistenz mit Tensor", fabs(S - S_tensor) < 1e-3);

    ProPhysics_Density_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- D9
 * ρ → Purity → Lindblad → Purity: Konsistenz und Stabilitaet.
 * Insbesondere: Purity ist monoton fallend unter Amplitude-Damping
 * (solange wir nicht an |4> saturieren).
 */
static void test_D9_purity_monotonie(void)
{
    printf("\n--- D9: Purity monoton unter Lindblad ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t rho_id = 0;
    ProPhysics_Density_Create(&pu, &rho_id);

    ProAmpVector psi = superposition(UR_POSITRON_CW, UR_NEGATRON_CCW);
    ProPhysics_Density_From_Pure(&pu, rho_id, &psi);

    double pu_prev = ProPhysics_Density_Purity(&pu, rho_id);
    double pu_min = pu_prev;
    int monotone = 1;

    for (int t = 0; t < 50; ++t) {
        ProPhysics_Density_Apply_Lindblad_Step(
            &pu, rho_id, PRO_LINDBLAD_PHASE_DAMP, 0.1);
        const double p = ProPhysics_Density_Purity(&pu, rho_id);
        if (p > pu_prev + 1e-6) monotone = 0;
        if (p < pu_min) pu_min = p;
        pu_prev = p;
    }

    uint32_t rho_dbg = 0;
    ProPhysics_Density_Create(&pu, &rho_dbg);
    ProAmpVector psi_dbg = superposition(UR_POSITRON_CW, UR_NEGATRON_CCW);
    ProPhysics_Density_From_Pure(&pu, rho_dbg, &psi_dbg);
    const double pu_initial = ProPhysics_Density_Purity(&pu, rho_dbg);
    ProPhysics_Density_Destroy(&pu, rho_dbg);

    printf("  Purity(0)  = %.9f\n", pu_initial);
    printf("  Purity_min = %.9f\n", pu_min);
    printf("  Monoton fallend: %s\n", monotone ? "ja" : "nein");

    check("Purity monoton fallend", monotone);
    check("Purity < 1 nach Lindblad", pu_min < 0.99);

    ProPhysics_Density_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- D10
 * Tr(ρ) = 1 unter Lindblad fuer alle drei Kanaele.
 */
static void test_D10_trace_preservation(void)
{
    printf("\n--- D10: Tr(ρ) = 1 unter Lindblad ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    const uint32_t kinds[3] = {
        PRO_LINDBLAD_AMP_DAMP,
        PRO_LINDBLAD_PHASE_DAMP,
        PRO_LINDBLAD_DEPOLARIZE
    };
    const char* names[3] = { "amp_damp", "phase_damp", "depolarize" };

    for (int k = 0; k < 3; ++k) {
        uint32_t rho_id = 0;
        ProPhysics_Density_Create(&pu, &rho_id);

        ProAmpVector psi = superposition(UR_POSITRON_CW, UR_NEGATRON_CCW);
        ProPhysics_Density_From_Pure(&pu, rho_id, &psi);

        for (int t = 0; t < 100; ++t) {
            ProPhysics_Density_Apply_Lindblad_Step(
                &pu, rho_id, kinds[k], 0.05);
        }

        const double tr = ProPhysics_Density_Trace(&pu, rho_id);
        printf("  %-14s Tr(ρ) = %.9f\n", names[k], tr);
        check(names[k], fabs(tr - 1.0) < 1e-3);

        ProPhysics_Density_Destroy(&pu, rho_id);
    }

    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- B1
 * Slater(1,4) → Pair → TensorDensity. Purity=1, Tr=1.
 */
static void test_B1_pure_tensor_density(void)
{
    printf("\n--- B1: Slater(1,4) -> 64x64 TensorDensity ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    uint32_t rho_id = 0;
    ProPhysics_TensorDensity_Create(&pu, pid, &rho_id);
    ProPhysics_TensorDensity_From_Pair(&pu, rho_id, pid);

    const double tr = ProPhysics_TensorDensity_Trace(&pu, rho_id);
    const double pu_ = ProPhysics_TensorDensity_Purity(&pu, rho_id);

    /* Slater(1,4): Tensor-Indizes (1*8+4) = 12 und (4*8+1) = 33. */
    int32_t r12_12 = 0, i12_12 = 0, r12_33 = 0, i12_33 = 0;
    int32_t r33_33 = 0, i33_33 = 0;
    ProPhysics_TensorDensity_Get_Element(&pu, rho_id, 12, 12, &r12_12, &i12_12);
    ProPhysics_TensorDensity_Get_Element(&pu, rho_id, 12, 33, &r12_33, &i12_33);
    ProPhysics_TensorDensity_Get_Element(&pu, rho_id, 33, 33, &r33_33, &i33_33);

    const double d12 = (double)r12_12 / 2147483647.0;
    const double d12_33 = (double)r12_33 / 2147483647.0;
    const double d33 = (double)r33_33 / 2147483647.0;

    printf("  Tr(rho)   = %.12f (erwartet 1)\n", tr);
    printf("  Purity    = %.12f (erwartet 1)\n", pu_);
    printf("  rho[12,12]= %+.6f (erwartet +0.5)\n", d12);
    printf("  rho[12,33]= %+.6f (erwartet -0.5)\n", d12_33);
    printf("  rho[33,33]= %+.6f (erwartet +0.5)\n", d33);

    check("Tr(rho) = 1", fabs(tr - 1.0) < 1e-4);
    check("Purity = 1", fabs(pu_ - 1.0) < 1e-3);
    check("rho[12,12] = +0.5", fabs(d12 - 0.5) < 1e-3);
    check("rho[12,33] = -0.5", fabs(d12_33 + 0.5) < 1e-3);
    check("rho[33,33] = +0.5", fabs(d33 - 0.5) < 1e-3);

    ProPhysics_TensorDensity_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- B2
 * Bell + Partial_Trace_A: S(rho_A) = ln2, konsistent mit Tensor-Pfad.
 */
static void test_B2_partial_trace_consistency(void)
{
    printf("\n--- B2: Partial_Trace_A vs. Tensor-Pfad ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    uint32_t rho_id = 0;
    ProPhysics_TensorDensity_Create(&pu, pid, &rho_id);
    ProPhysics_TensorDensity_From_Pair(&pu, rho_id, pid);

    uint32_t rho_A_id = 0;
    ProPhysics_Density_Create(&pu, &rho_A_id);
    ProPhysics_TensorDensity_Partial_Trace_A(&pu, rho_id, rho_A_id);

    const double S_A = ProPhysics_Density_Von_Neumann_Entropy(&pu, rho_A_id);
    const double pu_A = ProPhysics_Density_Purity(&pu, rho_A_id);
    const double S_tensor = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    printf("  S(rho_A)  = %.12f\n", S_A);
    printf("  Purity_A  = %.12f\n", pu_A);
    printf("  S_tensor  = %.12f\n", S_tensor);
    printf("  |dS|      = %.4e\n", fabs(S_A - S_tensor));

    check("S(rho_A) = ln2", fabs(S_A - LN2) < 1e-3);
    check("Purity_A = 0.5", fabs(pu_A - 0.5) < 1e-3);
    check("Konsistenz mit Tensor-Pfad", fabs(S_A - S_tensor) < 1e-3);

    ProPhysics_Density_Destroy(&pu, rho_A_id);
    ProPhysics_TensorDensity_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- B3
 * Bell + A-Depolarize: Kohaerenz von rho_full verschwindet.
 *
 * Wichtige Beobachtung: die reduzierte Dichte rho_A eines Bell-Zustands
 * ist per Konstruktion DIAGONAL (ρ_A = diag(0.5, 0.5) auf {1,4}).
 * Die Verschraenkung steckt in rho_full, nicht in rho_A.
 * Also messen wir die Off-Diagonalen von rho_full bei (12, 33).
 */
static void test_B3_A_depolarize(void)
{
    printf("\n--- B3: Bell + A-Depolarize -> rho_full-Kohaerenz weg ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    uint32_t rho_id = 0;
    ProPhysics_TensorDensity_Create(&pu, pid, &rho_id);
    ProPhysics_TensorDensity_From_Pair(&pu, rho_id, pid);

    int32_t r12_33_before = 0, i12_33_before = 0;
    ProPhysics_TensorDensity_Get_Element(
        &pu, rho_id, 12, 33, &r12_33_before, &i12_33_before);

    const int STEPS = 100;
    for (int t = 0; t < STEPS; ++t) {
        ProPhysics_TensorDensity_Apply_Local_Lindblad(
            &pu, rho_id, 0, PRO_LINDBLAD_DEPOLARIZE, 0.1);
    }

    int32_t r12_33_after = 0, i12_33_after = 0;
    ProPhysics_TensorDensity_Get_Element(
        &pu, rho_id, 12, 33, &r12_33_after, &i12_33_after);

    uint32_t rho_A_id = 0;
    ProPhysics_Density_Create(&pu, &rho_A_id);
    ProPhysics_TensorDensity_Partial_Trace_A(&pu, rho_id, rho_A_id);

    const double S_after_A = ProPhysics_Density_Von_Neumann_Entropy(&pu, rho_A_id);
    const double tr_full = ProPhysics_TensorDensity_Trace(&pu, rho_id);
    const double coh_before = (double)r12_33_before / 2147483647.0;
    const double coh_after = (double)r12_33_after / 2147483647.0;

    printf("  rho_full[12,33] before = %+.6f (erwartet -0.5)\n", coh_before);
    printf("  rho_full[12,33] after  = %+.6f (erwartet  0.0)\n", coh_after);
    printf("  Tr(rho_full)           = %.9f\n", tr_full);
    printf("  S(rho_A) after Depol.  = %.6f (ln2 = %.6f)\n",
        S_after_A, LN2);

    check("Kohaerenz vorher = -0.5", fabs(coh_before + 0.5) < 1e-3);
    check("Kohaerenz nachher ~ 0", fabs(coh_after) < 1e-3);
    check("Tr(rho_full) = 1", fabs(tr_full - 1.0) < 1e-4);
    check("S(rho_A) bleibt ln2", fabs(S_after_A - LN2) < 1e-3);

    ProPhysics_Density_Destroy(&pu, rho_A_id);
    ProPhysics_TensorDensity_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- B4
 * Bell + A-Amp-Damp: rho_A wird reiner Zustand |4><4|.
 */
static void test_B4_A_amp_damp(void)
{
    printf("\n--- B4: Bell + A-Amp-Damp -> rho_A wird rein ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    uint32_t rho_id = 0;
    ProPhysics_TensorDensity_Create(&pu, pid, &rho_id);
    ProPhysics_TensorDensity_From_Pair(&pu, rho_id, pid);

    const int STEPS = 200;
    for (int t = 0; t < STEPS; ++t) {
        ProPhysics_TensorDensity_Apply_Local_Lindblad(
            &pu, rho_id, 0, PRO_LINDBLAD_AMP_DAMP, 0.05);
    }

    uint32_t rho_A_id = 0;
    ProPhysics_Density_Create(&pu, &rho_A_id);
    ProPhysics_TensorDensity_Partial_Trace_A(&pu, rho_id, rho_A_id);

    const double S_A = ProPhysics_Density_Von_Neumann_Entropy(&pu, rho_A_id);
    const double pu_A = ProPhysics_Density_Purity(&pu, rho_A_id);

    int32_t r44 = 0, i44 = 0;
    ProPhysics_Density_Get_Element(&pu, rho_A_id, 4, 4, &r44, &i44);
    const double d44 = (double)r44 / 2147483647.0;

    printf("  S(rho_A)   = %.6f (nahe 0: reiner Zustand)\n", S_A);
    printf("  Purity_A   = %.6f (nahe 1)\n", pu_A);
    printf("  rho_A[4,4] = %.6f (nahe 1: A ist im Grundzustand)\n", d44);

    check("Purity_A > 0.9", pu_A > 0.9);
    check("S(rho_A) < 0.1", S_A < 0.1);
    check("rho_A[4,4] > 0.9", d44 > 0.9);

    ProPhysics_Density_Destroy(&pu, rho_A_id);
    ProPhysics_TensorDensity_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- B5
 * Bell + beidseitig Depolarize: Kohaerenz verschwindet, Tr bleibt 1.
 *
 * Hinweis zur Erwartung: der lokale Depolarizer wirkt nur auf dem
 * 2-dim {1,4}-Subraum. S(rho_A) ist daher auf ln2 gedeckelt; ein
 * Bell-Zustand hat bereits S(rho_A) = ln2. Getestet wird deshalb,
 * dass die Kohaerenz (off-diag) verschwindet und Tr erhalten bleibt.
 */
static void test_B5_both_depolarize(void)
{
    printf("\n--- B5: Bell + beidseitig Depolarize -> Kohaerenz weg ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    uint32_t rho_id = 0;
    ProPhysics_TensorDensity_Create(&pu, pid, &rho_id);
    ProPhysics_TensorDensity_From_Pair(&pu, rho_id, pid);

    int32_t r12_33_before = 0, i12_33_before = 0;
    ProPhysics_TensorDensity_Get_Element(
        &pu, rho_id, 12, 33, &r12_33_before, &i12_33_before);

    const int STEPS = 200;
    for (int t = 0; t < STEPS; ++t) {
        ProPhysics_TensorDensity_Apply_Local_Lindblad(
            &pu, rho_id, 0, PRO_LINDBLAD_DEPOLARIZE, 0.1);
        ProPhysics_TensorDensity_Apply_Local_Lindblad(
            &pu, rho_id, 1, PRO_LINDBLAD_DEPOLARIZE, 0.1);
    }

    int32_t r12_33_after = 0, i12_33_after = 0;
    ProPhysics_TensorDensity_Get_Element(
        &pu, rho_id, 12, 33, &r12_33_after, &i12_33_after);

    uint32_t rho_A_id = 0;
    ProPhysics_Density_Create(&pu, &rho_A_id);
    ProPhysics_TensorDensity_Partial_Trace_A(&pu, rho_id, rho_A_id);

    const double S_A = ProPhysics_Density_Von_Neumann_Entropy(&pu, rho_A_id);
    const double tr = ProPhysics_TensorDensity_Trace(&pu, rho_id);
    const double coh_before = (double)r12_33_before / 2147483647.0;
    const double coh_after = (double)r12_33_after / 2147483647.0;

    printf("  Tr(rho)                = %.9f\n", tr);
    printf("  S(rho_A)               = %.6f (bleibt ~ln2 = %.6f)\n",
        S_A, LN2);
    printf("  rho_full[12,33] before = %+.6f\n", coh_before);
    printf("  rho_full[12,33] after  = %+.6f\n", coh_after);

    check("Tr(rho) = 1", fabs(tr - 1.0) < 1e-4);
    check("S(rho_A) bleibt nahe ln2", fabs(S_A - LN2) < 1e-3);
    check("Kohaerenz (12,33) verschwunden", fabs(coh_after) < 1e-3);

    ProPhysics_Density_Destroy(&pu, rho_A_id);
    ProPhysics_TensorDensity_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- B6
 * Tr-Erhaltung fuer alle Kanaele x beide Seiten.
 */
static void test_B6_trace_all_channels(void)
{
    printf("\n--- B6: Tr-Erhaltung x {A,B} x {3 Kanaele} ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    const uint32_t kinds[3] = {
        PRO_LINDBLAD_AMP_DAMP,
        PRO_LINDBLAD_PHASE_DAMP,
        PRO_LINDBLAD_DEPOLARIZE
    };
    const char* names[3] = { "amp_damp", "phase_damp", "depolarize" };

    for (int side = 0; side < 2; ++side) {
        for (int k = 0; k < 3; ++k) {
            uint32_t pid = 0;
            ProPhysics_Tensor_Create_Pair(&pu, (uint64_t)(2 * side), (uint64_t)(2 * side + 1), &pid);
            ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

            uint32_t rho_id = 0;
            ProPhysics_TensorDensity_Create(&pu, pid, &rho_id);
            ProPhysics_TensorDensity_From_Pair(&pu, rho_id, pid);

            for (int t = 0; t < 100; ++t) {
                ProPhysics_TensorDensity_Apply_Local_Lindblad(
                    &pu, rho_id, side, kinds[k], 0.05);
            }

            const double tr = ProPhysics_TensorDensity_Trace(&pu, rho_id);
            char label[64];
            snprintf(label, sizeof(label), "side=%d %-13s", side, names[k]);
            printf("  %s Tr(rho) = %.9f\n", label, tr);
            check(label, fabs(tr - 1.0) < 1e-3);

            ProPhysics_TensorDensity_Destroy(&pu, rho_id);
            ProPhysics_Tensor_Destroy_Pair(&pu, pid);
        }
    }

    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- E1
 * Fock-Basis |0...0> -> FockDensity. Tr=1, Purity=1.
 */
static void test_E1_vacuum(void)
{
    printf("\n--- E1: Fock-Dichte Vakuum |0...0> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);

    uint32_t rho_id = 0;
    ProPhysics_FockDensity_Create(&pu, fid, &rho_id);
    ProPhysics_FockDensity_From_Fock(&pu, rho_id, fid);

    const double tr = ProPhysics_FockDensity_Trace(&pu, rho_id);
    const double pu_ = ProPhysics_FockDensity_Purity(&pu, rho_id);

    printf("  Tr(rho) = %.12f (erwartet 1)\n", tr);
    printf("  Purity  = %.12f (erwartet 1)\n", pu_);

    for (uint32_t m = 0; m < 8u; ++m) {
        const double n_m = ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, m);
        if (n_m != 0.0) {
            printf("  <n_%u> = %.6f (erwartet 0)\n", m, n_m);
        }
    }

    check("Tr = 1", fabs(tr - 1.0) < 1e-6);
    check("Purity = 1", fabs(pu_ - 1.0) < 1e-6);
    check("Alle <n_m> = 0",
        ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, 0) < 1e-9 &&
        ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, 7) < 1e-9);

    ProPhysics_FockDensity_Destroy(&pu, rho_id);
    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- E2
 * Superposition (|1> + |2>)/sqrt2 im Fock-Raum.
 * Erwartung: Tr=1, Purity=1, <n_0>=<n_1>=0.5.
 */
static void test_E2_superposition(void)
{
    printf("\n--- E2: Fock-Superposition (|1>+|2>)/sqrt2 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    const int32_t amp = (int32_t)lround(0.7071067811865476 * 2147483647.0);
    ProPhysics_Fock_Set_Amplitude(&pu, fid, 0x00u, 0, 0);
    ProPhysics_Fock_Set_Amplitude(&pu, fid, 0x01u, amp, 0);
    ProPhysics_Fock_Set_Amplitude(&pu, fid, 0x02u, amp, 0);

    uint32_t rho_id = 0;
    ProPhysics_FockDensity_Create(&pu, fid, &rho_id);
    ProPhysics_FockDensity_From_Fock(&pu, rho_id, fid);

    const double tr = ProPhysics_FockDensity_Trace(&pu, rho_id);
    const double pu_ = ProPhysics_FockDensity_Purity(&pu, rho_id);
    const double n0 = ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, 0);
    const double n1 = ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, 1);

    printf("  Tr       = %.12f\n", tr);
    printf("  Purity   = %.12f (erwartet 1)\n", pu_);
    printf("  <n_0>    = %.6f (erwartet 0.5)\n", n0);
    printf("  <n_1>    = %.6f (erwartet 0.5)\n", n1);

    check("Tr = 1", fabs(tr - 1.0) < 1e-6);
    check("Purity = 1", fabs(pu_ - 1.0) < 1e-6);
    check("<n_0> = 0.5", fabs(n0 - 0.5) < 1e-4);
    check("<n_1> = 0.5", fabs(n1 - 0.5) < 1e-4);

    ProPhysics_FockDensity_Destroy(&pu, rho_id);
    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- E3
 * Gemisch 0.5|1> + 0.5|2>. Purity = 0.5.
 */
static void test_E3_mixture(void)
{
    printf("\n--- E3: Fock-Gemisch 0.5|1> + 0.5|2> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t rho_id = 0;
    ProPhysics_FockDensity_Create(&pu, UINT64_MAX, &rho_id);

    /* Manuell bauen: rho = 0.5|1><1| + 0.5|2><2|.
     * Q31-Diagonale: 0.5 * 2^31 / 2^31 -> "0.5 in physikalischer Norm"
     * = Q31-Wert 2^30. */
    const int32_t half = INT32_MAX / 2;
    ProPhysics_FockDensity_Set_Element(&pu, rho_id, 0x01u, 0x01u, half, 0);
    ProPhysics_FockDensity_Set_Element(&pu, rho_id, 0x02u, 0x02u, half, 0);
    /* Vakuum-Diagonale explizit nullen. */
    ProPhysics_FockDensity_Set_Element(&pu, rho_id, 0x00u, 0x00u, 0, 0);

    const double tr = ProPhysics_FockDensity_Trace(&pu, rho_id);
    const double pu_ = ProPhysics_FockDensity_Purity(&pu, rho_id);
    const double n0 = ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, 0);
    const double n1 = ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, 1);

    printf("  Tr     = %.12f\n", tr);
    printf("  Purity = %.12f (erwartet 0.5)\n", pu_);
    printf("  <n_0>  = %.6f (erwartet 0.5)\n", n0);
    printf("  <n_1>  = %.6f (erwartet 0.5)\n", n1);

    check("Tr = 1", fabs(tr - 1.0) < 1e-4);
    check("Purity = 0.5", fabs(pu_ - 0.5) < 1e-3);
    check("<n_0> = 0.5", fabs(n0 - 0.5) < 1e-4);
    check("<n_1> = 0.5", fabs(n1 - 0.5) < 1e-4);

    ProPhysics_FockDensity_Destroy(&pu, rho_id);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- E4
 * Fock-Basis |1,2> (Bits 0 und 1 besetzt) -> Partial_Trace (n_keep=2).
 * Erwartung: rho_A = |1,1><1,1| auf 4-dim (n_0,n_1).
 */
static void test_E4_partial_trace(void)
{
    printf("\n--- E4: Partial_Trace auf ersten 2 Moden ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x03u);   /* Bits 0 und 1 besetzt */

    uint32_t rho_id = 0;
    ProPhysics_FockDensity_Create(&pu, fid, &rho_id);
    ProPhysics_FockDensity_From_Fock(&pu, rho_id, fid);

    ProAmpQ31 rho_A[4 * 4];
    uint32_t dim_A = 0;
    const bool ok = ProPhysics_FockDensity_Partial_Trace_Modes(
        &pu, rho_id, 2u, rho_A, &dim_A);
    check("Partial_Trace ok", ok);
    check("dim_A = 4", dim_A == 4u);

    if (ok) {
        /* Erwartung: rho_A[3][3] = 1, alle anderen 0.
         * (a=3 entspricht n_0=1, n_1=1). */
        int32_t r33 = 0, i33 = 0;
        r33 = pro_amp_real(rho_A[3 * 4 + 3]);
        i33 = pro_amp_imag(rho_A[3 * 4 + 3]);
        const double d33 = (double)r33 / 2147483647.0;

        int32_t r00 = 0, i00 = 0;
        r00 = pro_amp_real(rho_A[0 * 4 + 0]);
        const double d00 = (double)r00 / 2147483647.0;

        printf("  rho_A[3][3] = %.6f (erwartet 1)\n", d33);
        printf("  rho_A[0][0] = %.6f (erwartet 0)\n", d00);

        check("rho_A[3][3] = 1", fabs(d33 - 1.0) < 1e-4);
        check("rho_A[0][0] = 0", fabs(d00) < 1e-6);
    }

    ProPhysics_FockDensity_Destroy(&pu, rho_id);
    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- E5
 * Mode 0 Amp-Damp aus |1>. <n_0> -> e^{-γt}, <n_1..7> bleiben 0.
 */
static void test_E5_amp_damp_mode0(void)
{
    printf("\n--- E5: Amp-Damp Mode 0 aus |1> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);   /* Nur Mode 0 besetzt */

    uint32_t rho_id = 0;
    ProPhysics_FockDensity_Create(&pu, fid, &rho_id);
    ProPhysics_FockDensity_From_Fock(&pu, rho_id, fid);

    const int STEPS = 100;
    const double gamma_dt = 0.02;

    for (int t = 0; t < STEPS; ++t) {
        ProPhysics_FockDensity_Apply_Mode_Lindblad(
            &pu, rho_id, 0u, PRO_LINDBLAD_AMP_DAMP, gamma_dt);
    }

    const double n0 = ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, 0);
    const double n1 = ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, 1);
    const double tr = ProPhysics_FockDensity_Trace(&pu, rho_id);

    const double expected = exp(-(double)STEPS * gamma_dt);
    const double dev = fabs(n0 - expected);

    printf("  <n_0>(0)   = 1.0\n");
    printf("  <n_0>(%d) = %.9f (erwartet %.9f)\n", STEPS, n0, expected);
    printf("  <n_1>      = %.9f (erwartet 0)\n", n1);
    printf("  Tr         = %.9f (erwartet 1)\n", tr);
    printf("  Abweichung = %.4e\n", dev);

    check("<n_0> nahe e^{-gamma t}", dev < 5e-3);
    check("<n_1> = 0", fabs(n1) < 1e-9);
    check("Tr = 1", fabs(tr - 1.0) < 1e-4);

    ProPhysics_FockDensity_Destroy(&pu, rho_id);
    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- E6
 * Mode 0 Phase-Damp auf Superposition (|vac> + |1>)/sqrt2.
 * Off-diagonal verschwindet, <n_0> bleibt konstant bei 0.5.
 */
static void test_E6_phase_damp(void)
{
    printf("\n--- E6: Phase-Damp Mode 0 auf Superposition ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    const int32_t amp = (int32_t)lround(0.7071067811865476 * 2147483647.0);
    /* Superposition (|vac> + |0x01>)/sqrt2: */
    ProPhysics_Fock_Set_Amplitude(&pu, fid, 0x00u, amp, 0);
    ProPhysics_Fock_Set_Amplitude(&pu, fid, 0x01u, amp, 0);

    uint32_t rho_id = 0;
    ProPhysics_FockDensity_Create(&pu, fid, &rho_id);
    ProPhysics_FockDensity_From_Fock(&pu, rho_id, fid);

    int32_t r01_before = 0, i01_before = 0;
    ProPhysics_FockDensity_Get_Element(&pu, rho_id, 0x00u, 0x01u,
        &r01_before, &i01_before);

    const int STEPS = 200;
    const double gamma_dt = 0.05;   /* total γ·t = 10 */

    for (int t = 0; t < STEPS; ++t) {
        ProPhysics_FockDensity_Apply_Mode_Lindblad(
            &pu, rho_id, 0u, PRO_LINDBLAD_PHASE_DAMP, gamma_dt);
    }

    int32_t r01_after = 0, i01_after = 0;
    ProPhysics_FockDensity_Get_Element(&pu, rho_id, 0x00u, 0x01u,
        &r01_after, &i01_after);
    const double off_before = (double)r01_before / 2147483647.0;
    const double off_after = (double)r01_after / 2147483647.0;
    const double n0 = ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, 0);
    const double tr = ProPhysics_FockDensity_Trace(&pu, rho_id);

    printf("  rho[0,1] before = %.9f (erwartet 0.5)\n", off_before);
    printf("  rho[0,1] after  = %.9e (nahe 0, e^-10 = 4.5e-5)\n", off_after);
    printf("  <n_0>           = %.9f (erwartet 0.5)\n", n0);
    printf("  Tr              = %.9f\n", tr);

    check("Off-diag before = 0.5", fabs(off_before - 0.5) < 1e-6);
    check("Off-diag after ~ 0", fabs(off_after) < 1e-3);
    check("<n_0> = 0.5 erhalten", fabs(n0 - 0.5) < 1e-6);
    check("Tr = 1", fabs(tr - 1.0) < 1e-4);

    ProPhysics_FockDensity_Destroy(&pu, rho_id);
    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
} 
/* ---------------------------------------------------------------- E7
 * Tr-Erhaltung fuer alle drei Kanaele auf Mode 2.
 */
static void test_E7_trace_all_channels(void)
{
    printf("\n--- E7: Tr-Erhaltung ueber alle Kanaele ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    const uint32_t kinds[3] = {
        PRO_LINDBLAD_AMP_DAMP,
        PRO_LINDBLAD_PHASE_DAMP,
        PRO_LINDBLAD_DEPOLARIZE
    };
    const char* names[3] = { "amp_damp", "phase_damp", "depolarize" };

    for (int k = 0; k < 3; ++k) {
        uint64_t fid = 0;
        ProPhysics_Fock_Create(&pu, &fid);
        ProPhysics_Fock_Set_Basis(&pu, fid, 0x05u);   /* Moden 0 und 2 besetzt */

        uint32_t rho_id = 0;
        ProPhysics_FockDensity_Create(&pu, fid, &rho_id);
        ProPhysics_FockDensity_From_Fock(&pu, rho_id, fid);

        for (int t = 0; t < 100; ++t) {
            ProPhysics_FockDensity_Apply_Mode_Lindblad(
                &pu, rho_id, 2u, kinds[k], 0.05);
        }

        const double tr = ProPhysics_FockDensity_Trace(&pu, rho_id);
        printf("  %-12s Tr = %.9f\n", names[k], tr);
        check(names[k], fabs(tr - 1.0) < 1e-3);

        ProPhysics_FockDensity_Destroy(&pu, rho_id);
        ProPhysics_Fock_Destroy(&pu, fid);
    }

    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- E8
 * Alle 8 Moden Amp-Damp auf |1,2,3,4,5,6,7,8> × 200 Steps -> Vakuum.
 */
static void test_E8_all_modes_decay(void)
{
    printf("\n--- E8: Alle 8 Moden Amp-Damp -> Vakuum ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0xFFu);   /* Alle 8 Moden besetzt */

    uint32_t rho_id = 0;
    ProPhysics_FockDensity_Create(&pu, fid, &rho_id);
    ProPhysics_FockDensity_From_Fock(&pu, rho_id, fid);

    const int STEPS = 200;
    for (int t = 0; t < STEPS; ++t) {
        for (uint32_t m = 0; m < 8u; ++m) {
            ProPhysics_FockDensity_Apply_Mode_Lindblad(
                &pu, rho_id, m, PRO_LINDBLAD_AMP_DAMP, 0.05);
        }
    }

    const double tr = ProPhysics_FockDensity_Trace(&pu, rho_id);
    double total_n = 0.0;
    for (uint32_t m = 0; m < 8u; ++m) {
        total_n += ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, m);
    }

    printf("  Tr       = %.9f\n", tr);
    printf("  <N>_tot  = %.9f (erwartet ~0)\n", total_n);

    check("Tr = 1", fabs(tr - 1.0) < 1e-3);
    check("<N>_tot nahe 0", total_n < 0.05);

    ProPhysics_FockDensity_Destroy(&pu, rho_id);
    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- C1
 * TensorDensity + auto_sync_density: Tr=1, Purity in [0.5,1]
 */
static void test_C1_tensor_auto_sync(void)
{
    printf("\n--- C1: TensorDensity Auto-Sync (10 Ticks) ---\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);
    ProPhysics_Tensor_Mark_Nodes(&pu, pid);

    uint32_t rho_id = 0;
    ProPhysics_TensorDensity_Create(&pu, pid, &rho_id);
    ProPhysics_TensorDensity_From_Pair(&pu, rho_id, pid);

    ProPhysics_Density_Set_Auto_Sync(&pu, 1);

    for (int t = 0; t < 10; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    const double tr = ProPhysics_TensorDensity_Trace(&pu, rho_id);
    const double pu_ = ProPhysics_TensorDensity_Purity(&pu, rho_id);
    printf("  Tr = %.9f (Ziel 1), Purity = %.6f\n", tr, pu_);
    check("C1 Tr = 1", fabs(tr - 1.0) < 1e-4);
    check("C1 Purity in [0.5,1]", pu_ > 0.5 - 1e-3 && pu_ < 1.0 + 1e-3);

    ProPhysics_TensorDensity_Destroy(&pu, rho_id);
    ProPhysics_Tensor_Unmark_Nodes(&pu, pid);
    ProPhysics_Tensor_Destroy_Pair(&pu, pid);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- C2
 * FockDensity + auto_sync_density: <N> konsistent
 */
static void test_C2_fock_auto_sync(void)
{
    printf("\n--- C2: FockDensity Auto-Sync (10 Ticks) ---\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x03u);  /* |1,2> */

    uint32_t rho_id = 0;
    ProPhysics_FockDensity_Create(&pu, fid, &rho_id);
    ProPhysics_FockDensity_From_Fock(&pu, rho_id, fid);

    ProPhysics_Density_Set_Auto_Sync(&pu, 1);

    for (int t = 0; t < 10; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    const double tr = ProPhysics_FockDensity_Trace(&pu, rho_id);
    double N = 0.0;
    for (uint32_t m = 0; m < 8u; ++m) {
        N += ProPhysics_FockDensity_Mode_Occupation(&pu, rho_id, m);
    }
    printf("  Tr = %.9f (Ziel 1), <N> = %.6f (Ziel 2)\n", tr, N);
    check("C2 Tr = 1", fabs(tr - 1.0) < 1e-4);
    check("C2 <N> = 2", fabs(N - 2.0) < 1e-3);

    ProPhysics_FockDensity_Destroy(&pu, rho_id);
    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- C3
 * Bell + Sync: rho bleibt rein (kein Lindblad aktiv)
 */
static void test_C3_bell_sync_pure(void)
{
    printf("\n--- C3: Bell + Sync, rho bleibt rein ---\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);
    ProPhysics_Tensor_Mark_Nodes(&pu, pid);

    uint32_t rho_id = 0;
    ProPhysics_TensorDensity_Create(&pu, pid, &rho_id);
    ProPhysics_TensorDensity_From_Pair(&pu, rho_id, pid);

    const double pur_before = ProPhysics_TensorDensity_Purity(&pu, rho_id);

    ProPhysics_Density_Set_Auto_Sync(&pu, 1);
    for (int t = 0; t < 10; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }
    ProPhysics_TensorDensity_Sync_From_Pair(&pu, rho_id);

    const double pur_after = ProPhysics_TensorDensity_Purity(&pu, rho_id);
    printf("  Purity vor  = %.9f\n", pur_before);
    printf("  Purity nach = %.9f\n", pur_after);
    check("C3 Purity erhalten", fabs(pur_after - pur_before) < 1e-3);

    ProPhysics_TensorDensity_Destroy(&pu, rho_id);
    ProPhysics_Tensor_Unmark_Nodes(&pu, pid);
    ProPhysics_Tensor_Destroy_Pair(&pu, pid);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- C4
 * Slater(1,4) + Sync: rho[12,12]=0.5, rho[12,33]=-0.5
 */
static void test_C4_slater_elements(void)
{
    printf("\n--- C4: Slater(1,4) + Sync, rho-Elemente ---\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    uint32_t rho_id = 0;
    ProPhysics_TensorDensity_Create(&pu, pid, &rho_id);
    ProPhysics_TensorDensity_From_Pair(&pu, rho_id, pid);

    ProPhysics_Density_Set_Auto_Sync(&pu, 1);
    ProPhysics_TensorDensity_Sync_From_Pair(&pu, rho_id);

    int32_t r12_12 = 0, i12_12 = 0, r12_33 = 0, i12_33 = 0;
    ProPhysics_TensorDensity_Get_Element(&pu, rho_id, 12, 12, &r12_12, &i12_12);
    ProPhysics_TensorDensity_Get_Element(&pu, rho_id, 12, 33, &r12_33, &i12_33);
    const double d12 = (double)r12_12 / 2147483647.0;
    const double d1233 = (double)r12_33 / 2147483647.0;

    printf("  rho[12,12] = %+.6f (Ziel +0.5)\n", d12);
    printf("  rho[12,33] = %+.6f (Ziel -0.5)\n", d1233);
    check("C4 rho[12,12]=+0.5", fabs(d12 - 0.5) < 1e-3);
    check("C4 rho[12,33]=-0.5", fabs(d1233 + 0.5) < 1e-3);

    ProPhysics_TensorDensity_Destroy(&pu, rho_id);
    ProPhysics_Tensor_Destroy_Pair(&pu, pid);
    ProPhysics_Free(&pu);
}

/* ---------------------------------------------------------------- C5
 * Ring-Test: Pair -> Tick -> Sync -> 100x, Tr-Drift < 1e-3
 */
static void test_C5_ring_drift(void)
{
    printf("\n--- C5: Ring-Test (100 Iterationen, Tr-Drift) ---\n");
    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);
    ProPhysics_Tensor_Mark_Nodes(&pu, pid);
    ProPhysics_Tensor_Set_Auto_Sync(&pu, 1);

    uint32_t rho_id = 0;
    ProPhysics_TensorDensity_Create(&pu, pid, &rho_id);
    ProPhysics_TensorDensity_From_Pair(&pu, rho_id, pid);

    ProPhysics_Density_Set_Auto_Sync(&pu, 1);

    for (int t = 0; t < 100; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
        ProPhysics_TensorDensity_Sync_From_Pair(&pu, rho_id);
    }

    const double tr = ProPhysics_TensorDensity_Trace(&pu, rho_id);
    printf("  Tr nach 100 Runden = %.9f (Ziel 1)\n", tr);
    check("C5 Tr-Drift < 1e-3", fabs(tr - 1.0) < 1e-3);

    ProPhysics_TensorDensity_Destroy(&pu, rho_id);
    ProPhysics_Tensor_Unmark_Nodes(&pu, pid);
    ProPhysics_Tensor_Destroy_Pair(&pu, pid);
    ProPhysics_Free(&pu);
}

/* =================================================================== main */
int main(void)
{
    /* Unbuffered: ermöglicht Crash-Lokalisierung per Logfile. */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("========================================================================\n");
    printf("  Etappe 16a + 16b + 16d -- Dichtematrix / Lindblad (ASCII)\n");
    printf("  8x8 Knoten-Dichte + 64x64 Tensor-Dichte + 256x256 Fock-Dichte\n");
    printf("========================================================================\n");

    /* --- Etappe 16a: 8x8 Dichte --- */
    test_D1_pure_basis();
    test_D2_pure_superposition();
    test_D3_mixture();
    test_D4_unitary();
    test_D5_amp_damp();
    test_D6_phase_damp();
    test_D7_depolarize();
    test_D8_tensor_rho_consistency();
    test_D9_purity_monotonie();
    test_D10_trace_preservation();

    /* --- Etappe 16b: 64x64 Tensor-Dichte --- */
    test_B1_pure_tensor_density();
    test_B2_partial_trace_consistency();
    test_B3_A_depolarize();
    test_B4_A_amp_damp();
    test_B5_both_depolarize();
    test_B6_trace_all_channels();

    /* --- Etappe 16d: 256x256 Fock-Dichte --- */
    test_E1_vacuum();
    test_E2_superposition();
    test_E3_mixture();
    test_E4_partial_trace();
    test_E5_amp_damp_mode0();
    test_E6_phase_damp();
    test_E7_trace_all_channels();
    test_E8_all_modes_decay();

    /* --- Etappe 16c: Dichte-Auto-Sync --- */
    test_C1_tensor_auto_sync();
    test_C2_fock_auto_sync();
    test_C3_bell_sync_pure();
    test_C4_slater_elements();
    test_C5_ring_drift();

    printf("\n========================================================================\n");
    printf("  Ergebnis: %d PASS, %d FAIL\n", g_pass, g_fail);
    printf("========================================================================\n");
    return (g_fail == 0) ? 0 : 1;
}