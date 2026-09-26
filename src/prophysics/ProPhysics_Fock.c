/* ==========================================================================
 * ProPhysics - Fock Modul
 * File: ProPhysics_Fock.c
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * 8-Moden-Fock-Raum in Q31, 256-dim Basis, Jordan-Wigner.
 *
 * Verantwortlich fuer:
 *   - Popcount (SWAR, 8 Bit)
 *   - Lifecycle: Create / Destroy / Set_Basis / Count
 *   - Amplitude Get/Set, Norm, Particle_Number, Is_Zero
 *   - Clone, Compare, Scale
 *   - Erzeuger/Vernichter mit Jordan-Wigner-String
 *   - Antikommutatoren {c_i, c+_j}, {c_i, c_j}, {c+_i, c+_j}
 *   - Fermionisches Hopping
 *
 * Basis-Konvention:
 *   Bit i = 1  bedeutet Mode i ist besetzt.
 *   Index = Bitmuster direkt (0..255).
 *
 * Normierung:
 *   Summe |coeff|^2 = 2^62 entspricht physikalisch 1.
 *
 * Jordan-Wigner:
 *   c+_i |n_0...n_7> = (-1)^{Sum_{j<i} n_j} (1-n_i) |...n_i=1...>
 *   c_i  |n_0...n_7> = (-1)^{Sum_{j<i} n_j}    n_i  |...n_i=0...>
 *
 * Interne Helfer:
 *   - pro_fock_apply_jw_sign (JW-Vorzeichen, ersetzt 3x)
 *   - pro_fock_apply_op (einzelne Erzeuger/Vernichter-Operation)
 *   - pro_fock_anticomm_impl (Antikommutator-Muster, ersetzt 3x)
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * Konfiguration:       siehe docs/project/CONFIG.md.
 * Modul-Dokumentation: siehe docs/project/Fock.md.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* ==========================================================================
  * Interne Helfer
  * ========================================================================== */

  /* Wendet das Jordan-Wigner-Vorzeichen auf einen Q31-Wert an:
   *   (-1)^{below_count} · v
   *
   * Der Zaehler below_count ist die Anzahl besetzter Moden strikt
   * unterhalb des Operators. Bei ungeradem Zaehler wird der Wert negiert.
   *
   * Ersetzt 3x wiederholten 4-Zeilen-Block in Create, Annihilate und
   * Create_Plus_Annihilate. */
static inline ProAmpQ31 pro_fock_apply_jw_sign(ProAmpQ31 v, uint32_t below_count)
{
    if (below_count & 1u) {
        return pro_amp_pack(pro_neg_q31(pro_amp_real(v)),
            pro_neg_q31(pro_amp_imag(v)));
    }
    return v;
}

/* Art einer fermionischen Operation: Erzeuger oder Vernichter.
 * Der Wert 0 bleibt fuer "No-Op" reserviert und wird nicht verwendet. */
typedef enum {
    PRO_FOCK_OP_CREATE = 1,
    PRO_FOCK_OP_ANNIHILATE = 2
} ProFockOpKind;

/* Wendet eine einzelne Erzeuger- oder Vernichter-Operation an.
 * Duenne Dispatcher-Funktion, damit pro_fock_anticomm_impl nicht
 * mit if/else-Ketten ueberladen wird. */
static void pro_fock_apply_op(ProUniverse* pu, uint64_t fock_id,
    ProFockOpKind kind, uint8_t mode)
{
    if (kind == PRO_FOCK_OP_CREATE) {
        ProPhysics_Fock_Apply_Create(pu, fock_id, mode);
    }
    else if (kind == PRO_FOCK_OP_ANNIHILATE) {
        ProPhysics_Fock_Apply_Annihilate(pu, fock_id, mode);
    }
}

/* Generisches Antikommutator-Muster.
 *
 * Wendet (AB + BA) auf einen Zustand an. A und B sind jeweils als
 * Folge von zwei Operationen kodiert (a1 mit Mode j, a2 mit Mode i
 * fuer A; b1 mit Mode i, b2 mit Mode j fuer B).
 *
 *   A = op(a2, i) · op(a1, j)
 *   B = op(b2, j) · op(b1, i)
 *
 * Ergebnis wird in dst = A·psi + B·psi zurueckgeschrieben.
 *
 * Ersetzt 3x wiederholte Clone+Apply+Sum+Destroy-Sequenz in CD, CC, DD. */
static bool pro_fock_anticomm_impl(
    ProUniverse* pu, uint64_t fock_id,
    uint8_t i, uint8_t j,
    ProFockOpKind a1, ProFockOpKind a2,
    ProFockOpKind b1, ProFockOpKind b2)
{
    uint64_t idA = 0, idB = 0;
    if (!ProPhysics_Fock_Clone(pu, fock_id, &idA)) return false;
    if (!ProPhysics_Fock_Clone(pu, fock_id, &idB)) {
        ProPhysics_Fock_Destroy(pu, idA);
        return false;
    }

    /* A = a2(i) · a1(j) */
    pro_fock_apply_op(pu, idA, a1, j);
    pro_fock_apply_op(pu, idA, a2, i);

    /* B = b2(j) · b1(i) */
    pro_fock_apply_op(pu, idB, b1, i);
    pro_fock_apply_op(pu, idB, b2, j);

    ProFockState* dst = &pu->fock_states[fock_id];
    const ProFockState* A = &pu->fock_states[idA];
    const ProFockState* B = &pu->fock_states[idB];

    for (uint32_t k = 0; k < PRO_FOCK_DIM; ++k) {
        dst->coeff[k] = pro_amp_add_q31(A->coeff[k], B->coeff[k]);
    }

    ProPhysics_Fock_Destroy(pu, idA);
    ProPhysics_Fock_Destroy(pu, idB);
    return true;
}

/* ==========================================================================
 * Popcount (SWAR, 8 Bit)
 * ========================================================================== */

PROPHYSICS_API uint32_t ProPhysics_Fock_Popcount(uint8_t bits)
{
    uint32_t x = (uint32_t)bits;
    x = x - ((x >> 1) & 0x55u);
    x = (x & 0x33u) + ((x >> 2) & 0x33u);
    x = (x + (x >> 4)) & 0x0Fu;
    return x;
}

/* ==========================================================================
 * Lifecycle
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Fock_Create(
    ProUniverse* pu, uint64_t* out_fock_id)
{
    if (!pu || !pu->fock_states) return false;
    if (pu->fock_state_count >= pu->fock_state_capacity) return false;

    int64_t slot = -1;
    for (uint64_t i = 0; i < pu->fock_state_capacity; ++i) {
        if (pu->fock_states[i].active == 0u) {
            slot = (int64_t)i;
            break;
        }
    }
    if (slot < 0) return false;

    ProFockState* f = &pu->fock_states[slot];
    f->fock_id = (uint64_t)slot;
    f->active = 1u;
    f->n_particles = 0u;
    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) f->coeff[i] = 0;

    /* Vakuum: coeff[0] = 1. */
    f->coeff[0] = pro_amp_pack(INT32_MAX, 0);

    pu->fock_state_count++;
    if (out_fock_id) *out_fock_id = (uint64_t)slot;
    return true;
}

PROPHYSICS_API bool ProPhysics_Fock_Destroy(
    ProUniverse* pu, uint64_t fock_id)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    if (pu->fock_states[fock_id].active == 0u) return false;

    pu->fock_states[fock_id].active = 0u;
    if (pu->fock_state_count > 0u) pu->fock_state_count--;
    return true;
}

PROPHYSICS_API bool ProPhysics_Fock_Set_Basis(
    ProUniverse* pu, uint64_t fock_id, uint8_t bits)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return false;

    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) f->coeff[i] = 0;
    f->coeff[bits] = pro_amp_pack(INT32_MAX, 0);
    f->n_particles = ProPhysics_Fock_Popcount(bits);
    return true;
}

PROPHYSICS_API uint64_t ProPhysics_Fock_Count(const ProUniverse* pu)
{
    return pu ? pu->fock_state_count : 0u;
}

/* ==========================================================================
 * Amplitude-Get/Set (einzelne Koeffizienten)
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Fock_Get_Amplitude(
    const ProUniverse* pu, uint64_t fock_id, uint8_t bits,
    int32_t* out_re_q31, int32_t* out_im_q31)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    const ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return false;
    if (!out_re_q31 || !out_im_q31) return false;

    *out_re_q31 = pro_amp_real(f->coeff[bits]);
    *out_im_q31 = pro_amp_imag(f->coeff[bits]);
    return true;
}

PROPHYSICS_API bool ProPhysics_Fock_Set_Amplitude(
    ProUniverse* pu, uint64_t fock_id, uint8_t bits,
    int32_t re_q31, int32_t im_q31)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return false;

    f->coeff[bits] = pro_amp_pack(re_q31, im_q31);
    return true;
}

/* ==========================================================================
 * Norm, Particle_Number, Is_Zero
 * ========================================================================== */

PROPHYSICS_API double ProPhysics_Fock_Norm(
    const ProUniverse* pu, uint64_t fock_id)
{
    if (!pu || !pu->fock_states) return 0.0;
    if (fock_id >= pu->fock_state_capacity) return 0.0;
    const ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return 0.0;

    /* ProU128, um Overflow bei 256 Koeffizienten zu vermeiden. */
    ProU128 sum = PRO_U128_ZERO;
    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) {
        const uint64_t sq = pro_amp_abs2(f->coeff[i]);
        sum = pro_u128_add(sum, pro_u128_from_u64(sq));
    }
    return pro_u128_to_double(sum) / 4611686018427387904.0;
}

PROPHYSICS_API double ProPhysics_Fock_Particle_Number(
    const ProUniverse* pu, uint64_t fock_id)
{
    if (!pu || !pu->fock_states) return 0.0;
    if (fock_id >= pu->fock_state_capacity) return 0.0;
    const ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return 0.0;

    const double inv_q62 = 1.0 / 4611686018427387904.0;
    double sum = 0.0;
    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) {
        const double sq = (double)pro_amp_abs2(f->coeff[i]) * inv_q62;
        sum += (double)ProPhysics_Fock_Popcount((uint8_t)i) * sq;
    }
    return sum;
}

PROPHYSICS_API bool ProPhysics_Fock_Is_Zero(
    const ProUniverse* pu, uint64_t fock_id)
{
    if (!pu || !pu->fock_states) return true;
    if (fock_id >= pu->fock_state_capacity) return true;
    const ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return true;
    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) {
        if (f->coeff[i] != 0) return false;
    }
    return true;
}

/* ==========================================================================
 * Clone, Compare, Scale
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Fock_Clone(
    ProUniverse* pu, uint64_t src_fock_id, uint64_t* out_dst_fock_id)
{
    if (!pu || !pu->fock_states) return false;
    if (src_fock_id >= pu->fock_state_capacity) return false;
    const ProFockState* src = &pu->fock_states[src_fock_id];
    if (src->active == 0u) return false;

    uint64_t dst_id = 0;
    if (!ProPhysics_Fock_Create(pu, &dst_id)) return false;
    ProFockState* dst = &pu->fock_states[dst_id];
    dst->n_particles = src->n_particles;
    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) {
        dst->coeff[i] = src->coeff[i];
    }
    if (out_dst_fock_id) *out_dst_fock_id = dst_id;
    return true;
}

PROPHYSICS_API double ProPhysics_Fock_Compare(
    const ProUniverse* pu, uint64_t fid_a, uint64_t fid_b)
{
    if (!pu || !pu->fock_states) return 1e300;
    if (fid_a >= pu->fock_state_capacity) return 1e300;
    if (fid_b >= pu->fock_state_capacity) return 1e300;
    const ProFockState* a = &pu->fock_states[fid_a];
    const ProFockState* b = &pu->fock_states[fid_b];
    if (a->active == 0u || b->active == 0u) return 1e300;

    double max_diff = 0.0;
    for (uint32_t k = 0; k < PRO_FOCK_DIM; ++k) {
        const int64_t dr = (int64_t)pro_amp_real(a->coeff[k])
            - (int64_t)pro_amp_real(b->coeff[k]);
        const int64_t di = (int64_t)pro_amp_imag(a->coeff[k])
            - (int64_t)pro_amp_imag(b->coeff[k]);
        const double err = sqrt((double)(dr * dr + di * di));
        if (err > max_diff) max_diff = err;
    }
    return max_diff;
}

PROPHYSICS_API bool ProPhysics_Fock_Scale(
    ProUniverse* pu, uint64_t fock_id, int32_t scale_q31)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return false;

    const int64_t s = (int64_t)scale_q31;
    for (uint32_t k = 0; k < PRO_FOCK_DIM; ++k) {
        f->coeff[k] = pro_amp_scale_q31(f->coeff[k], s);
    }
    return true;
}

/* ==========================================================================
 * Erzeuger / Vernichter mit Jordan-Wigner-String
 *
 * Sign: (-1)^{Anzahl besetzter Moden strikt unterhalb mode}.
 * Auf Superpositionen: der ganze 256-dim Zustand wird transformiert.
 * Rueckgabe false, wenn das Ergebnis exakt Null ist (z.B. Pauli-Blockade).
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Fock_Apply_Create(
    ProUniverse* pu, uint64_t fock_id, uint8_t mode)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return false;
    if (mode >= PRO_FOCK_MODES) return false;

    const uint32_t mask = 1u << mode;
    const uint8_t  below_mask = (uint8_t)(mask - 1u);

    ProAmpQ31 tmp[PRO_FOCK_DIM];
    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) tmp[i] = 0;

    for (uint32_t bits = 0; bits < PRO_FOCK_DIM; ++bits) {
        const ProAmpQ31 c = f->coeff[bits];
        if (c == 0) continue;
        if ((bits & mask) != 0u) continue;   /* schon besetzt -> Pauli */

        const uint8_t result = (uint8_t)(bits | mask);
        const uint32_t below = ProPhysics_Fock_Popcount(
            (uint8_t)(bits & below_mask));

        tmp[result] = pro_fock_apply_jw_sign(c, below);
    }

    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) f->coeff[i] = tmp[i];
    return !ProPhysics_Fock_Is_Zero(pu, fock_id);
}

PROPHYSICS_API bool ProPhysics_Fock_Apply_Annihilate(
    ProUniverse* pu, uint64_t fock_id, uint8_t mode)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return false;
    if (mode >= PRO_FOCK_MODES) return false;

    const uint32_t mask = 1u << mode;
    const uint8_t  below_mask = (uint8_t)(mask - 1u);

    ProAmpQ31 tmp[PRO_FOCK_DIM];
    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) tmp[i] = 0;

    for (uint32_t bits = 0; bits < PRO_FOCK_DIM; ++bits) {
        const ProAmpQ31 c = f->coeff[bits];
        if (c == 0) continue;
        if ((bits & mask) == 0u) continue;   /* leer -> 0 */

        const uint8_t result = (uint8_t)(bits & ~mask);
        const uint32_t below = ProPhysics_Fock_Popcount(
            (uint8_t)(bits & below_mask));

        tmp[result] = pro_fock_apply_jw_sign(c, below);
    }

    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) f->coeff[i] = tmp[i];
    return !ProPhysics_Fock_Is_Zero(pu, fock_id);
}

PROPHYSICS_API bool ProPhysics_Fock_Apply_Create_Plus_Annihilate(
    ProUniverse* pu, uint64_t fock_id, uint8_t mode)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return false;
    if (mode >= PRO_FOCK_MODES) return false;

    const uint32_t mask = 1u << mode;
    const uint8_t  below_mask = (uint8_t)(mask - 1u);

    ProAmpQ31 tmp[PRO_FOCK_DIM];
    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) tmp[i] = 0;

    for (uint32_t bits = 0; bits < PRO_FOCK_DIM; ++bits) {
        const ProAmpQ31 c = f->coeff[bits];
        if (c == 0) continue;

        const uint32_t below = ProPhysics_Fock_Popcount(
            (uint8_t)(bits & below_mask));
        const ProAmpQ31 v = pro_fock_apply_jw_sign(c, below);

        /* (c+_i + c_i) toggelt die Besetzung in Mode i. */
        const uint8_t result = (uint8_t)(bits ^ mask);
        tmp[result] = pro_amp_add_q31(tmp[result], v);
    }

    for (uint32_t i = 0; i < PRO_FOCK_DIM; ++i) f->coeff[i] = tmp[i];
    return !ProPhysics_Fock_Is_Zero(pu, fock_id);
}

/* ==========================================================================
 * Antikommutatoren-Test
 *
 * Wendet {A, B} |psi> = (AB + BA) |psi> an und schreibt das Ergebnis
 * nach fock_id zurueck. Intern wird auf Klonen gearbeitet, um kein
 * Zwischenergebnis zu verlieren.
 *
 * Die drei Varianten unterscheiden sich nur in den Operationen:
 *   CD: {c_i, c+_j}  -> A = c+_j, c_i  |  B = c_i, c+_j
 *   CC: {c_i, c_j}   -> A = c_j,  c_i  |  B = c_i, c_j
 *   DD: {c+_i, c+_j} -> A = c+_j, c+_i |  B = c+_i, c+_j
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Fock_Apply_Anticomm_CD(
    ProUniverse* pu, uint64_t fock_id, uint8_t i, uint8_t j)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    if (i >= PRO_FOCK_MODES || j >= PRO_FOCK_MODES) return false;

    /* A = c_i(c+_j)|psi>;  B = c+_j(c_i)|psi> */
    return pro_fock_anticomm_impl(pu, fock_id, i, j,
        PRO_FOCK_OP_CREATE, PRO_FOCK_OP_ANNIHILATE,
        PRO_FOCK_OP_ANNIHILATE, PRO_FOCK_OP_CREATE);
}

PROPHYSICS_API bool ProPhysics_Fock_Apply_Anticomm_CC(
    ProUniverse* pu, uint64_t fock_id, uint8_t i, uint8_t j)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    if (i >= PRO_FOCK_MODES || j >= PRO_FOCK_MODES) return false;

    /* A = c_i(c_j)|psi>;  B = c_j(c_i)|psi> */
    return pro_fock_anticomm_impl(pu, fock_id, i, j,
        PRO_FOCK_OP_ANNIHILATE, PRO_FOCK_OP_ANNIHILATE,
        PRO_FOCK_OP_ANNIHILATE, PRO_FOCK_OP_ANNIHILATE);
}

PROPHYSICS_API bool ProPhysics_Fock_Apply_Anticomm_DD(
    ProUniverse* pu, uint64_t fock_id, uint8_t i, uint8_t j)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    if (i >= PRO_FOCK_MODES || j >= PRO_FOCK_MODES) return false;

    /* A = c+_i(c+_j)|psi>;  B = c+_j(c+_i)|psi> */
    return pro_fock_anticomm_impl(pu, fock_id, i, j,
        PRO_FOCK_OP_CREATE, PRO_FOCK_OP_CREATE,
        PRO_FOCK_OP_CREATE, PRO_FOCK_OP_CREATE);
}

/* ==========================================================================
 * Fermionisches Hopping
 *
 *   U(theta) = exp(-i theta H),  H = c+_i c_j + c+_j c_i
 *
 * Wirkt als 2x2-Rotation in jedem Unterraum {|S>, |S'>}, wobei in S
 * Mode i besetzt und Mode j leer ist, und S' = S ^ (mask_i | mask_j).
 * Jordan-Wigner-Zwischenstring liefert das Vorzeichen
 *   sigma = (-1)^{|S & (Moden strikt zwischen i und j)|}.
 *
 * Erhaelt Norm und Teilchenzahl exakt.
 * ========================================================================== */

PROPHYSICS_API bool ProPhysics_Fock_Apply_Hopping(
    ProUniverse* pu, uint64_t fock_id,
    uint8_t orbital_i, uint8_t orbital_j, int32_t theta_q15)
{
    if (!pu || !pu->fock_states) return false;
    if (fock_id >= pu->fock_state_capacity) return false;
    ProFockState* f = &pu->fock_states[fock_id];
    if (f->active == 0u) return false;
    if (orbital_i >= PRO_FOCK_MODES || orbital_j >= PRO_FOCK_MODES) return false;
    if (orbital_i == orbital_j) return false;
    if (theta_q15 == 0) return true;

    /* Normiere auf i < j. */
    const uint32_t i = (orbital_i < orbital_j) ? orbital_i : orbital_j;
    const uint32_t j = (orbital_i < orbital_j) ? orbital_j : orbital_i;

    const uint32_t mask_i = 1u << i;
    const uint32_t mask_j = 1u << j;
    const uint32_t pair_mask = mask_i | mask_j;
    const uint32_t between_mask = mask_j - (mask_i << 1u);

    const double theta = (double)theta_q15 / 32768.0;
    const double c = cos(theta);
    const double s = sin(theta);

    ProAmpQ31 tmp[PRO_FOCK_DIM];
    for (uint32_t k = 0; k < PRO_FOCK_DIM; ++k) tmp[k] = f->coeff[k];

    for (uint32_t bits = 0; bits < PRO_FOCK_DIM; ++bits) {
        if ((bits & mask_i) == 0u) continue;   /* i muss besetzt */
        if ((bits & mask_j) != 0u) continue;   /* j muss leer */

        const uint32_t bits2 = bits ^ pair_mask;
        const uint32_t between = bits & between_mask;
        const uint32_t pcount = ProPhysics_Fock_Popcount((uint8_t)between);
        const double s_signed = s * ((pcount & 1u) ? -1.0 : +1.0);

        const ProAmpQ31 a = tmp[bits];
        const ProAmpQ31 b = tmp[bits2];

        const int64_t ar = (int64_t)pro_amp_real(a);
        const int64_t ai = (int64_t)pro_amp_imag(a);
        const int64_t br = (int64_t)pro_amp_real(b);
        const int64_t bi = (int64_t)pro_amp_imag(b);

        /* Update: a' = c*a + i*s_signed*b
         *         b' = c*b + i*s_signed*a
         * i*s_signed*z = s_signed*(im z) - i*s_signed*(re z) */
        const double na_re = c * (double)ar + s_signed * (double)bi;
        const double na_im = c * (double)ai - s_signed * (double)br;
        const double nb_re = c * (double)br + s_signed * (double)ai;
        const double nb_im = c * (double)bi - s_signed * (double)ar;

        tmp[bits] = pro_amp_pack(pro_sat_i32((int64_t)llround(na_re)),
            pro_sat_i32((int64_t)llround(na_im)));
        tmp[bits2] = pro_amp_pack(pro_sat_i32((int64_t)llround(nb_re)),
            pro_sat_i32((int64_t)llround(nb_im)));
    }

    for (uint32_t k = 0; k < PRO_FOCK_DIM; ++k) f->coeff[k] = tmp[k];
    return true;
}

/* ==========================================================================
 * End of ProPhysics_Fock.c
 * ========================================================================== */