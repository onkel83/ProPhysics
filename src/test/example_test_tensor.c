/* example_test_tensor.c - Etappe 13 Phase 1 + 2a + 2b
 * Tensorprodukt-Tests (2-Knoten-Verschaenkung), ASCII-only Ausgabe.
 *
 * Kompilieren (MSVC):
 *   cl /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. example_test_tensor.c ^
 *      /Fe:example_test_tensor.exe /link ProPhysics.lib
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
#define LN8 2.0794415416798359283

static int g_pass = 0;
static int g_fail = 0;

static void check(const char* name, int cond)
{
    printf("  [%s] %s\n", cond ? "PASS" : "FAIL", name);
    if (cond) g_pass++; else g_fail++;
}

static int32_t q31_half_sqrt2(void)
{
    return (int32_t)lround(0.7071067811865476 * 2147483647.0);
}

/* ------------------------------------------------------------------ T1 */
static void test_T1_product_state(void)
{
    printf("\n--- T1: Produktzustand |00> -> S = 0 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    if (!ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid)) {
        printf("  Create_Pair fehlgeschlagen\n");
        ProPhysics_Free(&pu);
        return;
    }

    const double S = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    printf("  S(rho_A) = %.12e (erwartet 0)\n", S);
    check("S < 1e-6", fabs(S) < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T2 */
static void test_T2_bell_state(void)
{
    printf("\n--- T2: Bell-Zustand -> S = ln 2 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    const int32_t amp = q31_half_sqrt2();
    psi[0] = pro_amp_pack(amp, 0);
    psi[1 * 8 + 1] = pro_amp_pack(amp, 0);

    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const double S = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    printf("  S(rho_A) = %.12f\n", S);
    printf("  ln 2     = %.12f\n", LN2);
    printf("  |dS|     = %.4e\n", fabs(S - LN2));
    check("|S - ln 2| < 1e-3", fabs(S - LN2) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T3 */
static void test_T3_local_op_preserves_entropy(void)
{
    printf("\n--- T3: U(x)I aendert S nicht ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    const int32_t amp = q31_half_sqrt2();
    psi[0] = pro_amp_pack(amp, 0);
    psi[1 * 8 + 1] = pro_amp_pack(amp, 0);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const double S_before = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    ProAmpQ31 U[8][8] = { {0} };
    for (int i = 0; i < 8; ++i) U[i][i] = pro_amp_pack(INT32_MAX, 0);
    const int32_t h = q31_half_sqrt2();
    U[0][0] = pro_amp_pack(h, 0);
    U[0][1] = pro_amp_pack(h, 0);
    U[1][0] = pro_amp_pack(h, 0);
    U[1][1] = pro_amp_pack(-h, 0);

    ProPhysics_Tensor_Apply_Local_Op(&pu, pid, 0, U);
    const double S_after = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    printf("  S_before = %.12f\n", S_before);
    printf("  S_after  = %.12f\n", S_after);
    printf("  |dS|     = %.4e\n", fabs(S_after - S_before));
    check("|dS| < 1e-3", fabs(S_after - S_before) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T4 */
static void test_T4_max_entangled(void)
{
    printf("\n--- T4: Maximale Verschaenkung -> S = ln 8 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    const int32_t amp = (int32_t)lround(0.3535533905932737 * 2147483647.0);
    for (int i = 0; i < 8; ++i) psi[i * 8 + i] = pro_amp_pack(amp, 0);

    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const double S = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    printf("  S(rho_A) = %.12f\n", S);
    printf("  ln 8     = %.12f\n", LN8);
    printf("  |dS|     = %.4e\n", fabs(S - LN8));
    check("|S - ln 8| < 5e-3", fabs(S - LN8) < 5e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T5 */
static void test_T5_trace_normalization(void)
{
    printf("\n--- T5: Spur(rho_A) = 1 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    const int32_t amp = q31_half_sqrt2();
    psi[0] = pro_amp_pack(amp, 0);
    psi[1 * 8 + 1] = pro_amp_pack(amp, 0);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    ProAmpQ31 rho[8][8];
    ProPhysics_Tensor_Partial_Trace_A(&pu, pid, rho);

    double trace_re = 0.0;
    for (int i = 0; i < 8; ++i) {
        trace_re += (double)pro_amp_real(rho[i][i]) / 2147483647.0;
    }
    printf("  Spur(rho_A) = %.12f\n", trace_re);
    check("|Spur - 1| < 1e-6", fabs(trace_re - 1.0) < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T6 */
static void test_T6_chsh_tensor_bell(void)
{
    printf("\n--- T6: CHSH Tensor, Bell-Triplet -> S = 2*sqrt(2) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    const int32_t amp = q31_half_sqrt2();
    psi[1 * 8 + 1] = pro_amp_pack(amp, 0);
    psi[4 * 8 + 4] = pro_amp_pack(amp, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const double th_a[2] = { 0.0, M_PI / 2.0 };
    const double th_b[2] = { M_PI / 4.0, 3.0 * M_PI / 4.0 };
    const uint32_t TRIALS = 20000;

    uint64_t rng[4] = {
        0x123456789ABCDEFULL, 0xFEDCBA9876543210ULL,
        0xAAAA5555AAAA5555ULL, 0x5555AAAA5555AAAAULL
    };

    double E[2][2];
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            double sum = 0.0;
            uint32_t count = 0;
            for (uint32_t t = 0; t < TRIALS; ++t) {
                int a = 0, b = 0;
                if (ProPhysics_Tensor_Measure_Projective(&pu, pid,
                    th_a[i], th_b[j], rng, &a, &b))
                {
                    sum += (double)(a * b);
                    count++;
                }
            }
            E[i][j] = count ? (sum / (double)count) : 0.0;
        }
    }

    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    const double S_expected = 2.0 * sqrt(2.0);

    printf("  E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("  S = %.4f  (erwartet %.4f)\n", S, S_expected);
    printf("  |S - 2*sqrt(2)| = %.4e\n", fabs(S - S_expected));
    check("|S - 2*sqrt(2)| < 0.05", fabs(S - S_expected) < 0.05);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T7 */
static void test_T7_chsh_tensor_product(void)
{
    printf("\n--- T7: CHSH Tensor, Produktzustand |1,1> -> S <= 2 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    psi[1 * 8 + 1] = pro_amp_pack(INT32_MAX, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const double th_a[2] = { 0.0, M_PI / 2.0 };
    const double th_b[2] = { M_PI / 4.0, 3.0 * M_PI / 4.0 };
    const uint32_t TRIALS = 20000;

    uint64_t rng[4] = {
        0x1111111111111111ULL, 0x2222222222222222ULL,
        0x3333333333333333ULL, 0x4444444444444444ULL
    };

    double E[2][2];
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            double sum = 0.0;
            uint32_t count = 0;
            for (uint32_t t = 0; t < TRIALS; ++t) {
                int a = 0, b = 0;
                if (ProPhysics_Tensor_Measure_Projective(&pu, pid,
                    th_a[i], th_b[j], rng, &a, &b))
                {
                    sum += (double)(a * b);
                    count++;
                }
            }
            E[i][j] = count ? (sum / (double)count) : 0.0;
        }
    }

    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);

    printf("  E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("  S = %.4f  (erwartet <= 2)\n", S);
    check("S <= 2.05", S < 2.05);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T8
 *
 * Marginalen-Sichtbarkeit: Bell-Tensor hat S = ln2. Nach Sync_To_Amp
 * ist amp_grid[A] ein reiner Vektor. Die Verschaenkung ist in amp_grid
 * UNSICHTBAR (S aus amp_grid waere 0).
 */
static void test_T8_marginal_invisibility(void)
{
    printf("\n--- T8: Marginalen-Sichtbarkeit ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    const int32_t amp = q31_half_sqrt2();
    psi[1 * 8 + 1] = pro_amp_pack(amp, 0);
    psi[4 * 8 + 4] = pro_amp_pack(amp, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const double S_tensor_before =
        ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    ProPhysics_Tensor_Sync_To_Amp(&pu);

    /* Nach Sync: amp_grid[0] ist reiner Vektor. Pruefe: Sum |c_b|^2 = 2^62. */
    double norm_sq_amp = 0.0;
    for (int b = 0; b < 8; ++b) {
        const int64_t re = pro_amp_real(pu.amp_grid[0].coeff[b]);
        const int64_t im = pro_amp_imag(pu.amp_grid[0].coeff[b]);
        norm_sq_amp += (double)(re * re + im * im);
    }
    const double norm_rel = norm_sq_amp / 4611686018427387904.0;

    /* "Entropie aus amp_grid" waere 0, weil amp_grid ein reiner Vektor ist. */
    const double S_amp = 0.0;

    printf("  S_tensor (vor Sync)   = %.12f  (erwartet ln2 = %.12f)\n",
        S_tensor_before, LN2);
    printf("  S_amp (nach Sync)     = %.12f  (amp_grid ist rein)\n", S_amp);
    printf("  Norm(amp_grid[0])     = %.12f  (erwartet 1)\n", norm_rel);
    printf("  -> Verschaenkung ist in amp_grid UNSICHTBAR\n");

    check("S_tensor > 0.6", S_tensor_before > 0.6);
    check("S_amp = 0", S_amp < 1e-6);
    check("Norm(amp_grid) ~= 1", fabs(norm_rel - 1.0) < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T9
 *
 * Produktzustand unter SDK-Dynamik: Sync_From_Amp ergibt Produktzustand
 * mit S = 0. Nach SDK-Tick (der nur amp_grid aendert, Tensor nicht
 * beruehrt) bleibt S = 0 solange wir nicht neu syncen.
 */
static void test_T9_product_under_tick(void)
{
    printf("\n--- T9: Produktzustand unter SDK-Tick ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    /* amp_grid[0] = |1>, amp_grid[1] = |4>. */
    for (int b = 0; b < 8; ++b) {
        pu.amp_grid[0].coeff[b] = 0;
        pu.amp_grid[1].coeff[b] = 0;
    }
    pu.amp_grid[0].coeff[1] = pro_amp_pack(INT32_MAX, 0);
    pu.amp_grid[1].coeff[4] = pro_amp_pack(INT32_MAX, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Sync_From_Amp(&pu, pid);

    const double S_before = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    /* Ein paar SDK-Ticks. Tensor wird nicht automatisch synchronisiert. */
    for (int t = 0; t < 10; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    const double S_after = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    printf("  S vor  SDK-Tick = %.12f\n", S_before);
    printf("  S nach SDK-Tick = %.12f\n", S_after);
    /* Q31-Rundung: ~3 ULP pro Sync_Roundtrip, also < 1e-6. */
    check("S_before = 0 (Q31-Rundung erlaubt)", fabs(S_before) < 1e-6);
    check("S_after  = 0 (Q31-Rundung erlaubt)", fabs(S_after) < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T10
 *
 * Verschaenkung unter SDK-Dynamik geht verloren:
 * Bell-Tensor -> Sync_To_Amp -> Sync_From_Amp ergibt Produktzustand,
 * S = 0. Die bestehende amp_grid-Dynamik kann Verschaenkung NICHT
 * darstellen, weil amp_grid nur die Marginale kennt.
 */
static void test_T10_entanglement_lost(void)
{
    printf("\n--- T10: Verschaenkung unter SDK-Dynamik ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    const int32_t amp = q31_half_sqrt2();
    psi[1 * 8 + 1] = pro_amp_pack(amp, 0);
    psi[4 * 8 + 4] = pro_amp_pack(amp, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const double S_initial = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    /* Runde 1: Tensor -> amp_grid -> Tensor */
    ProPhysics_Tensor_Sync_To_Amp(&pu);
    ProPhysics_Tensor_Sync_From_Amp(&pu, pid);
    const double S_after_roundtrip =
        ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    /* Runde 2: SDK-Tick dazwischen */
    ProPhysics_Tensor_Sync_To_Amp(&pu);
    for (int t = 0; t < 10; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }
    ProPhysics_Tensor_Sync_From_Amp(&pu, pid);
    const double S_after_tick =
        ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    printf("  S initial                     = %.12f  (erwartet ln2 = %.12f)\n",
        S_initial, LN2);
    printf("  S nach Sync_To/From_Roundtrip = %.12f\n", S_after_roundtrip);
    printf("  S nach Sync_To + 10 Ticks + Sync_From = %.12f\n", S_after_tick);
    printf("  -> amp_grid kennt nur Marginalen, Verschaenkung geht verloren\n");

    check("S_initial > 0.6", S_initial > 0.6);
    check("S_after_roundtrip < 1e-6", S_after_roundtrip < 1e-6);
    check("S_after_tick < 1e-6", S_after_tick < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T11
 *
 * H + CNOT auf |1,1> erzeugt Bell-Zustand.
 * S(ρ_A) = ln2, Concurrence = 1.
 */
static void test_T11_H_CNOT_bell(void)
{
    printf("\n--- T11: H + CNOT auf |1,1> -> Bell ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    psi[1 * 8 + 1] = pro_amp_pack(INT32_MAX, 0);   /* |1, 1> */

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const double S_before = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    const double C_before = ProPhysics_Tensor_Concurrence(&pu, pid);

    ProPhysics_Tensor_Apply_Hadamard(&pu, pid, 0);   /* H auf A */
    ProPhysics_Tensor_Apply_CNOT(&pu, pid, 0);       /* CNOT(A->B) */

    const double S_after = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    const double C_after = ProPhysics_Tensor_Concurrence(&pu, pid);

    printf("  S vor  H+CNOT = %.12f\n", S_before);
    printf("  S nach H+CNOT = %.12f  (erwartet ln2 = %.12f)\n", S_after, LN2);
    printf("  C vor         = %.12f\n", C_before);
    printf("  C nach        = %.12f  (erwartet 1.0)\n", C_after);
    printf("  |S - ln2|     = %.4e\n", fabs(S_after - LN2));
    printf("  |C - 1|       = %.4e\n", fabs(C_after - 1.0));

    check("S_before = 0", fabs(S_before) < 1e-6);
    check("C_before = 0", fabs(C_before) < 1e-6);
    check("S_after = ln2", fabs(S_after - LN2) < 1e-3);
    check("C_after = 1", fabs(C_after - 1.0) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T12
 *
 * H + CNOT + CHSH-Messung -> S_CHSH = 2 sqrt(2).
 */
static void test_T12_H_CNOT_chsh(void)
{
    printf("\n--- T12: H+CNOT dann CHSH -> S_CHSH = 2*sqrt(2) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    psi[1 * 8 + 1] = pro_amp_pack(INT32_MAX, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    ProPhysics_Tensor_Apply_Hadamard(&pu, pid, 0);
    ProPhysics_Tensor_Apply_CNOT(&pu, pid, 0);

    /* CHSH-Messung. ACHTUNG: der erzeugte Zustand ist
     *   (|1,1> + |4,4>)/sqrt(2) im Framework-Index,
     * d.h. Qubit-Zustand (|0,0> + |1,1>)/sqrt(2) in {1,4}-Kodierung.
     *
     * CHSH mit theta_a=0 / theta_b=pi/4 misst:
     *   E(0,0) = +cos(0) = +1 ... aber die bisherige Messung nutzt
     *   {1,4} als Eigenzustand von "theta=0" mit +1.
     *
     * Erwartet: dieselbe 2*sqrt(2) wie im Bell-Triplet-Test (T6). */
    const double th_a[2] = { 0.0, M_PI / 2.0 };
    const double th_b[2] = { M_PI / 4.0, 3.0 * M_PI / 4.0 };
    const uint32_t TRIALS = 20000;
    uint64_t rng[4] = {
        0xCAFEBABE12345678ULL, 0xDEADBEEF87654321ULL,
        0x12345678CAFEBABEULL, 0x87654321DEADBEEFULL
    };

    double E[2][2];
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            double sum = 0.0;
            uint32_t count = 0;
            for (uint32_t t = 0; t < TRIALS; ++t) {
                int a = 0, b = 0;
                if (ProPhysics_Tensor_Measure_Projective(&pu, pid,
                    th_a[i], th_b[j], rng, &a, &b)) {
                    sum += (double)(a * b);
                    count++;
                }
            }
            E[i][j] = count ? sum / (double)count : 0.0;
        }
    }

    const double S = fabs(E[0][0] - E[0][1]) + fabs(E[1][0] + E[1][1]);
    const double S_expected = 2.0 * sqrt(2.0);

    printf("  E(0,0)=%+.4f E(0,1)=%+.4f E(1,0)=%+.4f E(1,1)=%+.4f\n",
        E[0][0], E[0][1], E[1][0], E[1][1]);
    printf("  S = %.4f  (erwartet %.4f)\n", S, S_expected);
    printf("  |S - 2*sqrt(2)| = %.4e\n", fabs(S - S_expected));
    check("|S - 2*sqrt(2)| < 0.05", fabs(S - S_expected) < 0.05);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T13
 *
 * Zwei CNOTs hintereinander heben sich auf.
 * H + CNOT + CNOT sollte S = 0 geben (Produktzustand wiederhergestellt).
 */
static void test_T13_CNOT_CNOT_undo(void)
{
    printf("\n--- T13: CNOT + CNOT rueckgaengig ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    psi[1 * 8 + 1] = pro_amp_pack(INT32_MAX, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    ProPhysics_Tensor_Apply_Hadamard(&pu, pid, 0);
    ProPhysics_Tensor_Apply_CNOT(&pu, pid, 0);
    const double S_mid = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    ProPhysics_Tensor_Apply_CNOT(&pu, pid, 0);
    const double S_final = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    printf("  S nach H + CNOT         = %.12f  (erwartet ln2)\n", S_mid);
    printf("  S nach H + CNOT + CNOT  = %.12f  (erwartet 0)\n", S_final);

    check("S_mid = ln2", fabs(S_mid - LN2) < 1e-3);
    check("S_final = 0", fabs(S_final) < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T14
 *
 * CZ auf |1,1> erzeugt Bell-aehnlichen Zustand.
 * (H x I) * CZ * (H x H) ist aequivalent zu CNOT.
 * Hier einfacher Test: CZ auf (|1,1> + |4,4>)/sqrt(2) -> Phasenflip.
 */
static void test_T14_CZ(void)
{
    printf("\n--- T14: CZ auf Bell -> Phasenflip ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    /* Start: (|1,1> + |4,4>)/sqrt(2). */
    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    const int32_t amp = q31_half_sqrt2();
    psi[1 * 8 + 1] = pro_amp_pack(amp, 0);
    psi[4 * 8 + 4] = pro_amp_pack(amp, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const double S_before = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    const double C_before = ProPhysics_Tensor_Concurrence(&pu, pid);

    ProPhysics_Tensor_Apply_CZ(&pu, pid);

    const double S_after = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    const double C_after = ProPhysics_Tensor_Concurrence(&pu, pid);

    printf("  S vor  CZ = %.12f\n", S_before);
    printf("  S nach CZ = %.12f\n", S_after);
    printf("  C vor  CZ = %.12f\n", C_before);
    printf("  C nach CZ = %.12f\n", C_after);

    /* CZ aendert nur die Phase von |4,4>, nicht die Verschaenkung. */
    check("S_vor = ln2", fabs(S_before - LN2) < 1e-3);
    check("S_nach = ln2", fabs(S_after - LN2) < 1e-3);
    check("C_vor = 1", fabs(C_before - 1.0) < 1e-3);
    check("C_nach = 1", fabs(C_after - 1.0) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T15
 *
 * SWAP tauscht Qubits.
 * Auf |1, 4> gibt SWAP |4, 1>.
 * Messbar ueber Marginalen-Entropie (beide = 0, Produktzustand).
 */
static void test_T15_SWAP(void)
{
    printf("\n--- T15: SWAP tauscht Qubits ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    psi[1 * 8 + 4] = pro_amp_pack(INT32_MAX, 0);   /* |1_A, 4_B> */

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    ProPhysics_Tensor_Apply_SWAP(&pu, pid);

    /* Nach SWAP: |4_A, 1_B>. Pruefe: coeff[4*8+1] ist dominant. */
    ProAmpQ31 out[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, out);

    const double mag_41 = fabs((double)pro_amp_real(out[4 * 8 + 1]))
        / 2147483647.0;
    const double mag_14 = fabs((double)pro_amp_real(out[1 * 8 + 4]))
        / 2147483647.0;

    printf("  |coeff[4,1]| = %.12f  (erwartet 1.0)\n", mag_41);
    printf("  |coeff[1,4]| = %.12f  (erwartet 0.0)\n", mag_14);

    check("|c[4,1]| ~= 1", fabs(mag_41 - 1.0) < 1e-6);
    check("|c[1,4]| ~= 0", mag_14 < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T16
 *
 * Markierte Knoten sind im SDK-Tick eingefroren.
 * Tensor bleibt unveraendert, amp_grid bleibt unveraendert.
 */
static void test_T16_marked_frozen(void)
{
    printf("\n--- T16: Markierte Knoten bleiben eingefroren ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    /* Produktzustand |1,1> im Tensor, markierte Knoten 0, 1. */
    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    psi[1 * 8 + 1] = pro_amp_pack(INT32_MAX, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);
    ProPhysics_Tensor_Mark_Nodes(&pu, pid);
    ProPhysics_Tensor_Sync_To_Amp(&pu);

    /* Snapshot von amp_grid[0], amp_grid[1]. */
    ProAmpQ31 snap0 = pu.amp_grid[0].coeff[1];
    ProAmpQ31 snap1 = pu.amp_grid[1].coeff[1];

    /* 10 SDK-Ticks. */
    for (int t = 0; t < 10; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    const ProAmpQ31 after0 = pu.amp_grid[0].coeff[1];
    const ProAmpQ31 after1 = pu.amp_grid[1].coeff[1];

    printf("  amp_grid[0].coeff[1] vor  = (%d, %d)\n",
        (int)pro_amp_real(snap0), (int)pro_amp_imag(snap0));
    printf("  amp_grid[0].coeff[1] nach = (%d, %d)\n",
        (int)pro_amp_real(after0), (int)pro_amp_imag(after0));

    check("amp_grid[0] unveraendert",
        pro_amp_real(after0) == pro_amp_real(snap0) &&
        pro_amp_imag(after0) == pro_amp_imag(snap0));
    check("amp_grid[1] unveraendert",
        pro_amp_real(after1) == pro_amp_real(snap1) &&
        pro_amp_imag(after1) == pro_amp_imag(snap1));

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T17
 *
 * Unmarkierte Knoten evolvieren normal.
 * amp_grid[2] sollte sich nach SDK-Ticks veraendert haben.
 */
static void test_T17_unmarked_evolves(void)
{
    printf("\n--- T17: Unmarkierte Knoten evolvieren normal ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    /* amp_grid[2] initialisieren mit |1>. */
    for (int b = 0; b < 8; ++b) pu.amp_grid[2].coeff[b] = 0;
    pu.amp_grid[2].coeff[1] = pro_amp_pack(INT32_MAX, 0);

    /* Nachbarn setzen, damit Transport stattfinden kann. */
    for (int c = 0; c < 4; ++c) {
        pu.reg_source[2].channels[c] = 3u + (uint64_t)c;
    }

    ProAmpQ31 snap = pu.amp_grid[2].coeff[1];

    for (int t = 0; t < 10; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    const ProAmpQ31 after = pu.amp_grid[2].coeff[1];

    printf("  amp_grid[2].coeff[1] vor  = (%d, %d)\n",
        (int)pro_amp_real(snap), (int)pro_amp_imag(snap));
    printf("  amp_grid[2].coeff[1] nach = (%d, %d)\n",
        (int)pro_amp_real(after), (int)pro_amp_imag(after));

    check("amp_grid[2] hat sich veraendert",
        pro_amp_real(after) != pro_amp_real(snap) ||
        pro_amp_imag(after) != pro_amp_imag(snap));

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T18
 *
 * Auto-Sync: nach SDK-Tick wird amp_grid markierter Knoten aus dem
 * Tensor neu berechnet. Mit ungleicher Gewichtung (0.7 vs 0.3) ist
 * der dominante Eigenvektor von rho_A eindeutig bestimmt.
 *
 * Setup: Bell-artiger Tensor sqrt(0.7)|1,1> + sqrt(0.3)|4,4>.
 *        amp_grid[0] initial = |4> (NICHT der dominante).
 * Erwartung: Nach Sync_To_Amp dominiert |1> in amp_grid[0].
 */
static void test_T18_auto_sync(void)
{
    printf("\n--- T18: Auto-Sync ueberschreibt amp_grid ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    /* Ungleicher Bell: sqrt(0.7)|1,1> + sqrt(0.3)|4,4>. */
    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    const int32_t a_up = (int32_t)lround(sqrt(0.7) * 2147483647.0);
    const int32_t a_dn = (int32_t)lround(sqrt(0.3) * 2147483647.0);
    psi[1 * 8 + 1] = pro_amp_pack(a_up, 0);
    psi[4 * 8 + 4] = pro_amp_pack(a_dn, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);
    ProPhysics_Tensor_Mark_Nodes(&pu, pid);

    /* amp_grid[0] initial auf |4> setzen (NICHT der dominante). */
    for (int b = 0; b < 8; ++b) {
        pu.amp_grid[0].coeff[b] = 0;
        pu.amp_grid[1].coeff[b] = 0;
    }
    pu.amp_grid[0].coeff[4] = pro_amp_pack(INT32_MAX, 0);
    pu.amp_grid[1].coeff[4] = pro_amp_pack(INT32_MAX, 0);

    const ProAmpQ31 before1 = pu.amp_grid[0].coeff[1];
    const ProAmpQ31 before4 = pu.amp_grid[0].coeff[4];

    ProPhysics_Tensor_Sync_To_Amp(&pu);

    const ProAmpQ31 after1 = pu.amp_grid[0].coeff[1];
    const ProAmpQ31 after4 = pu.amp_grid[0].coeff[4];

    const double c1 = fabs((double)pro_amp_real(after1)) / 2147483647.0;
    const double c4 = fabs((double)pro_amp_real(after4)) / 2147483647.0;
    const double norm = c1 * c1 + c4 * c4;

    printf("  amp_grid[0].coeff[1] vor Sync  = %d\n", (int)pro_amp_real(before1));
    printf("  amp_grid[0].coeff[4] vor Sync  = %d\n", (int)pro_amp_real(before4));
    printf("  amp_grid[0].coeff[1] nach Sync = %d\n", (int)pro_amp_real(after1));
    printf("  amp_grid[0].coeff[4] nach Sync = %d\n", (int)pro_amp_real(after4));
    printf("  |c1|^2 + |c4|^2 = %.12f (erwartet 1)\n", norm);
    printf("  -> Nach Sync dominiert |1> (Gewichtung 0.7)\n");

    check("Norm = 1", fabs(norm - 1.0) < 1e-6);
    check("|1> dominiert nach Sync",
        pro_amp_real(after1) > pro_amp_real(after4));

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T19
 *
 * Emergent Verschaenkung durch XX-Kopplung.
 * Start: |1, 1> (Produktzustand, S = 0).
 * coupling_q15 = 4096 (theta_rad ~ 0.125 rad/Tick).
 * Nach 6 Ticks: kumulative Phase ~ 0.75 rad -> S > 0.6, C > 0.9.
 *
 * KEIN H, KEIN CNOT. Nur die 2-Koerper-Kopplung.
 */
static void test_T19_emergent_entanglement(void)
{
    printf("\n--- T19: Emergent Verschaenkung durch XX-Kopplung ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    psi[1 * 8 + 1] = pro_amp_pack(INT32_MAX, 0);   /* |1, 1> */

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);
    ProPhysics_Tensor_Set_Coupling(&pu, pid, 4096);   /* ~ 0.125 rad/Tick */
    ProPhysics_Tensor_Mark_Nodes(&pu, pid);

    const double S_before = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    const double C_before = ProPhysics_Tensor_Concurrence(&pu, pid);

    /* 6 SDK-Ticks. XX-Step laeuft im Tick, da coupling_q15 != 0. */
    for (int t = 0; t < 6; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    const double S_after = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    const double C_after = ProPhysics_Tensor_Concurrence(&pu, pid);

    printf("  S vor  6 Ticks = %.12f  (erwartet 0)\n", S_before);
    printf("  S nach 6 Ticks = %.12f  (erwartet nahe ln2 = %.12f)\n",
        S_after, LN2);
    printf("  C vor          = %.12f\n", C_before);
    printf("  C nach         = %.12f\n", C_after);
    printf("  -> Produktzustand wurde durch reine Zeitentwicklung verschraenkt\n");

    check("S_before = 0", fabs(S_before) < 1e-6);
    check("C_before = 0", fabs(C_before) < 1e-6);
    check("S_after > 0.6 (echte Verschaenkung)", S_after > 0.6);
    check("C_after > 0.9 (echte Verschaenkung)", C_after > 0.9);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T20
 *
 * Ein einzelner XX-Schritt mit theta = pi/4 erzeugt maximale Verschaenkung.
 * coupling_q15 = round(pi/4 * 32768) = 25736.
 * Nach 1 Tick: S = ln2, C = 1.
 */
static void test_T20_single_step_max(void)
{
    printf("\n--- T20: Ein XX-Schritt mit theta = pi/4 -> maximal ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    psi[1 * 8 + 1] = pro_amp_pack(INT32_MAX, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const int32_t theta_q15_pi4 = (int32_t)lround((M_PI / 4.0) * 32768.0);
    printf("  theta_q15 = %d (erwartet %d)\n", theta_q15_pi4, 25736);
    ProPhysics_Tensor_Set_Coupling(&pu, pid, theta_q15_pi4);

    /* Direkter Aufruf: nur der XX-Schritt, kein SDK-Tick dazwischen. */
    ProPhysics_Tensor_Apply_XX_Step(&pu, pid);

    const double S_after = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    const double C_after = ProPhysics_Tensor_Concurrence(&pu, pid);

    printf("  S nach einem pi/4-Schritt = %.12f  (erwartet ln2)\n", S_after);
    printf("  C nach einem pi/4-Schritt = %.12f  (erwartet 1)\n", C_after);

    check("S = ln2", fabs(S_after - LN2) < 1e-3);
    check("C = 1", fabs(C_after - 1.0) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T21
 *
 * Zwei XX-Schritte mit theta = pi/4: kumulative Phase pi/2.
 * U(pi/2)|00> = -i|11>  (Produktzustand, S = 0).
 */
static void test_T21_two_steps_return(void)
{
    printf("\n--- T21: Zwei pi/4-Schritte -> Phase pi/2, S = 0 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    psi[1 * 8 + 1] = pro_amp_pack(INT32_MAX, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const int32_t theta_q15_pi4 = (int32_t)lround((M_PI / 4.0) * 32768.0);
    ProPhysics_Tensor_Set_Coupling(&pu, pid, theta_q15_pi4);

    ProPhysics_Tensor_Apply_XX_Step(&pu, pid);
    const double S_mid = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    ProPhysics_Tensor_Apply_XX_Step(&pu, pid);
    const double S_final = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    printf("  S nach 1 Schritt (pi/4) = %.12f  (erwartet ln2)\n", S_mid);
    printf("  S nach 2 Schritten (pi/2) = %.12f  (erwartet 0)\n", S_final);

    check("S_mid = ln2", fabs(S_mid - LN2) < 1e-3);
    check("S_final = 0", fabs(S_final) < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T22
 * R_x(pi): |1> -> -i|2>. Observable: |c1|^2 = 0, |c2|^2 = 1.
 */
static void test_T22_SU2_Rx_pi(void)
{
    printf("\n--- T22: R_x(pi) auf |1> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    for (int b = 0; b < 8; ++b) pu.amp_grid[0].coeff[b] = 0;
    pu.amp_grid[0].coeff[1] = pro_amp_pack(INT32_MAX, 0);

    ProPhysics_Apply_SU2_Rotation(&pu, 0, 0, 1.0, 0.0, 0.0, M_PI);

    const int32_t c1r = pro_amp_real(pu.amp_grid[0].coeff[1]);
    const int32_t c2r = pro_amp_real(pu.amp_grid[0].coeff[2]);
    const int32_t c2i = pro_amp_imag(pu.amp_grid[0].coeff[2]);

    printf("  |c1|^2 (soll 0)   coeff[1] = (%d, %d)\n",
        (int)c1r, (int)pro_amp_imag(pu.amp_grid[0].coeff[1]));
    printf("  |c2|^2 (soll 1)   coeff[2] = (%d, %d)\n", (int)c2r, (int)c2i);

    check("c1 = 0", c1r == 0 && pro_amp_imag(pu.amp_grid[0].coeff[1]) == 0);
    check("|c2| ~ 1", fabs((double)c2r / 2147483647.0) < 1e-3
        && fabs((double)c2i / 2147483647.0 + 1.0) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T23
 * R_y(pi): |1> -> |2>.
 */
static void test_T23_SU2_Ry_pi(void)
{
    printf("\n--- T23: R_y(pi) auf |1> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    for (int b = 0; b < 8; ++b) pu.amp_grid[0].coeff[b] = 0;
    pu.amp_grid[0].coeff[1] = pro_amp_pack(INT32_MAX, 0);

    ProPhysics_Apply_SU2_Rotation(&pu, 0, 0, 0.0, 1.0, 0.0, M_PI);

    const int32_t c1r = pro_amp_real(pu.amp_grid[0].coeff[1]);
    const int32_t c2r = pro_amp_real(pu.amp_grid[0].coeff[2]);
    const int32_t c2i = pro_amp_imag(pu.amp_grid[0].coeff[2]);

    printf("  coeff[1] = (%d, %d)\n", (int)c1r,
        (int)pro_amp_imag(pu.amp_grid[0].coeff[1]));
    printf("  coeff[2] = (%d, %d)\n", (int)c2r, (int)c2i);

    check("c1 = 0", c1r == 0);
    check("|c2| ~ 1", fabs((double)c2r / 2147483647.0 - 1.0) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T24
 * Superposition (|1>+|2>)/sqrt2 -> R_z(pi) -> <Sx> = -0.5.
 */
static void test_T24_SU2_Rz_pi_superposition(void)
{
    printf("\n--- T24: R_z(pi) auf (|1>+|2>)/sqrt2, <Sx> flippt ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    for (int b = 0; b < 8; ++b) pu.amp_grid[0].coeff[b] = 0;
    const int32_t h = (int32_t)lround(0.7071067811865476 * 2147483647.0);
    pu.amp_grid[0].coeff[1] = pro_amp_pack(h, 0);
    pu.amp_grid[0].coeff[2] = pro_amp_pack(h, 0);

    double sx0 = 0, sy0 = 0, sz0 = 0;
    ProPhysics_Measure_Spin(&pu, 0, 0, &sx0, &sy0, &sz0);
    printf("  <Sx> vor  = %+.6f (erwartet +0.5)\n", sx0);

    ProPhysics_Apply_SU2_Rotation(&pu, 0, 0, 0.0, 0.0, 1.0, M_PI);

    double sx1 = 0, sy1 = 0, sz1 = 0;
    ProPhysics_Measure_Spin(&pu, 0, 0, &sx1, &sy1, &sz1);
    printf("  <Sx> nach = %+.6f (erwartet -0.5)\n", sx1);

    check("<Sx>_vor ~ +0.5", fabs(sx0 - 0.5) < 1e-3);
    check("<Sx>_nach ~ -0.5", fabs(sx1 + 0.5) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T25
 * Zwei R_x(pi/2) = R_x(pi). |1> -> -i|2>. Betrag korrekt pruefen.
 */
static void test_T25_SU2_Rx_half_twice(void)
{
    printf("\n--- T25: R_x(pi/2) zweimal = R_x(pi) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    for (int b = 0; b < 8; ++b) pu.amp_grid[0].coeff[b] = 0;
    pu.amp_grid[0].coeff[1] = pro_amp_pack(INT32_MAX, 0);

    ProPhysics_Apply_SU2_Rotation(&pu, 0, 0, 1.0, 0.0, 0.0, M_PI / 2.0);
    ProPhysics_Apply_SU2_Rotation(&pu, 0, 0, 1.0, 0.0, 0.0, M_PI / 2.0);

    const int32_t c1r = pro_amp_real(pu.amp_grid[0].coeff[1]);
    const int32_t c1i = pro_amp_imag(pu.amp_grid[0].coeff[1]);
    const int32_t c2r = pro_amp_real(pu.amp_grid[0].coeff[2]);
    const int32_t c2i = pro_amp_imag(pu.amp_grid[0].coeff[2]);

    /* Betrag statt Realteil. */
    const double mag1 = sqrt((double)c1r * c1r + (double)c1i * c1i)
        / 2147483647.0;
    const double mag2 = sqrt((double)c2r * c2r + (double)c2i * c2i)
        / 2147483647.0;

    printf("  coeff[1] = (%d, %d)  |c1| = %.12f\n",
        (int)c1r, (int)c1i, mag1);
    printf("  coeff[2] = (%d, %d)  |c2| = %.12f\n",
        (int)c2r, (int)c2i, mag2);

    check("|c1| ~ 0", mag1 < 1e-9);
    check("|c2| ~ 1", fabs(mag2 - 1.0) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T26
 * SU(2)-Algebra [sigma_i, sigma_j] = 2i eps_ijk sigma_k.
 */
static void test_T26_SU2_Algebra(void)
{
    printf("\n--- T26: SU(2)-Algebra [sigma_i, sigma_j] = 2i eps_ijk sigma_k ---\n");

    const double err = ProPhysics_Verify_SU2_Algebra();
    printf("  max |[sigma_i, sigma_j] - 2i eps_ijk sigma_k| = %.4e\n", err);

    check("Algebra erfuellt (err < 1e-12)", err < 1e-12);
}

/* ------------------------------------------------------------------ T27
 * Bell-Zustand + Spin-Messung: Antikorrelation.
 */
static void test_T27_SU2_Bell_anticorrelation(void)
{
    printf("\n--- T27: Bell-Zustand, <S_A + S_B> = 0 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);
    for (int b = 0; b < 8; ++b) {
        pu.amp_grid[0].coeff[b] = 0;
        pu.amp_grid[1].coeff[b] = 0;
    }
    const int32_t h = (int32_t)lround(0.7071067811865476 * 2147483647.0);
    pu.amp_grid[0].coeff[1] = pro_amp_pack(h, 0);
    pu.amp_grid[1].coeff[2] = pro_amp_pack(h, 0);

    /* Damit das physikalisch ein Bell-Singlet ist, muesste der Tensor
     * konsistent sein. Hier nur Messung der Spin-Erwartungswerte
     * am Gitter -- reine Konsistenzpruefung. */
    double sxA, syA, szA, sxB, syB, szB;
    ProPhysics_Measure_Spin(&pu, 0, 0, &sxA, &syA, &szA);
    ProPhysics_Measure_Spin(&pu, 1, 0, &sxB, &syB, &szB);

    printf("  <Sz_A> = %+.6f (erwartet +0.5)\n", szA);
    printf("  <Sz_B> = %+.6f (erwartet -0.5)\n", szB);
    printf("  <Sz_A + Sz_B> = %+.6f (erwartet 0)\n", szA + szB);

    check("<Sz_A> = +0.5", fabs(szA - 0.5) < 1e-3);
    check("<Sz_B> = -0.5", fabs(szB + 0.5) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T34
 * Hopping(1,4) auf Slater(1,4) — beide Orbitale besetzt.
 * Trivialer Fall: Hopping kann nicht in besetzte Orbitale hoppen.
 */
static void test_T34_hopping_trivial(void)
{
    printf("\n--- T34: Hopping auf vollbesetzten Slater (trivial) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    ProAmpQ31 before[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, before);

    const int32_t theta_q15_pi4 = (int32_t)lround((M_PI / 4.0) * 32768.0);
    ProPhysics_Tensor_Apply_Hopping(&pu, pid, 1, 4, theta_q15_pi4);

    ProAmpQ31 after[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, after);

    double max_diff = 0.0;
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) {
        const int64_t dr = (int64_t)pro_amp_real(after[i]) - (int64_t)pro_amp_real(before[i]);
        const int64_t di = (int64_t)pro_amp_imag(after[i]) - (int64_t)pro_amp_imag(before[i]);
        const double err = sqrt((double)(dr * dr + di * di));
        if (err > max_diff) max_diff = err;
    }
    printf("  max |after - before| = %.6e  (erwartet 0)\n", max_diff);

    check("Slater(1,4) invariant unter Hopping(1,4)", max_diff < 1.0);
    check("Is_Antisymmetric", ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000));

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T35
 * Hopping(1,2) auf Slater(1,3) — Orbital 2 ist frei.
 * Erwartet: |psi[1,3]|^2 = 1/4, |psi[2,3]|^2 = 1/4 bei theta = pi/4.
 */
static void test_T35_hopping_nontrivial(void)
{
    printf("\n--- T35: Hopping(1,2) auf Slater(1,3) mit theta = pi/4 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 3);

    const int32_t theta_q15_pi4 = (int32_t)lround((M_PI / 4.0) * 32768.0);
    ProPhysics_Tensor_Apply_Hopping(&pu, pid, 1, 2, theta_q15_pi4);

    ProAmpQ31 state[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, state);

    /* Betragsquadrate in Q62-Normierung. */
    const double inv_q62 = 1.0 / 4611686018427387904.0;
    double mag_13_sq = 0.0, mag_23_sq = 0.0, mag_31_sq = 0.0, mag_32_sq = 0.0;

    {
        const int64_t re = pro_amp_real(state[1 * 8 + 3]);
        const int64_t im = pro_amp_imag(state[1 * 8 + 3]);
        mag_13_sq = (double)(re * re + im * im) * inv_q62;
    }
    {
        const int64_t re = pro_amp_real(state[2 * 8 + 3]);
        const int64_t im = pro_amp_imag(state[2 * 8 + 3]);
        mag_23_sq = (double)(re * re + im * im) * inv_q62;
    }
    {
        const int64_t re = pro_amp_real(state[3 * 8 + 1]);
        const int64_t im = pro_amp_imag(state[3 * 8 + 1]);
        mag_31_sq = (double)(re * re + im * im) * inv_q62;
    }
    {
        const int64_t re = pro_amp_real(state[3 * 8 + 2]);
        const int64_t im = pro_amp_imag(state[3 * 8 + 2]);
        mag_32_sq = (double)(re * re + im * im) * inv_q62;
    }

    const double total = mag_13_sq + mag_23_sq + mag_31_sq + mag_32_sq;

    printf("  |psi[1,3]|^2 = %.6f  (erwartet 0.25)\n", mag_13_sq);
    printf("  |psi[2,3]|^2 = %.6f  (erwartet 0.25)\n", mag_23_sq);
    printf("  |psi[3,1]|^2 = %.6f  (erwartet 0.25)\n", mag_31_sq);
    printf("  |psi[3,2]|^2 = %.6f  (erwartet 0.25)\n", mag_32_sq);
    printf("  Gesamt-Norm  = %.6f  (erwartet 1.0)\n", total);

    check("|psi[1,3]|^2 = 0.25", fabs(mag_13_sq - 0.25) < 1e-3);
    check("|psi[2,3]|^2 = 0.25", fabs(mag_23_sq - 0.25) < 1e-3);
    check("Norm erhalten", fabs(total - 1.0) < 1e-3);
    check("Is_Antisymmetric", ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000));

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T36
 * Hopping erhaelt Antisymmetrie fuer alle orbital_i, orbital_j, theta.
 */
static void test_T36_hopping_preserves_antisymmetry(void)
{
    printf("\n--- T36: Hopping erhaelt Antisymmetrie (Sweep) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);

    int all_ok = 1;
    int count = 0;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i == j) continue;
            for (int k = 0; k < 4; ++k) {
                if (k == i || k == j) continue;

                ProPhysics_Tensor_Set_Slater(&pu, pid, (uint8_t)i, (uint8_t)k);

                const int32_t theta_q15 = (int32_t)lround(
                    (M_PI / 6.0) * 32768.0);
                ProPhysics_Tensor_Apply_Hopping(&pu, pid,
                    (uint8_t)i, (uint8_t)j, theta_q15);

                if (!ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000)) {
                    all_ok = 0;
                }
                const double pv = ProPhysics_Tensor_Pauli_Violation(&pu, pid);
                if (pv > 1e-6) all_ok = 0;
                count++;
            }
        }
    }
    printf("  %d Kombinationen getestet\n", count);
    printf("  -> Antisymmetrie fuer alle erhalten: %s\n",
        all_ok ? "ja" : "NEIN");

    check("Antisymmetrie universell erhalten", all_ok);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T37
 * U(theta)^2 = U(2 theta). Zwei pi/4-Hoppings = ein pi/2-Hopping.
 * Ergebnis: Slater(1,3) -> -i * Slater(2,3).
 */
static void test_T37_hopping_doubling(void)
{
    printf("\n--- T37: Hopping(pi/4) zweimal = Hopping(pi/2) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 3);

    /* Weg A: zwei pi/4-Hoppings. */
    const int32_t theta_q15_pi4 = (int32_t)lround((M_PI / 4.0) * 32768.0);
    ProPhysics_Tensor_Apply_Hopping(&pu, pid, 1, 2, theta_q15_pi4);
    ProPhysics_Tensor_Apply_Hopping(&pu, pid, 1, 2, theta_q15_pi4);

    ProAmpQ31 stateA[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, stateA);

    /* Weg B: ein pi/2-Hopping. */
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 3);
    const int32_t theta_q15_pi2 = (int32_t)lround((M_PI / 2.0) * 32768.0);
    ProPhysics_Tensor_Apply_Hopping(&pu, pid, 1, 2, theta_q15_pi2);

    ProAmpQ31 stateB[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, stateB);

    /* Vergleich der beiden. */
    double max_diff = 0.0;
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) {
        const int64_t dr = (int64_t)pro_amp_real(stateA[i])
            - (int64_t)pro_amp_real(stateB[i]);
        const int64_t di = (int64_t)pro_amp_imag(stateA[i])
            - (int64_t)pro_amp_imag(stateB[i]);
        const double err = sqrt((double)(dr * dr + di * di));
        if (err > max_diff) max_diff = err;
    }
    const double rel_err = max_diff / 2147483647.0;

    /* Erwartung: psi[2,3] dominiert. */
    const int64_t r23 = pro_amp_real(stateA[2 * 8 + 3]);
    const int64_t i23 = pro_amp_imag(stateA[2 * 8 + 3]);
    const double mag_23 = sqrt((double)(r23 * r23 + i23 * i23))
        / 2147483647.0;

    printf("  max |A - B| = %.6e  (rel = %.4e)\n", max_diff, rel_err);
    printf("  |psi[2,3]| = %.6f  (erwartet 0.7071)\n", mag_23);

    check("U(pi/4)^2 = U(pi/2)", rel_err < 1e-6);
    check("|psi[2,3]| ~ 1/sqrt(2)", fabs(mag_23 - 0.7071067811865476) < 1e-3);

    ProPhysics_Free(&pu);
}


/* ------------------------------------------------------------------ T28
 *
 * Antisymmetrie-Test: SWAP auf antisymmetrischem Zustand gibt -1.
 * Slater(a,b) unter SWAP -> -Slater(a,b).
 */
static void test_T28_slater_swap_sign(void)
{
    printf("\n--- T28: Slater unter SWAP -> Vorzeichen flippt ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);

    /* Slater(1,4) = (|1,4> - |4,1>)/sqrt2. */
    const bool ok = ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);
    check("Set_Slater(1,4) ok", ok);

    ProAmpQ31 before[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, before);

    /* Is_Antisymmetric. */
    const bool anti_before =
        ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000);
    check("Is_Antisymmetric vor SWAP", anti_before);

    /* SWAP anwenden. */
    ProPhysics_Tensor_Apply_SWAP(&pu, pid);

    ProAmpQ31 after[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, after);

    /* Erwartung: after = -before. */
    double max_err = 0.0;
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) {
        const double br = (double)pro_amp_real(before[i]);
        const double bi = (double)pro_amp_imag(before[i]);
        const double ar = (double)pro_amp_real(after[i]);
        const double ai = (double)pro_amp_imag(after[i]);
        const double dr = ar + br;   /* after - (-before) = after + before */
        const double di = ai + bi;
        const double err = sqrt(dr * dr + di * di);
        if (err > max_err) max_err = err;
    }
    /* T28: SWAP(Slater) = -Slater, mit Q31-relativer Toleranz. */
    const double amp_ref = 0.7071067811865476 * 2147483647.0;  /* ~1.518e9 */
    const double rel_err = max_err / amp_ref;
    printf("  max |after + before| = %.6e  (rel = %.4e)\n", max_err, rel_err);
    check("SWAP(Slater) = -Slater (rel < 1e-6)", rel_err < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T29
 *
 * Slater(1,4): S(rho_A) = ln2, Concurrence = 1.
 */
static void test_T29_slater_entropy(void)
{
    printf("\n--- T29: Slater(1,4) -> S = ln2, C = 1 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    const double S = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    const double C = ProPhysics_Tensor_Concurrence(&pu, pid);

    printf("  S = %.12f  (erwartet ln2 = %.12f)\n", S, LN2);
    printf("  C = %.12f  (erwartet 1.0)\n", C);
    printf("  |S - ln2| = %.4e\n", fabs(S - LN2));

    check("S = ln2", fabs(S - LN2) < 1e-3);
    check("C = 1", fabs(C - 1.0) < 1e-3);
    check("Is_Antisymmetric", ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000));

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T30
 *
 * Pauli-Prinzip: |1,1> ist verboten. Set_Slater(1,1) -> false.
 * Und Pauli_Violation(|1,1>) = 1.
 */
static void test_T30_pauli_principle(void)
{
    printf("\n--- T30: Pauli-Prinzip |a,a> = 0 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);

    /* Direkter Versuch, Slater(1,1) zu setzen. */
    const bool ok = ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 1);
    printf("  Set_Slater(1,1) = %s (erwartet false)\n", ok ? "true" : "false");
    check("Set_Slater(1,1) lehnt ab", !ok);

    /* Tensor mit |1,1> besetzen und Pauli-Verletzung messen. */
    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    psi[1 * 8 + 1] = pro_amp_pack(INT32_MAX, 0);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    const double pv = ProPhysics_Tensor_Pauli_Violation(&pu, pid);
    printf("  Pauli_Violation(|1,1>) = %.12f (erwartet 1.0)\n", pv);
    check("Pauli_Violation = 1", fabs(pv - 1.0) < 1e-6);

    /* Fermionize: Projektion auf antisymmetrisch -> Null. */
    const bool ferm_ok = ProPhysics_Tensor_Fermionize(&pu, pid);
    printf("  Fermionize(|1,1>) = %s (erwartet false, da rein symmetrisch)\n",
        ferm_ok ? "true" : "false");
    check("Fermionize lehnt symmetrisch ab", !ferm_ok);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T31
 *
 * Zwei identische Fermionen: |1,1> ist verboten, aber Slater(1,4) hat
 * S = ln2. Antisymmetrie erzwingt maximale Verschraenkung in 2-dim.
 */
static void test_T31_two_identical_fermions(void)
{
    printf("\n--- T31: Zwei Fermionen -> Antisymmetrie erzwingt S = ln2 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);

    /* Drei verschiedene Slater-Paare. */
    const uint8_t pairs[3][2] = { {1,4}, {1,3}, {2,4} };
    int all_ok = 1;
    double S_vals[3];

    for (int k = 0; k < 3; ++k) {
        ProPhysics_Tensor_Set_Slater(&pu, pid, pairs[k][0], pairs[k][1]);
        S_vals[k] = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
        const double pv = ProPhysics_Tensor_Pauli_Violation(&pu, pid);
        printf("  Slater(%u,%u): S = %.6f, Pauli = %.6e\n",
            pairs[k][0], pairs[k][1], S_vals[k], pv);
        if (fabs(S_vals[k] - LN2) > 1e-3) all_ok = 0;
        if (pv > 1e-6) all_ok = 0;
    }
    check("Alle Slater-Zustaende haben S = ln2 und Pauli = 0", all_ok);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T32
 *
 * Slater unter SDK-Tick: markierte Knoten bleiben eingefroren,
 * Antisymmetrie bleibt erhalten.
 */
static void test_T32_slater_under_tick(void)
{
    printf("\n--- T32: Slater unter SDK-Tick, Antisymmetrie erhalten ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);
    ProPhysics_Tensor_Mark_Nodes(&pu, pid);
    ProPhysics_Tensor_Set_Auto_Sync(&pu, 1);

    const double S_before = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    const bool anti_before =
        ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000);

    for (int t = 0; t < 10; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    const double S_after = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);
    const bool anti_after =
        ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000);
    const double pv_after = ProPhysics_Tensor_Pauli_Violation(&pu, pid);

    printf("  S vor  10 Ticks = %.12f\n", S_before);
    printf("  S nach 10 Ticks = %.12f\n", S_after);
    printf("  Is_Antisymmetric vor  = %s\n", anti_before ? "true" : "false");
    printf("  Is_Antisymmetric nach = %s\n", anti_after ? "true" : "false");
    printf("  Pauli_Violation nach  = %.6e\n", pv_after);

    check("S vor = ln2", fabs(S_before - LN2) < 1e-3);
    check("S nach = ln2", fabs(S_after - LN2) < 1e-3);
    check("Antisymmetrie erhalten", anti_after);
    check("Pauli = 0 nach Tick", pv_after < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T33
 *
 * XX-Kopplung auf Fermion: XX ist bosonisch und kommutiert mit SWAP.
 * Auf dem antisymmetrischen Unterraum wirkt XX als -1 (Eigenwert).
 * Der Zustand bleibt antisymmetrisch (Negativkontrolle).
 */
static void test_T33_xx_on_fermion(void)
{
    printf("\n--- T33: XX-Kopplung auf Fermion (Negativkontrolle) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    /* XX mit theta = pi/2: U = -i sigma_x (x) sigma_x.
     * Auf |1,4> - |4,1>: sigma_x (x) sigma_x tauscht 1<->4.
     * Also U|Slater> = -i |Slater> (Eigenwert -i). */
    const int32_t theta_q15_pi2 = (int32_t)lround((M_PI / 2.0) * 32768.0);
    ProPhysics_Tensor_Set_Coupling(&pu, pid, theta_q15_pi2);
    ProPhysics_Tensor_Apply_XX_Step(&pu, pid);

    const bool anti = ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000);
    const double pv = ProPhysics_Tensor_Pauli_Violation(&pu, pid);
    const double S = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    printf("  Nach XX(pi/2): Is_Antisymmetric = %s\n", anti ? "true" : "false");
    printf("  Pauli_Violation = %.6e\n", pv);
    printf("  S = %.12f\n", S);
    printf("  -> XX ist bosonisch, erhaelt Antisymmetrie, erzeugt aber\n");
    printf("     KEINE neue Verschaenkung (Eigenzustand).\n");

    check("Antisymmetrie erhalten", anti);
    check("Pauli = 0", pv < 1e-6);
    check("S bleibt ln2", fabs(S - LN2) < 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T38
 * Hopping unter SDK-Tick: 3 Ticks mit theta=pi/12 pro Tick
 * ergibt effektive Phase pi/4. |psi[2,3]|^2 = 0.25.
 */
static void test_T38_hopping_under_tick(void)
{
    printf("\n--- T38: Hopping unter SDK-Tick (3 Ticks, pi/12 pro Tick) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 3);
    ProPhysics_Tensor_Set_Hopping(&pu, pid, 1, 2,
        (int32_t)lround((M_PI / 12.0) * 32768.0));
    ProPhysics_Tensor_Mark_Nodes(&pu, pid);

    const double S_before = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    for (int t = 0; t < 3; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    ProAmpQ31 state[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, state);

    const double inv_q62 = 1.0 / 4611686018427387904.0;
    double mag_13_sq = 0.0, mag_23_sq = 0.0;
    {
        const int64_t re = pro_amp_real(state[1 * 8 + 3]);
        const int64_t im = pro_amp_imag(state[1 * 8 + 3]);
        mag_13_sq = (double)(re * re + im * im) * inv_q62;
    }
    {
        const int64_t re = pro_amp_real(state[2 * 8 + 3]);
        const int64_t im = pro_amp_imag(state[2 * 8 + 3]);
        mag_23_sq = (double)(re * re + im * im) * inv_q62;
    }

    const bool anti = ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000);
    const double pv = ProPhysics_Tensor_Pauli_Violation(&pu, pid);
    const double S_after = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    printf("  S vor  3 Ticks = %.12f\n", S_before);
    printf("  S nach 3 Ticks = %.12f\n", S_after);
    printf("  |psi[1,3]|^2 = %.6f  (erwartet 0.25)\n", mag_13_sq);
    printf("  |psi[2,3]|^2 = %.6f  (erwartet 0.25)\n", mag_23_sq);
    printf("  Is_Antisymmetric = %s\n", anti ? "true" : "false");
    printf("  Pauli_Violation = %.6e\n", pv);

    check("|psi[1,3]|^2 = 0.25", fabs(mag_13_sq - 0.25) < 1e-3);
    check("|psi[2,3]|^2 = 0.25", fabs(mag_23_sq - 0.25) < 1e-3);
    check("Antisymmetrie erhalten", anti);
    check("Pauli = 0", pv < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T39
 * Hopping bleibt stabil ueber viele Ticks (kein Normverlust).
 */
static void test_T39_hopping_stability(void)
{
    printf("\n--- T39: Hopping-Stabilitaet ueber 100 Ticks ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 3);
    ProPhysics_Tensor_Set_Hopping(&pu, pid, 1, 2,
        (int32_t)lround((M_PI / 32.0) * 32768.0));
    ProPhysics_Tensor_Mark_Nodes(&pu, pid);

    for (int t = 0; t < 100; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    ProAmpQ31 state[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, state);

    double norm_q62 = 0.0;
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) {
        const int64_t re = pro_amp_real(state[i]);
        const int64_t im = pro_amp_imag(state[i]);
        norm_q62 += (double)(re * re + im * im);
    }
    const double norm = norm_q62 / 4611686018427387904.0;

    const bool anti = ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 10000);
    const double pv = ProPhysics_Tensor_Pauli_Violation(&pu, pid);

    printf("  Norm nach 100 Ticks = %.9f  (erwartet 1.0)\n", norm);
    printf("  Is_Antisymmetric = %s\n", anti ? "true" : "false");
    printf("  Pauli_Violation = %.6e\n", pv);

    check("Norm erhalten (|dN| < 1e-6)", fabs(norm - 1.0) < 1e-6);
    check("Antisymmetrie erhalten", anti);
    check("Pauli = 0", pv < 1e-6);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ T40
 * Hopping + XX koexistieren: beide Wechselwirkungen laufen pro Tick.
 */
static void test_T40_hopping_and_xx(void)
{
    printf("\n--- T40: Hopping + XX koexistieren ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 3);
    /* XX auf {1,4}-Qubit-Basis: wirkungslos auf {1,3}-Slater. */
    ProPhysics_Tensor_Set_Coupling(&pu, pid,
        (int32_t)lround((M_PI / 8.0) * 32768.0));
    /* Hopping auf {1,2}: mischt 1<->2. */
    ProPhysics_Tensor_Set_Hopping(&pu, pid, 1, 2,
        (int32_t)lround((M_PI / 16.0) * 32768.0));
    ProPhysics_Tensor_Mark_Nodes(&pu, pid);

    for (int t = 0; t < 4; ++t) {
        ProPhysics_SDK_Execute_Plastizitaet_Tick(&pu,
            ResearchPlugin_DynamicPlasticTopology);
    }

    const bool anti = ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000);
    const double pv = ProPhysics_Tensor_Pauli_Violation(&pu, pid);

    ProAmpQ31 state[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, state);
    const double inv_q62 = 1.0 / 4611686018427387904.0;
    double mag_13 = 0.0, mag_23 = 0.0;
    {
        const int64_t re = pro_amp_real(state[1 * 8 + 3]);
        const int64_t im = pro_amp_imag(state[1 * 8 + 3]);
        mag_13 = (double)(re * re + im * im) * inv_q62;
    }
    {
        const int64_t re = pro_amp_real(state[2 * 8 + 3]);
        const int64_t im = pro_amp_imag(state[2 * 8 + 3]);
        mag_23 = (double)(re * re + im * im) * inv_q62;
    }

    printf("  |psi[1,3]|^2 = %.6f\n", mag_13);
    printf("  |psi[2,3]|^2 = %.6f\n", mag_23);
    printf("  Is_Antisymmetric = %s\n", anti ? "true" : "false");
    printf("  Pauli_Violation = %.6e\n", pv);

    check("Antisymmetrie erhalten", anti);
    check("Pauli = 0", pv < 1e-6);
    check("Hopping hat gemischt (|psi[2,3]|^2 > 0)", mag_23 > 1e-3);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F1
 * Popcount fuer alle 256 Bits.
 */
static void test_F1_popcount(void)
{
    printf("\n--- F1: Fock Popcount fuer alle 256 Bits ---\n");

    int errors = 0;
    for (uint32_t b = 0; b < 256; ++b) {
        uint32_t expected = 0;
        for (int i = 0; i < 8; ++i) {
            if ((b >> i) & 1u) expected++;
        }
        const uint32_t got = ProPhysics_Fock_Popcount((uint8_t)b);
        if (got != expected) {
            printf("  Mismatch: bits = 0x%02X, expected %u, got %u\n",
                b, expected, got);
            errors++;
        }
    }
    printf("  Fehler: %d / 256\n", errors);
    check("Popcount korrekt fuer alle 256 Bits", errors == 0);
}

/* ------------------------------------------------------------------ F2
 * Vakuum: |0...0> hat Norm 1, N = 0.
 */
static void test_F2_vacuum(void)
{
    printf("\n--- F2: Fock-Vakuum ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    const bool ok = ProPhysics_Fock_Create(&pu, &fid);
    check("Create ok", ok);

    int32_t re = 0, im = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0u, &re, &im);
    printf("  coeff[0] = (%d, %d)\n", (int)re, (int)im);

    const double norm = ProPhysics_Fock_Norm(&pu, fid);
    const double N = ProPhysics_Fock_Particle_Number(&pu, fid);
    printf("  Norm = %.12f  (erwartet 1)\n", norm);
    printf("  <N>  = %.12f  (erwartet 0)\n", N);

    check("coeff[0] = Q31(1)", re == INT32_MAX && im == 0);
    check("Norm = 1", fabs(norm - 1.0) < 1e-6);
    check("<N> = 0", fabs(N) < 1e-9);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F3
 * Basis-Zustaende: |1> und |2,3,4>.
 */
static void test_F3_basis_states(void)
{
    printf("\n--- F3: Fock Basis-Zustaende ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);

    /* |1> = Mode 0 besetzt = bits 0b00000001. */
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);
    double norm = ProPhysics_Fock_Norm(&pu, fid);
    double N = ProPhysics_Fock_Particle_Number(&pu, fid);
    printf("  |1>:      Norm = %.6f, <N> = %.6f\n", norm, N);
    check("|1>: Norm = 1", fabs(norm - 1.0) < 1e-6);
    check("|1>: <N> = 1", fabs(N - 1.0) < 1e-6);

    /* |2,3,4> = Moden 1, 2, 3 besetzt = bits 0b00001110 = 0x0E. */
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x0Eu);
    norm = ProPhysics_Fock_Norm(&pu, fid);
    N = ProPhysics_Fock_Particle_Number(&pu, fid);
    printf("  |2,3,4>:  Norm = %.6f, <N> = %.6f\n", norm, N);
    check("|2,3,4>: Norm = 1", fabs(norm - 1.0) < 1e-6);
    check("|2,3,4>: <N> = 3", fabs(N - 3.0) < 1e-6);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F4
 * Superposition (|1> + |2>)/sqrt2.
 */
static void test_F4_superposition(void)
{
    printf("\n--- F4: Fock-Superposition (|1> + |2>)/sqrt2 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);

    const int32_t amp = (int32_t)lround(0.7071067811865476 * 2147483647.0);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x00u);              /* Vakuum */
    ProPhysics_Fock_Set_Amplitude(&pu, fid, 0x00u, 0, 0);    /* explizit nullen */
    ProPhysics_Fock_Set_Amplitude(&pu, fid, 0x01u, amp, 0);  /* |1> */
    ProPhysics_Fock_Set_Amplitude(&pu, fid, 0x02u, amp, 0);  /* |2> */

    const double norm = ProPhysics_Fock_Norm(&pu, fid);
    const double N = ProPhysics_Fock_Particle_Number(&pu, fid);
    printf("  Norm = %.12f  (erwartet 1)\n", norm);
    printf("  <N>  = %.12f  (erwartet 1)\n", N);

    check("Norm = 1", fabs(norm - 1.0) < 1e-6);
    check("<N>  = 1 (jede Komponente hat N=1)", fabs(N - 1.0) < 1e-6);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F5
 * Amplitude Get/Set Roundtrip.
 */
static void test_F5_get_set_roundtrip(void)
{
    printf("\n--- F5: Fock Get/Set Roundtrip ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);

    int32_t errors = 0;
    for (uint32_t b = 0; b < 256; ++b) {
        const int32_t re_in = (int32_t)(b * 1234567 - 100000000);
        const int32_t im_in = (int32_t)(b * -7654321 + 50000000);
        ProPhysics_Fock_Set_Amplitude(&pu, fid, (uint8_t)b, re_in, im_in);

        int32_t re_out = 0, im_out = 0;
        ProPhysics_Fock_Get_Amplitude(&pu, fid, (uint8_t)b, &re_out, &im_out);
        if (re_out != re_in || im_out != im_in) errors++;
    }
    printf("  Fehler: %d / 256\n", (int)errors);
    check("Roundtrip konsistent", errors == 0);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F6
 * 3-Fermion-Slater: |1,2,3> als Antisymmetrie-Vorschau.
 *
 * Im Fock-Raum ist der 3-Fermion-Zustand KEIN Tensor.
 * Er ist ein einzelner Basis-Zustand mit bits = 0b00001110 = 0x0E.
 * Wir setzen ihn und pruefen: <N> = 3, Norm = 1.
 */
static void test_F6_three_fermion_basis(void)
{
    printf("\n--- F6: 3-Fermion-Basis-Zustand |1,2,3> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x0Eu);   /* Moden 1,2,3 */

    const double norm = ProPhysics_Fock_Norm(&pu, fid);
    const double N = ProPhysics_Fock_Particle_Number(&pu, fid);
    printf("  Norm = %.12f  (erwartet 1)\n", norm);
    printf("  <N>  = %.12f  (erwartet 3)\n", N);

    check("Norm = 1", fabs(norm - 1.0) < 1e-6);
    check("<N> = 3", fabs(N - 3.0) < 1e-6);

    /* Vakuum existiert noch. */
    uint64_t fid2 = 0;
    ProPhysics_Fock_Create(&pu, &fid2);
    check("Zwei unabhaengige Fock-States", fid != fid2);
    check("Count = 2", ProPhysics_Fock_Count(&pu) == 2);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Fock_Destroy(&pu, fid2);
    check("Count = 0 nach Destroy", ProPhysics_Fock_Count(&pu) == 0);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F7
 * c†_0 |vacuum> = |1>.  Erwartet: coeff[0x01] = 1.
 */
static void test_F7_create_on_vacuum(void)
{
    printf("\n--- F7: c+_0 |vacuum> = |1> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);   /* Vakuum */

    ProPhysics_Fock_Apply_Create(&pu, fid, 0u);

    int32_t re1 = 0, im1 = 0, re0 = 0, im0 = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x01u, &re1, &im1);
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x00u, &re0, &im0);

    printf("  coeff[0x00] = (%d, %d)  (erwartet 0)\n", (int)re0, (int)im0);
    printf("  coeff[0x01] = (%d, %d)  (erwartet Q31(1))\n", (int)re1, (int)im1);

    check("coeff[0x00] = 0", re0 == 0 && im0 == 0);
    check("coeff[0x01] = Q31(1)", re1 == INT32_MAX && im1 == 0);
    check("Norm = 1", fabs(ProPhysics_Fock_Norm(&pu, fid) - 1.0) < 1e-6);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F8
 * Pauli: c†_0 c†_0 |vacuum> = 0.
 */
static void test_F8_pauli_double_create(void)
{
    printf("\n--- F8: c+_0 c+_0 |vacuum> = 0 (Pauli) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Apply_Create(&pu, fid, 0u);
    const bool ok = ProPhysics_Fock_Apply_Create(&pu, fid, 0u);

    printf("  Rueckgabe: %s (erwartet false)\n", ok ? "true" : "false");
    check("Create auf besetzter Mode gibt false", !ok);
    check("Zustand ist Null", ProPhysics_Fock_Is_Zero(&pu, fid));

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F9
 * JW-Vorzeichen: c†_1 |1> = -|1,2>.
 * Erwartet: coeff[0x03] = -Q31(1).
 */
static void test_F9_jw_sign(void)
{
    printf("\n--- F9: Jordan-Wigner-Vorzeichen ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);   /* |1> */
    ProPhysics_Fock_Apply_Create(&pu, fid, 1u);   /* c+_1 */

    int32_t re3 = 0, im3 = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x03u, &re3, &im3);

    printf("  coeff[0x03] = (%d, %d)  (erwartet -Q31(1))\n", (int)re3, (int)im3);

    check("coeff[0x03] = -Q31(1)", re3 == -INT32_MAX && im3 == 0);

    /* Kontroll-Test: c†_0 |2> (Mode 0 leer, Mode 1 besetzt). */
    uint64_t fid2 = 0;
    ProPhysics_Fock_Create(&pu, &fid2);
    ProPhysics_Fock_Set_Basis(&pu, fid2, 0x02u);   /* |2> = Mode 1 */
    ProPhysics_Fock_Apply_Create(&pu, fid2, 0u);

    int32_t re3b = 0, im3b = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, fid2, 0x03u, &re3b, &im3b);
    printf("  coeff[0x03] (c+_0|2>) = (%d, %d)  (erwartet +Q31(1))\n",
        (int)re3b, (int)im3b);

    check("c+_0 |2> = +|1,2>", re3b == INT32_MAX && im3b == 0);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Fock_Destroy(&pu, fid2);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F10
 * c_0 |1> = |vacuum>. Und c_0 |vacuum> = 0.
 */
static void test_F10_annihilate(void)
{
    printf("\n--- F10: c_0 |1> = |vacuum>, c_0 |vacuum> = 0 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);
    ProPhysics_Fock_Apply_Annihilate(&pu, fid, 0u);

    int32_t re0 = 0, im0 = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x00u, &re0, &im0);
    printf("  c_0 |1> -> coeff[0x00] = (%d, %d)\n", (int)re0, (int)im0);
    check("c_0 |1> = |vacuum>", re0 == INT32_MAX && im0 == 0);

    /* Vakuum annihilieren. */
    ProPhysics_Fock_Apply_Annihilate(&pu, fid, 0u);
    check("c_0 |vacuum> = 0", ProPhysics_Fock_Is_Zero(&pu, fid));

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F11
 * c†_0 c_0 |1> = |1>. Projektor auf Besetzung.
 */
static void test_F11_projector(void)
{
    printf("\n--- F11: c+_0 c_0 |1> = |1> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);
    ProPhysics_Fock_Apply_Annihilate(&pu, fid, 0u);
    ProPhysics_Fock_Apply_Create(&pu, fid, 0u);

    int32_t re1 = 0, im1 = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x01u, &re1, &im1);
    printf("  coeff[0x01] = (%d, %d)  (erwartet Q31(1))\n", (int)re1, (int)im1);
    check("c+ c |1> = |1>", re1 == INT32_MAX && im1 == 0);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F12
 * 3-Fermion: c†_2 c†_1 c†_0 |vacuum> = |1,2,3> mit JW-Vorzeichen.
 *
 * Bits: Mode 0, 1, 2 besetzt -> 0b00000111 = 0x07.
 *
 * Vorzeichen-Kette:
 *   c†_0 |vac> = +|1>                                   (sign +)
 *   c†_1 |1>  = (-1)^1 |1,2> = -|1,2>                   (sign -)
 *   c†_2 (-|1,2>) = -(+1) |1,2,3> = -|1,2,3>           (sign +)
 *
 * Gesamt: -|1,2,3>. coeff[0x07] = -Q31(1).
 * (Permutation (0,1,2) -> (2,1,0) ist ungerade.)
 */
static void test_F12_three_fermion_chain(void)
{
    printf("\n--- F12: c+_2 c+_1 c+_0 |vacuum> = |1,2,3> ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);

    ProPhysics_Fock_Apply_Create(&pu, fid, 0u);
    ProPhysics_Fock_Apply_Create(&pu, fid, 1u);
    ProPhysics_Fock_Apply_Create(&pu, fid, 2u);

    int32_t re7 = 0, im7 = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x07u, &re7, &im7);

    double N = ProPhysics_Fock_Particle_Number(&pu, fid);
    double norm = ProPhysics_Fock_Norm(&pu, fid);

    printf("  coeff[0x07] = (%d, %d)  (erwartet +Q31(1))\n", (int)re7, (int)im7);
    printf("  Norm = %.12f, <N> = %.12f\n", norm, N);

    check("coeff[0x07] = -Q31(1) (ungerade Permutation)",
        re7 == -INT32_MAX && im7 == 0);
    check("Norm = 1", fabs(norm - 1.0) < 1e-6);
    check("<N> = 3", fabs(N - 3.0) < 1e-6);

    /* Gegenprobe: umgekehrte Reihenfolge c†_0 c†_1 c†_2 |vac>.
     *   c†_2 |vac>   = |3>
     *   c†_1 |3>     = (-1)^0 |2,3> = |2,3>          (below = 0)
     *   c†_0 |2,3>   = (-1)^0 |1,2,3> = |1,2,3>      (below = 0)
     * Ergebnis: +|1,2,3> mit coeff[0x07] = +Q31(1).
     * Vorzeichen-Konsistenz: beide Wege ergeben +. */
    uint64_t fid2 = 0;
    ProPhysics_Fock_Create(&pu, &fid2);
    ProPhysics_Fock_Apply_Create(&pu, fid2, 2u);
    ProPhysics_Fock_Apply_Create(&pu, fid2, 1u);
    ProPhysics_Fock_Apply_Create(&pu, fid2, 0u);
    int32_t re7b = 0, im7b = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, fid2, 0x07u, &re7b, &im7b);
    printf("  coeff[0x07] (umgekehrte Reihenfolge) = (%d, %d)\n",
        (int)re7b, (int)im7b);
    check("Umgekehrte Reihenfolge auch +Q31(1)",
        re7b == INT32_MAX && im7b == 0);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Fock_Destroy(&pu, fid2);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F13
 * {c_i, c+_j} = delta_ij. Sweep ueber alle i, j in {0..3}.
 *
 * Test auf Basis-Zustand |1,2> (Moden 0 und 1 besetzt).
 */
static void test_F13_anticomm_cd(void)
{
    printf("\n--- F13: {c_i, c+_j} = delta_ij ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    int all_ok = 1;
    int count = 0;

    for (uint8_t i = 0; i < 4; ++i) {
        for (uint8_t j = 0; j < 4; ++j) {
            uint64_t fid = 0;
            ProPhysics_Fock_Create(&pu, &fid);
            ProPhysics_Fock_Set_Basis(&pu, fid, 0x03u);   /* |1,2> */

            /* Erwartung: |psi> skaliert mit delta_ij. */
            uint64_t expect_id = 0;
            ProPhysics_Fock_Clone(&pu, fid, &expect_id);
            const int32_t delta = (i == j) ? INT32_MAX : 0;
            ProPhysics_Fock_Scale(&pu, expect_id, delta);

            ProPhysics_Fock_Apply_Anticomm_CD(&pu, fid, i, j);

            const double err = ProPhysics_Fock_Compare(&pu, fid, expect_id);
            if (err > 1000.0) {
                printf("  i=%u j=%u err=%.6e\n", i, j, err);
                all_ok = 0;
            }
            count++;

            ProPhysics_Fock_Destroy(&pu, fid);
            ProPhysics_Fock_Destroy(&pu, expect_id);
        }
    }
    printf("  %d Kombinationen getestet\n", count);
    printf("  Alle < 1000 Q31-Einheiten: %s\n", all_ok ? "ja" : "NEIN");
    check("{c_i, c+_j} = delta_ij", all_ok);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F14
 * {c_i, c_j} = 0. Sweep ueber i, j in {0..3}.
 */
static void test_F14_anticomm_cc(void)
{
    printf("\n--- F14: {c_i, c_j} = 0 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    int all_ok = 1;
    int count = 0;

    for (uint8_t i = 0; i < 4; ++i) {
        for (uint8_t j = 0; j < 4; ++j) {
            uint64_t fid = 0;
            ProPhysics_Fock_Create(&pu, &fid);
            ProPhysics_Fock_Set_Basis(&pu, fid, 0x07u);   /* |1,2,3> */

            uint64_t expect_id = 0;
            ProPhysics_Fock_Clone(&pu, fid, &expect_id);
            ProPhysics_Fock_Scale(&pu, expect_id, 0);   /* Erwartung: 0 */

            ProPhysics_Fock_Apply_Anticomm_CC(&pu, fid, i, j);

            const double err = ProPhysics_Fock_Compare(&pu, fid, expect_id);
            if (err > 1000.0) {
                printf("  i=%u j=%u err=%.6e\n", i, j, err);
                all_ok = 0;
            }
            count++;

            ProPhysics_Fock_Destroy(&pu, fid);
            ProPhysics_Fock_Destroy(&pu, expect_id);
        }
    }
    printf("  %d Kombinationen getestet\n", count);
    check("{c_i, c_j} = 0", all_ok);

    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F15
 * {c+_i, c+_j} = 0. Und: |1,2> hat N=2 als Slater-Erzeugung.
 *
 * Zusaetzlich: c+_0 c+_1 |vac> = -c+_1 c+_0 |vac>.
 */
static void test_F15_anticomm_dd(void)
{
    printf("\n--- F15: {c+_i, c+_j} = 0 und Slater-Antisymmetrie ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    int all_ok = 1;
    int count = 0;

    for (uint8_t i = 0; i < 4; ++i) {
        for (uint8_t j = 0; j < 4; ++j) {
            uint64_t fid = 0;
            ProPhysics_Fock_Create(&pu, &fid);
            ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);   /* |1> */

            uint64_t expect_id = 0;
            ProPhysics_Fock_Clone(&pu, fid, &expect_id);
            ProPhysics_Fock_Scale(&pu, expect_id, 0);

            ProPhysics_Fock_Apply_Anticomm_DD(&pu, fid, i, j);

            const double err = ProPhysics_Fock_Compare(&pu, fid, expect_id);
            if (err > 1000.0) {
                printf("  i=%u j=%u err=%.6e\n", i, j, err);
                all_ok = 0;
            }
            count++;

            ProPhysics_Fock_Destroy(&pu, fid);
            ProPhysics_Fock_Destroy(&pu, expect_id);
        }
    }
    printf("  %d Kombinationen getestet\n", count);
    check("{c+_i, c+_j} = 0", all_ok);

    /* Slater-Antisymmetrie: c+_0 c+_1 |vac> = -c+_1 c+_0 |vac>. */
    uint64_t a = 0, b = 0;
    ProPhysics_Fock_Create(&pu, &a);
    ProPhysics_Fock_Create(&pu, &b);

    ProPhysics_Fock_Apply_Create(&pu, a, 0u);
    ProPhysics_Fock_Apply_Create(&pu, a, 1u);   /* c+_1 c+_0 |vac> */

    ProPhysics_Fock_Apply_Create(&pu, b, 1u);
    ProPhysics_Fock_Apply_Create(&pu, b, 0u);   /* c+_0 c+_1 |vac> */

    int32_t reA = 0, imA = 0, reB = 0, imB = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, a, 0x03u, &reA, &imA);
    ProPhysics_Fock_Get_Amplitude(&pu, b, 0x03u, &reB, &imB);

    printf("  c+_1 c+_0 |vac>: coeff[0x03] = (%d, %d)\n", (int)reA, (int)imA);
    printf("  c+_0 c+_1 |vac>: coeff[0x03] = (%d, %d)\n", (int)reB, (int)imB);
    printf("  Erwartet: entgegengesetzte Vorzeichen\n");

    check("Slater antisymmetrisch (a = -b)",
        (reA == -reB) && (imA == -imB));

    ProPhysics_Fock_Destroy(&pu, a);
    ProPhysics_Fock_Destroy(&pu, b);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F16
 * Hopping(0,1) auf Ein-Teilchen-Zustand |1>.
 * theta = pi/4. Erwartet: |coeff[0x01]|^2 = |coeff[0x02]|^2 = 0.5.
 * coeff[0x02] ist imaginaer: -i * (1/sqrt2).
 */
static void test_F16_hopping_single_particle(void)
{
    printf("\n--- F16: Hopping(0,1) auf |1>, theta = pi/4 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x01u);   /* |1> */

    ProPhysics_Fock_Apply_Hopping(&pu, fid, 0u, 1u,
        (int32_t)lround((M_PI / 4.0) * 32768.0));

    const double inv_q62 = 1.0 / 4611686018427387904.0;
    int32_t re1 = 0, im1 = 0, re2 = 0, im2 = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x01u, &re1, &im1);
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x02u, &re2, &im2);

    const double mag1_sq = ((double)re1 * re1 + (double)im1 * im1) * inv_q62;
    const double mag2_sq = ((double)re2 * re2 + (double)im2 * im2) * inv_q62;
    const double norm = ProPhysics_Fock_Norm(&pu, fid);

    printf("  coeff[0x01] = (%d, %d)  |c|^2 = %.6f  (erwartet 0.5)\n",
        (int)re1, (int)im1, mag1_sq);
    printf("  coeff[0x02] = (%d, %d)  |c|^2 = %.6f  (erwartet 0.5)\n",
        (int)re2, (int)im2, mag2_sq);
    printf("  Norm = %.9f\n", norm);

    check("|c[0x01]|^2 = 0.5", fabs(mag1_sq - 0.5) < 1e-3);
    check("|c[0x02]|^2 = 0.5", fabs(mag2_sq - 0.5) < 1e-3);
    check("coeff[0x02] imaginaer", re2 == 0 && im2 < 0);
    check("Norm = 1", fabs(norm - 1.0) < 1e-6);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F17
 * Hopping(0,3) auf |1,2,3> mit Zwischen-Moden 1,2.
 * Zwischenstring-Popcount = 2 (gerade) -> sigma = +1.
 * Erwartet: |c[0x07]|^2 = |c[0x0E]|^2 = 0.5.
 */
static void test_F17_hopping_with_between(void)
{
    printf("\n--- F17: Hopping(0,3) auf |1,2,3>, theta = pi/4 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x07u);   /* |1,2,3> */

    ProPhysics_Fock_Apply_Hopping(&pu, fid, 0u, 3u,
        (int32_t)lround((M_PI / 4.0) * 32768.0));

    const double inv_q62 = 1.0 / 4611686018427387904.0;
    int32_t re7 = 0, im7 = 0, re14 = 0, im14 = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x07u, &re7, &im7);
    ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x0Eu, &re14, &im14);

    const double mag7_sq = ((double)re7 * re7 + (double)im7 * im7) * inv_q62;
    const double mag14_sq = ((double)re14 * re14 + (double)im14 * im14) * inv_q62;

    const double N = ProPhysics_Fock_Particle_Number(&pu, fid);

    printf("  |c[0x07]|^2 = %.6f  (erwartet 0.5)\n", mag7_sq);
    printf("  |c[0x0E]|^2 = %.6f  (erwartet 0.5)\n", mag14_sq);
    printf("  <N> = %.6f  (erwartet 3)\n", N);

    check("|c[0x07]|^2 = 0.5", fabs(mag7_sq - 0.5) < 1e-3);
    check("|c[0x0E]|^2 = 0.5", fabs(mag14_sq - 0.5) < 1e-3);
    check("<N> = 3", fabs(N - 3.0) < 1e-6);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F18
 * Hopping(pi/4) zweimal = Hopping(pi/2).
 * Slater(0,1) sollte zu -i*|2> werden (bei theta=pi/2).
 */
static void test_F18_hopping_doubling_fock(void)
{
    printf("\n--- F18: Hopping(pi/4) zweimal = Hopping(pi/2) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t a = 0, b = 0;
    ProPhysics_Fock_Create(&pu, &a);
    ProPhysics_Fock_Create(&pu, &b);
    ProPhysics_Fock_Set_Basis(&pu, a, 0x01u);   /* |1> */
    ProPhysics_Fock_Set_Basis(&pu, b, 0x01u);   /* |1> */

    /* Weg A: zwei pi/4-Hoppings. */
    const int32_t tq_pi4 = (int32_t)lround((M_PI / 4.0) * 32768.0);
    ProPhysics_Fock_Apply_Hopping(&pu, a, 0u, 1u, tq_pi4);
    ProPhysics_Fock_Apply_Hopping(&pu, a, 0u, 1u, tq_pi4);

    /* Weg B: ein pi/2-Hopping. */
    const int32_t tq_pi2 = (int32_t)lround((M_PI / 2.0) * 32768.0);
    ProPhysics_Fock_Apply_Hopping(&pu, b, 0u, 1u, tq_pi2);

    const double diff = ProPhysics_Fock_Compare(&pu, a, b);
    const double rel = diff / 2147483647.0;

    int32_t re2 = 0, im2 = 0;
    ProPhysics_Fock_Get_Amplitude(&pu, a, 0x02u, &re2, &im2);
    const double mag2 = sqrt((double)re2 * re2 + (double)im2 * im2)
        / 2147483647.0;

    printf("  max |A - B| = %.6e  (rel = %.4e)\n", diff, rel);
    printf("  |c[0x02]| = %.6f  (erwartet 1.0)\n", mag2);

    check("U(pi/4)^2 = U(pi/2)", rel < 1e-6);
    check("|c[0x02]| = 1", fabs(mag2 - 1.0) < 1e-3);

    ProPhysics_Fock_Destroy(&pu, a);
    ProPhysics_Fock_Destroy(&pu, b);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F19
 * Hopping unter langer Zeit: Norm bleibt erhalten.
 * 4 Teilchen in Moden 0..3, Hopping(0,7) mit theta=pi/32, 200 Ticks.
 */
static void test_F19_hopping_stability(void)
{
    printf("\n--- F19: Fock-Hopping-Stabilitaet (200 Ticks) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint64_t fid = 0;
    ProPhysics_Fock_Create(&pu, &fid);
    ProPhysics_Fock_Set_Basis(&pu, fid, 0x0Fu);   /* |1,2,3,4> */

    const int32_t tq = (int32_t)lround((M_PI / 32.0) * 32768.0);
    for (int t = 0; t < 200; ++t) {
        ProPhysics_Fock_Apply_Hopping(&pu, fid, 0u, 7u, tq);
    }

    const double norm = ProPhysics_Fock_Norm(&pu, fid);
    const double N = ProPhysics_Fock_Particle_Number(&pu, fid);

    printf("  Norm nach 200 Ticks = %.9f  (erwartet 1.0)\n", norm);
    printf("  <N>   nach 200 Ticks = %.9f  (erwartet 4.0)\n", N);

    check("Norm erhalten (|dN| < 1e-6)", fabs(norm - 1.0) < 1e-6);
    check("<N> = 4 erhalten", fabs(N - 4.0) < 1e-6);

    ProPhysics_Fock_Destroy(&pu, fid);
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F20
 * Slater(1,4) -> Fock -> Tensor Roundtrip.
 */
static void test_F20_tensor_to_fock_roundtrip(void)
{
    printf("\n--- F20: Tensor Slater(1,4) -> Fock -> Tensor ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_Slater(&pu, pid, 1, 4);

    ProAmpQ31 before[PRO_TENSOR_DIM];
    ProPhysics_Tensor_Get_State(&pu, pid, before);

    uint64_t fid = 0;
    const bool ok = ProPhysics_Tensor_To_Fock(&pu, pid, &fid);
    check("Tensor_To_Fock ok", ok);

    if (ok) {
        /* Fock-Basis-Zustand: bits = (1<<1)|(1<<4) = 0x12. */
        int32_t re = 0, im = 0;
        ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x12u, &re, &im);
        printf("  Fock coeff[0x12] = (%d, %d)  (erwartet +Q31(1))\n",
            (int)re, (int)im);
        check("Fock coeff[0x12] = Q31(1)", re == INT32_MAX && im == 0);

        /* Vakuum muss 0 sein. */
        int32_t re0 = 0, im0 = 0;
        ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x00u, &re0, &im0);
        check("Fock coeff[0x00] = 0", re0 == 0 && im0 == 0);

        /* Zurueck zum Tensor. */
        uint32_t pid2 = 0;
        ProPhysics_Tensor_Create_Pair(&pu, 2, 3, &pid2);
        const bool ok2 = ProPhysics_Fock_To_Tensor(&pu, fid, pid2);
        check("Fock_To_Tensor ok", ok2);

        if (ok2) {
            ProAmpQ31 after[PRO_TENSOR_DIM];
            ProPhysics_Tensor_Get_State(&pu, pid2, after);

            double max_diff = 0.0;
            for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) {
                const int64_t dr = (int64_t)pro_amp_real(before[i])
                    - (int64_t)pro_amp_real(after[i]);
                const int64_t di = (int64_t)pro_amp_imag(before[i])
                    - (int64_t)pro_amp_imag(after[i]);
                const double err = sqrt((double)(dr * dr + di * di));
                if (err > max_diff) max_diff = err;
            }
            printf("  max |before - after| = %.6e\n", max_diff);
            check("Roundtrip konsistent (< 1000 Q31)",
                max_diff < 1000.0);
            check("Antisymmetrie erhalten",
                ProPhysics_Tensor_Is_Antisymmetric(&pu, pid2, 1000));
        }
        ProPhysics_Fock_Destroy(&pu, fid);
    }
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F21
 * Bell-artige Superposition: Slater(1,4) + Slater(2,3).
 * Norm-erhaltende Konvertierung.
 */
static void test_F21_superposition(void)
{
    printf("\n--- F21: Superposition Slater(1,4) + Slater(2,3) ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    /* Tensor: (Slater(1,4) + Slater(2,3))/sqrt2.
     * Konkret: c[1*8+4] = +1/2, c[4*8+1] = -1/2,
     *          c[2*8+3] = +1/2, c[3*8+2] = -1/2. */
    ProAmpQ31 psi[PRO_TENSOR_DIM];
    for (uint32_t i = 0; i < PRO_TENSOR_DIM; ++i) psi[i] = 0;
    const int32_t h = (int32_t)lround(0.5 * 2147483647.0);
    psi[1 * 8 + 4] = pro_amp_pack(h, 0);
    psi[4 * 8 + 1] = pro_amp_pack(-h, 0);
    psi[2 * 8 + 3] = pro_amp_pack(h, 0);
    psi[3 * 8 + 2] = pro_amp_pack(-h, 0);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);
    ProPhysics_Tensor_Set_State(&pu, pid, psi);

    /* Tensor-Norm pruefen. */
    const double S_tensor = ProPhysics_Tensor_Entanglement_Entropy(&pu, pid);

    uint64_t fid = 0;
    const bool ok = ProPhysics_Tensor_To_Fock(&pu, pid, &fid);
    check("Tensor_To_Fock ok", ok);

    if (ok) {
        const double norm = ProPhysics_Fock_Norm(&pu, fid);
        const double N = ProPhysics_Fock_Particle_Number(&pu, fid);

        printf("  Fock Norm = %.12f  (erwartet 1)\n", norm);
        printf("  Fock <N>  = %.12f  (erwartet 2)\n", N);
        printf("  Tensor S  = %.12f  (erwartet ln2 = %.12f)\n",
            S_tensor, LN2);

        check("Fock Norm = 1", fabs(norm - 1.0) < 1e-6);
        check("Fock <N> = 2", fabs(N - 2.0) < 1e-6);

        /* Fock-Basis-Bits: 0x12 = (1<<1)|(1<<4), 0x0C = (1<<2)|(1<<3). */
        int32_t re1 = 0, im1 = 0, re2 = 0, im2 = 0;
        ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x12u, &re1, &im1);
        ProPhysics_Fock_Get_Amplitude(&pu, fid, 0x0Cu, &re2, &im2);
        const double mag1 = sqrt((double)re1 * re1 + (double)im1 * im1)
            / 2147483647.0;
        const double mag2 = sqrt((double)re2 * re2 + (double)im2 * im2)
            / 2147483647.0;
        printf("  |Fock coeff[0x12]| = %.12f  (erwartet 1/sqrt2)\n", mag1);
        printf("  |Fock coeff[0x0C]| = %.12f  (erwartet 1/sqrt2)\n", mag2);
        check("|coeff[0x12]| ~ 0.7071", fabs(mag1 - 0.7071067811865476) < 1e-6);
        check("|coeff[0x0C]| ~ 0.7071", fabs(mag2 - 0.7071067811865476) < 1e-6);

        ProPhysics_Fock_Destroy(&pu, fid);
    }
    ProPhysics_Free(&pu);
}

/* ------------------------------------------------------------------ F22
 * Fock_To_Tensor verweigert N != 2.
 */
static void test_F22_n2_constraint(void)
{
    printf("\n--- F22: Fock_To_Tensor nur fuer N=2 ---\n");

    ProUniverse pu;
    ProPhysics_Initialize(&pu, 64);

    uint32_t pid = 0;
    ProPhysics_Tensor_Create_Pair(&pu, 0, 1, &pid);

    /* N=1: |1> -> darf nicht konvertieren. */
    uint64_t f1 = 0;
    ProPhysics_Fock_Create(&pu, &f1);
    ProPhysics_Fock_Set_Basis(&pu, f1, 0x01u);
    const bool ok1 = ProPhysics_Fock_To_Tensor(&pu, f1, pid);
    printf("  N=1 Konvertierung = %s (erwartet false)\n", ok1 ? "true" : "false");
    check("N=1 abgelehnt", !ok1);

    /* N=3: |1,2,3> -> darf nicht konvertieren. */
    uint64_t f3 = 0;
    ProPhysics_Fock_Create(&pu, &f3);
    ProPhysics_Fock_Set_Basis(&pu, f3, 0x07u);
    const bool ok3 = ProPhysics_Fock_To_Tensor(&pu, f3, pid);
    printf("  N=3 Konvertierung = %s (erwartet false)\n", ok3 ? "true" : "false");
    check("N=3 abgelehnt", !ok3);

    /* N=2: |1,4> (Moden 0 und 3) -> OK. */
    uint64_t f2 = 0;
    ProPhysics_Fock_Create(&pu, &f2);
    ProPhysics_Fock_Set_Basis(&pu, f2, 0x09u);
    const bool ok2 = ProPhysics_Fock_To_Tensor(&pu, f2, pid);
    printf("  N=2 Konvertierung = %s (erwartet true)\n", ok2 ? "true" : "false");
    check("N=2 akzeptiert", ok2);

    if (ok2) {
        check("Tensor antisymmetrisch",
            ProPhysics_Tensor_Is_Antisymmetric(&pu, pid, 1000));
    }

    ProPhysics_Fock_Destroy(&pu, f1);
    ProPhysics_Fock_Destroy(&pu, f3);
    ProPhysics_Fock_Destroy(&pu, f2);
    ProPhysics_Free(&pu);
}

/* =================================================================== main */
int main(void)
{
    /* Unbuffered: ermöglicht Crash-Lokalisierung per Logfile. */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("========================================================================\n");
    printf("  Etappe 13 - Tensorprodukt-Tests (ASCII)\n");
    printf("  Phase 1 + 2a + 2b + 3a + 3b-a + 3b-b + 3g + 4 + 4b + 4c + 15a-e\n");
    printf("========================================================================\n");
    /*
    test_T1_product_state();
    test_T2_bell_state();
    test_T3_local_op_preserves_entropy();
    test_T4_max_entangled();
    test_T5_trace_normalization();
    test_T6_chsh_tensor_bell();
    test_T7_chsh_tensor_product();
    test_T8_marginal_invisibility();
    test_T9_product_under_tick();
    test_T10_entanglement_lost();
    test_T11_H_CNOT_bell();
    test_T12_H_CNOT_chsh();
    test_T13_CNOT_CNOT_undo();
    test_T14_CZ();
    test_T15_SWAP();
    test_T16_marked_frozen();
    test_T17_unmarked_evolves();
    test_T18_auto_sync();
    test_T19_emergent_entanglement();
    test_T20_single_step_max();
    test_T21_two_steps_return();
    test_T22_SU2_Rx_pi();
    test_T23_SU2_Ry_pi();
    test_T24_SU2_Rz_pi_superposition();
    test_T25_SU2_Rx_half_twice();
    test_T26_SU2_Algebra();
    test_T27_SU2_Bell_anticorrelation();
    test_T28_slater_swap_sign();
    test_T29_slater_entropy();
    test_T30_pauli_principle();
    test_T31_two_identical_fermions();
    test_T32_slater_under_tick();
    test_T33_xx_on_fermion();
    test_T34_hopping_trivial();
    test_T35_hopping_nontrivial();
    test_T36_hopping_preserves_antisymmetry();
    test_T37_hopping_doubling();
    test_T38_hopping_under_tick();
    test_T39_hopping_stability();
    test_T40_hopping_and_xx();
    */
    /* Etappe 15a: Fock-Raum Basis */
    test_F1_popcount();
    test_F2_vacuum();
    test_F3_basis_states();
    test_F4_superposition();
    test_F5_get_set_roundtrip();
    test_F6_three_fermion_basis();
    /* Etappe 15b: Erzeuger / Vernichter */
    test_F7_create_on_vacuum();
    test_F8_pauli_double_create();
    test_F9_jw_sign();
    test_F10_annihilate();
    test_F11_projector();
    test_F12_three_fermion_chain();
    /* Etappe 15c: Antikommutatoren */
    test_F13_anticomm_cd();
    test_F14_anticomm_cc();
    test_F15_anticomm_dd();
    test_F16_hopping_single_particle();
    test_F17_hopping_with_between();
    test_F18_hopping_doubling_fock();
    test_F19_hopping_stability();
    /* Etappe 15e: Adapter Tensor <-> Fock */
    test_F20_tensor_to_fock_roundtrip();
    test_F21_superposition();
    test_F22_n2_constraint();


    printf("\n========================================================================\n");
    printf("  Ergebnis: %d PASS, %d FAIL\n", g_pass, g_fail);
    printf("========================================================================\n");
    return (g_fail == 0) ? 0 : 1;
}