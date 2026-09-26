/* ==========================================================================
 * ProPhysics - SU(2)-Link-Dynamik
 * File: ProPhysics_SU2_Dynamics.c
 *
 * Kernel: 1.23.0
 * Etappe: 23
 *
 * Klassische Yang-Mills-Dynamik auf dem Gitter mit Leapfrog-Integration.
 *
 * Verantwortlich fuer:
 *   - ProPhysics_Enable_SU2_Dynamics / _Disable / _Is_Active
 *   - ProPhysics_Set_SU2_Yang_Mills
 *   - ProPhysics_Apply_SU2_Tick
 *   - ProPhysics_SU2_Plaquette_Action
 *   - ProPhysics_SU2_Total_Energy
 *   - ProPhysics_SU2_Link_Plaquette_Sum
 *
 * Hamilton-Funktion:
 *   H = S_plaq + (1/2) * Sum_links |E|^2
 *
 * Wilson-Plaquette-Action:
 *   S_plaq = Sum_plaq (1 - 0.5 * Re Tr(W_plaq))
 *   mit W_plaq = U_mu(x) U_nu(x+mu) U_mu(x+nu)^dagger U_nu(x)^dagger
 *
 * Leapfrog-Schritt (Stoermer-Verlet):
 *   1. E <- E + (dt/2) * g^2 * F(U)
 *   2. U <- exp(i * dt * E) * U
 *   3. E <- E + (dt/2) * g^2 * F(U_neu)
 * wobei F(U) die su(2)-Projektion der Staple-Summe ist.
 *
 * Alle Operationen sind additiv: sie modifizieren nur ProEdge.su2_*
 * und ProEdge.su2_E_*. amp_grid bleibt unberuehrt.
 *
 * R-Konformitaet:
 *   R1: Kein div/mod im Hotpath (Bit-Shift).
 *   R2: Kein malloc/calloc im Hotpath.
 *   R3: U5 bleibt erhalten (nur Link-Metadaten).
 *   R4: Link-Update unitaer (exp-Map ueber Quaternion).
 *   R7: Bei su2_dynamics_active == 0 liest niemand diese Felder.
 *
 * Interne Helfer:
 *   - su2_link_exists (Nachbarschafts-Check, kanal-spezifisch)
 *   - su2_read_link / su2_write_link (Link-Felder, via pro_su2_edge)
 *   - su2_read_E / su2_write_E (E-Felder, via pro_su2_edge)
 *   - su2_accumulate_staple (Staple-Beitrag zur su(2)-Kraft)
 *   - su2_force_on_link (Staple-Summe ueber 3 Richtungen)
 *   - su2_plaquette_action_at (4-Link-Plaquette -> Wilson-Aktion)
 *   - su2_leapfrog_kick_E (Schritt 1+3 der Leapfrog-Integration)
 *
 * Aenderungshistorie: siehe CHANGELOG.md im Projekt-Root.
 * Konfiguration:       siehe docs/project/CONFIG.md.
 * Modul-Dokumentation: siehe docs/project/SU2_Dynamics.md.
 * ========================================================================== */

#include "ProPhysics_Internal.h"

 /* --------------------------------------------------------------------------
  * Kanal-Mapping
  * -------------------------------------------------------------------------- */
static const uint8_t SU2_FWD_CH[3] = {
    PRO_NEIGHBOR_X_PLUS,  /* 0 = +x */
    PRO_NEIGHBOR_Y_PLUS,  /* 2 = +y */
    PRO_NEIGHBOR_Z_PLUS   /* 4 = +z */
};
static const uint8_t SU2_REV_CH[3] = {
    PRO_NEIGHBOR_X_MINUS, /* 1 = -x */
    PRO_NEIGHBOR_Y_MINUS, /* 3 = -y */
    PRO_NEIGHBOR_Z_MINUS  /* 6 = -z */
};

/* --------------------------------------------------------------------------
 * Grundoperationen
 * -------------------------------------------------------------------------- */

static inline bool su2_link_exists(const ProUniverse* pu, uint64_t x, uint8_t ch)
{
    if (!pu || !pu->reg_source) return false;
    if (x >= pu->total_nodes) return false;
    if (ch >= (uint8_t)CHANNELS_MAX) return false;
    const uint64_t nb = pu->reg_source[x].channels[ch];
    return (nb < pu->total_nodes) && (nb != x);
}

/* Link-Felder lesen/schreiben. pro_su2_edge ist in Internal.h
 * definiert (Patch 1.23.7, loest B7). Aufrufer muessen (x, ch)
 * validiert haben (typischerweise via su2_link_exists). */
static inline void su2_read_link(const ProUniverse* pu, uint64_t x, uint8_t ch,
    int32_t* a_re, int32_t* a_im, int32_t* b_re, int32_t* b_im)
{
    const ProEdge* e = pro_su2_edge(pu, x, ch);
    if (!e) return;
    *a_re = e->su2_a_re;
    *a_im = e->su2_a_im;
    *b_re = e->su2_b_re;
    *b_im = e->su2_b_im;
}

static inline void su2_read_E(const ProUniverse* pu, uint64_t x, uint8_t ch,
    int32_t* a_re, int32_t* a_im, int32_t* b_re, int32_t* b_im)
{
    const ProEdge* e = pro_su2_edge(pu, x, ch);
    if (!e) return;
    *a_re = e->su2_E_a_re;
    *a_im = e->su2_E_a_im;
    *b_re = e->su2_E_b_re;
    *b_im = e->su2_E_b_im;
}

static inline void su2_write_link(ProUniverse* pu, uint64_t x, uint8_t ch,
    int32_t a_re, int32_t a_im, int32_t b_re, int32_t b_im)
{
    ProEdge* e = pro_su2_edge_mut(pu, x, ch);
    if (!e) return;
    e->su2_a_re = a_re;
    e->su2_a_im = a_im;
    e->su2_b_re = b_re;
    e->su2_b_im = b_im;
}

static inline void su2_write_E(ProUniverse* pu, uint64_t x, uint8_t ch,
    int32_t a_re, int32_t a_im, int32_t b_re, int32_t b_im)
{
    ProEdge* e = pro_su2_edge_mut(pu, x, ch);
    if (!e) return;
    e->su2_E_a_re = a_re;
    e->su2_E_a_im = a_im;
    e->su2_E_b_re = b_re;
    e->su2_E_b_im = b_im;
}

/* --------------------------------------------------------------------------
 * Staple-Force-Beitrag
 *
 * P = U * S^dagger;  su(2)-Projektion von P ist (0, P_a_im, P_b_re, P_b_im).
 * Der Realteil P_a_re wird verworfen.
 * -------------------------------------------------------------------------- */
static inline void su2_accumulate_staple(
    int32_t u_ar, int32_t u_ai, int32_t u_br, int32_t u_bi,
    int32_t s_ar, int32_t s_ai, int32_t s_br, int32_t s_bi,
    int64_t* acc_ai, int64_t* acc_br, int64_t* acc_bi)
{
    /* S^dagger = (s_ar, -s_ai, -s_br, -s_bi). */
    const int32_t sd_ai = (int32_t)(0u - (uint32_t)s_ai);
    const int32_t sd_br = (int32_t)(0u - (uint32_t)s_br);
    const int32_t sd_bi = (int32_t)(0u - (uint32_t)s_bi);

    int32_t p_ar, p_ai, p_br, p_bi;
    pro_su2_mul(u_ar, u_ai, u_br, u_bi,
        s_ar, sd_ai, sd_br, sd_bi,
        &p_ar, &p_ai, &p_br, &p_bi);
    (void)p_ar;
    *acc_ai += (int64_t)p_ai;
    *acc_br += (int64_t)p_br;
    *acc_bi += (int64_t)p_bi;
}

/* --------------------------------------------------------------------------
 * Force auf Link (x, mu)
 *
 * mu in {0, 2, 4}. Liefert su(2)-Kraft in Quaternion-Form
 * (0, f_ai, f_br, f_bi).
 *
 * Standard-Staple-Formel:
 *   Forward:  S = U_nu(x+mu) * U_mu(x+nu)^dagger * U_nu(x)^dagger
 *   Backward: S = U_nu(x+mu-nu)^dagger * U_mu(x-nu)^dagger * U_nu(x-nu)
 * -------------------------------------------------------------------------- */
static void su2_force_on_link(const ProUniverse* pu, uint64_t x, uint8_t mu,
    int32_t* out_ai, int32_t* out_br, int32_t* out_bi)
{
    *out_ai = 0; *out_br = 0; *out_bi = 0;
    if (!su2_link_exists(pu, x, mu)) return;

    int32_t u_ar, u_ai, u_br, u_bi;
    su2_read_link(pu, x, mu, &u_ar, &u_ai, &u_br, &u_bi);

    int64_t acc_ai = 0, acc_br = 0, acc_bi = 0;
    int n = 0;

    for (int i = 0; i < 3; ++i) {
        const uint8_t nu = SU2_FWD_CH[i];
        if (nu == mu) continue;
        const uint8_t nu_rev = SU2_REV_CH[i];

        /* --- Forward --- */
        if (su2_link_exists(pu, x, nu)) {
            const uint64_t x_mu = pu->reg_source[x].channels[mu];
            const uint64_t x_nu = pu->reg_source[x].channels[nu];
            if (x_mu < pu->total_nodes && x_nu < pu->total_nodes &&
                su2_link_exists(pu, x_mu, nu) &&
                su2_link_exists(pu, x_nu, mu)) {

                int32_t l1ar, l1ai, l1br, l1bi; /* U(x+mu, nu) */
                int32_t l2ar, l2ai, l2br, l2bi; /* U(x+nu, mu) */
                int32_t l3ar, l3ai, l3br, l3bi; /* U(x, nu) */
                su2_read_link(pu, x_mu, nu, &l1ar, &l1ai, &l1br, &l1bi);
                su2_read_link(pu, x_nu, mu, &l2ar, &l2ai, &l2br, &l2bi);
                su2_read_link(pu, x, nu, &l3ar, &l3ai, &l3br, &l3bi);

                const int32_t l2ai_n = (int32_t)(0u - (uint32_t)l2ai);
                const int32_t l2br_n = (int32_t)(0u - (uint32_t)l2br);
                const int32_t l2bi_n = (int32_t)(0u - (uint32_t)l2bi);
                const int32_t l3ai_n = (int32_t)(0u - (uint32_t)l3ai);
                const int32_t l3br_n = (int32_t)(0u - (uint32_t)l3br);
                const int32_t l3bi_n = (int32_t)(0u - (uint32_t)l3bi);

                /* S = U_nu(x+mu) * U_mu(x+nu)^dagger * U_nu(x)^dagger
                 *   t = l2^dagger * l3^dagger
                 *   S = l1 * t */
                int32_t t_ar, t_ai, t_br, t_bi;
                int32_t s_ar, s_ai, s_br, s_bi;
                pro_su2_mul(l2ar, l2ai_n, l2br_n, l2bi_n,
                    l3ar, l3ai_n, l3br_n, l3bi_n,
                    &t_ar, &t_ai, &t_br, &t_bi);
                pro_su2_mul(l1ar, l1ai, l1br, l1bi,
                    t_ar, t_ai, t_br, t_bi,
                    &s_ar, &s_ai, &s_br, &s_bi);

                su2_accumulate_staple(u_ar, u_ai, u_br, u_bi,
                    s_ar, s_ai, s_br, s_bi,
                    &acc_ai, &acc_br, &acc_bi);
                n++;
            }
        }

        /* --- Backward --- */
        if (su2_link_exists(pu, x, nu_rev)) {
            const uint64_t xm_nu = pu->reg_source[x].channels[nu_rev];
            if (xm_nu < pu->total_nodes && su2_link_exists(pu, xm_nu, mu)) {
                const uint64_t xm_nu_mu = pu->reg_source[xm_nu].channels[mu];
                if (xm_nu_mu < pu->total_nodes &&
                    su2_link_exists(pu, xm_nu_mu, nu)) {

                    int32_t m1ar, m1ai, m1br, m1bi; /* U(x-nu, mu) */
                    int32_t m2ar, m2ai, m2br, m2bi; /* U(x-nu+mu, nu) */
                    int32_t m3ar, m3ai, m3br, m3bi; /* U(x-nu, nu) */
                    su2_read_link(pu, xm_nu, mu, &m1ar, &m1ai, &m1br, &m1bi);
                    su2_read_link(pu, xm_nu_mu, nu, &m2ar, &m2ai, &m2br, &m2bi);
                    su2_read_link(pu, xm_nu, nu, &m3ar, &m3ai, &m3br, &m3bi);

                    const int32_t m1ai_n = (int32_t)(0u - (uint32_t)m1ai);
                    const int32_t m1br_n = (int32_t)(0u - (uint32_t)m1br);
                    const int32_t m1bi_n = (int32_t)(0u - (uint32_t)m1bi);
                    const int32_t m2ai_n = (int32_t)(0u - (uint32_t)m2ai);
                    const int32_t m2br_n = (int32_t)(0u - (uint32_t)m2br);
                    const int32_t m2bi_n = (int32_t)(0u - (uint32_t)m2bi);

                    /* S = U_nu(x+mu-nu)^dagger * U_mu(x-nu)^dagger * U_nu(x-nu)
                     *   t = m1^dagger * m3
                     *   S = m2^dagger * t */
                    int32_t t_ar, t_ai, t_br, t_bi;
                    int32_t s_ar, s_ai, s_br, s_bi;
                    pro_su2_mul(m1ar, m1ai_n, m1br_n, m1bi_n,
                        m3ar, m3ai, m3br, m3bi,
                        &t_ar, &t_ai, &t_br, &t_bi);
                    pro_su2_mul(m2ar, m2ai_n, m2br_n, m2bi_n,
                        t_ar, t_ai, t_br, t_bi,
                        &s_ar, &s_ai, &s_br, &s_bi);

                    su2_accumulate_staple(u_ar, u_ai, u_br, u_bi,
                        s_ar, s_ai, s_br, s_bi,
                        &acc_ai, &acc_br, &acc_bi);
                    n++;
                }
            }
        }
    }

    if (n == 0) return;
    *out_ai = pro_sat_i32(acc_ai / n);
    *out_br = pro_sat_i32(acc_br / n);
    *out_bi = pro_sat_i32(acc_bi / n);
}

/* --------------------------------------------------------------------------
 * Plaquette-Action pro 4-Link-Loop
 *
 *   W = l1 * l2 * l3^dagger * l4^dagger
 *   S_plaq(W) = 1 - Re(a_W) / PRO_SU2_SCALE
 *
 * l1 = U(y, alpha),  l2 = U(y+alpha, beta),
 * l3 = U(y+beta, alpha),  l4 = U(y, beta).
 *
 * Rueckgabe 0.0, wenn einer der 4 Links nicht existiert.
 * Ersetzt den Hauptloop in ProPhysics_SU2_Plaquette_Action und die
 * vormals separate su2_plaquette_action_at-Funktion (identische
 * Berechnung, zwei Stellen).
 * -------------------------------------------------------------------------- */
static double su2_plaquette_action_at(
    const ProUniverse* pu, uint64_t y, uint8_t alpha, uint8_t beta)
{
    if (!su2_link_exists(pu, y, alpha)) return 0.0;
    if (!su2_link_exists(pu, y, beta)) return 0.0;

    const uint64_t y_a = pu->reg_source[y].channels[alpha];
    const uint64_t y_b = pu->reg_source[y].channels[beta];
    if (y_a >= pu->total_nodes || y_b >= pu->total_nodes) return 0.0;
    if (!su2_link_exists(pu, y_a, beta)) return 0.0;
    if (!su2_link_exists(pu, y_b, alpha)) return 0.0;

    int32_t l1ar, l1ai, l1br, l1bi;
    int32_t l2ar, l2ai, l2br, l2bi;
    int32_t l3ar, l3ai, l3br, l3bi;
    int32_t l4ar, l4ai, l4br, l4bi;
    su2_read_link(pu, y, alpha, &l1ar, &l1ai, &l1br, &l1bi);
    su2_read_link(pu, y_a, beta, &l2ar, &l2ai, &l2br, &l2bi);
    su2_read_link(pu, y_b, alpha, &l3ar, &l3ai, &l3br, &l3bi);
    su2_read_link(pu, y, beta, &l4ar, &l4ai, &l4br, &l4bi);

    const int32_t l3ai_n = (int32_t)(0u - (uint32_t)l3ai);
    const int32_t l3br_n = (int32_t)(0u - (uint32_t)l3br);
    const int32_t l3bi_n = (int32_t)(0u - (uint32_t)l3bi);
    const int32_t l4ai_n = (int32_t)(0u - (uint32_t)l4ai);
    const int32_t l4br_n = (int32_t)(0u - (uint32_t)l4br);
    const int32_t l4bi_n = (int32_t)(0u - (uint32_t)l4bi);

    int32_t t1ar, t1ai, t1br, t1bi;
    int32_t t2ar, t2ai, t2br, t2bi;
    int32_t w_ar, w_ai, w_br, w_bi;

    pro_su2_mul(l1ar, l1ai, l1br, l1bi,
        l2ar, l2ai, l2br, l2bi,
        &t1ar, &t1ai, &t1br, &t1bi);
    pro_su2_mul(t1ar, t1ai, t1br, t1bi,
        l3ar, l3ai_n, l3br, l3bi_n,
        &t2ar, &t2ai, &t2br, &t2bi);
    pro_su2_mul(t2ar, t2ai, t2br, t2bi,
        l4ar, l4ai_n, l4br, l4bi_n,
        &w_ar, &w_ai, &w_br, &w_bi);

    (void)w_ai; (void)w_br; (void)w_bi;
    return 1.0 - (double)w_ar / (double)PRO_SU2_SCALE;
}

/* --------------------------------------------------------------------------
 * Leapfrog E-Kick
 *
 *   E <- E + half_dt_g2 * F(U)
 *
 * Wird zweimal pro Leapfrog-Schritt aufgerufen (vor und nach dem
 * Link-Update). half_dt_g2 = (dt/2) * g^2 in Q15.
 *
 * Ersetzt die zwei identischen Schleifen in ProPhysics_Apply_SU2_Tick
 * (Schritt 1 und Schritt 3). Die Reihenfolge der Link-Iteration bleibt
 * (k aussen, d innen), damit die Bit-Identitaet zur Vorversion erhalten
 * bleibt.
 * -------------------------------------------------------------------------- */
static void su2_leapfrog_kick_E(ProUniverse* pu, int64_t half_dt_g2)
{
    const uint64_t N = pu->total_nodes;
    for (uint64_t k = 0; k < N; ++k) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = SU2_FWD_CH[d];
            if (!su2_link_exists(pu, k, mu)) continue;

            int32_t f_ai, f_br, f_bi;
            su2_force_on_link(pu, k, mu, &f_ai, &f_br, &f_bi);

            int32_t e_ar, e_ai, e_br, e_bi;
            su2_read_E(pu, k, mu, &e_ar, &e_ai, &e_br, &e_bi);

            const int64_t d_ai = ((int64_t)f_ai * half_dt_g2) >> 15;
            const int64_t d_br = ((int64_t)f_br * half_dt_g2) >> 15;
            const int64_t d_bi = ((int64_t)f_bi * half_dt_g2) >> 15;

            e_ai = pro_sat_i32((int64_t)e_ai + d_ai);
            e_br = pro_sat_i32((int64_t)e_br + d_br);
            e_bi = pro_sat_i32((int64_t)e_bi + d_bi);

            su2_write_E(pu, k, mu, e_ar, e_ai, e_br, e_bi);
        }
    }
}

/* ==========================================================================
 * Oeffentliche API
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Enable_SU2_Dynamics(ProUniverse* pu)
{
    if (!pu) return;
    pu->su2_dynamics_active = 1u;
}

PROPHYSICS_API void ProPhysics_Disable_SU2_Dynamics(ProUniverse* pu)
{
    if (!pu) return;
    pu->su2_dynamics_active = 0u;
}

PROPHYSICS_API int ProPhysics_Is_SU2_Dynamics_Active(const ProUniverse* pu)
{
    return pu ? (int)pu->su2_dynamics_active : 0;
}

PROPHYSICS_API void ProPhysics_Set_SU2_Yang_Mills(ProUniverse* pu, int32_t g2_q15)
{
    if (!pu) return;
    pu->su2_yang_mills_q15 = g2_q15;
}

/* ==========================================================================
 * Leapfrog-Tick
 * ========================================================================== */

PROPHYSICS_API void ProPhysics_Apply_SU2_Tick(ProUniverse* pu, uint32_t dt_q15)
{
    if (!pu || !pu->edge_phases) return;
    if (!pu->su2_dynamics_active) return;
    if (dt_q15 == 0u) return;

    const int32_t g2_q15 = pu->su2_yang_mills_q15;
    if (g2_q15 == 0) return;

    /* half_dt_g2 = (dt/2) * g^2 in Q15.
     * = (dt_q15 * g2_q15) >> 16. */
    const int64_t half_dt_g2 = ((int64_t)dt_q15 * (int64_t)g2_q15) >> 16;

    const uint64_t N = pu->total_nodes;

    /* --- Schritt 1: E += (dt/2) * g^2 * F(U) --- */
    su2_leapfrog_kick_E(pu, half_dt_g2);

    /* --- Schritt 2: U = exp(i*dt*E) * U --- */
    for (uint64_t k = 0; k < N; ++k) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = SU2_FWD_CH[d];
            if (!su2_link_exists(pu, k, mu)) continue;

            int32_t u_ar, u_ai, u_br, u_bi;
            su2_read_link(pu, k, mu, &u_ar, &u_ai, &u_br, &u_bi);

            int32_t e_ar, e_ai, e_br, e_bi;
            su2_read_E(pu, k, mu, &e_ar, &e_ai, &e_br, &e_bi);

            int32_t n_ar, n_ai, n_br, n_bi;
            pro_su2_exp_apply(u_ar, u_ai, u_br, u_bi,
                e_ar, e_ai, e_br, e_bi,
                dt_q15,
                &n_ar, &n_ai, &n_br, &n_bi);

            su2_write_link(pu, k, mu, n_ar, n_ai, n_br, n_bi);
        }
    }

    /* --- Schritt 3: E += (dt/2) * g^2 * F(U_neu) --- */
    su2_leapfrog_kick_E(pu, half_dt_g2);
}

/* ==========================================================================
 * Plaquette-Action
 *
 * S_plaq = Sum_plaq (1 - 0.5 * Re Tr(W_plaq))
 *        = Sum_plaq (1 - Re(a_plaq) / 2^30)
 * mit a_plaq = Realteil des Quaternions a von W.
 * ========================================================================== */

PROPHYSICS_API double ProPhysics_SU2_Plaquette_Action(const ProUniverse* pu)
{
    if (!pu || !pu->edge_phases) return 0.0;

    double sum = 0.0;
    const uint64_t N = pu->total_nodes;

    for (uint64_t k = 0; k < N; ++k) {
        for (int mi = 0; mi < 3; ++mi) {
            const uint8_t mu = SU2_FWD_CH[mi];
            if (!su2_link_exists(pu, k, mu)) continue;

            for (int ni = mi + 1; ni < 3; ++ni) {
                const uint8_t nu = SU2_FWD_CH[ni];
                /* su2_plaquette_action_at prueft alle vier Links
                 * selbst; die mu-Link-Pruefung bleibt als schneller
                 * Skip fuer Knoten ohne +x/+y/+z-Kante. */
                sum += su2_plaquette_action_at(pu, k, mu, nu);
            }
        }
    }
    return sum;
}

PROPHYSICS_API double ProPhysics_SU2_Total_Energy(const ProUniverse* pu)
{
    if (!pu || !pu->edge_phases) return 0.0;

    double e_sum = 0.0;
    const uint64_t N = pu->total_nodes;
    const double inv_scale_sq =
        1.0 / ((double)PRO_SU2_SCALE * (double)PRO_SU2_SCALE);

    for (uint64_t k = 0; k < N; ++k) {
        for (int d = 0; d < 3; ++d) {
            const uint8_t mu = SU2_FWD_CH[d];
            if (!su2_link_exists(pu, k, mu)) continue;

            int32_t e_ar, e_ai, e_br, e_bi;
            su2_read_E(pu, k, mu, &e_ar, &e_ai, &e_br, &e_bi);
            (void)e_ar;

            const double sq = (double)e_ai * (double)e_ai
                + (double)e_br * (double)e_br
                + (double)e_bi * (double)e_bi;
            e_sum += sq * inv_scale_sq;
        }
    }

    const double S = ProPhysics_SU2_Plaquette_Action(pu);
    return S + 0.5 * e_sum;
}

/* ==========================================================================
 * Lokale Plaquette-Summe um einen einzelnen Link.
 *
 * Fuer einen Link (x, mu) summiert diese Funktion die Wilson-Aktion
 * aller Plaquettes, die diesen Link enthalten. In 3D sind das 4:
 *   fuer jede Richtung nu != mu:
 *     - W_{alpha,beta}(x)          mit {alpha,beta} = sort({mu,nu})
 *     - W_{alpha,beta}(x - e_nu)
 *
 * Nutzen: Metropolis / HMC brauchen den Delta-S-Beitrag eines einzelnen
 * Link-Updates, nicht die globale Summe. Damit ist
 * dS = Link_Plaquette_Sum (neu) - Link_Plaquette_Sum (alt)
 * ein einziger Aufruf pro Update.
 *
 * Read-only. Veraendert keinen Zustand.
 * ========================================================================== */

PROPHYSICS_API double ProPhysics_SU2_Link_Plaquette_Sum(
    const ProUniverse* pu, uint64_t x, uint8_t mu)
{
    if (!pu || !pu->edge_phases || !pu->reg_source) return 0.0;
    if (x >= pu->total_nodes) return 0.0;
    if (mu >= (uint8_t)CHANNELS_MAX) return 0.0;
    if (!su2_link_exists(pu, x, mu)) return 0.0;

    double sum = 0.0;

    for (int i = 0; i < 3; ++i) {
        const uint8_t nu = SU2_FWD_CH[i];
        if (nu == mu) continue;

        const uint8_t alpha = (mu < nu) ? mu : nu;
        const uint8_t beta = (mu < nu) ? nu : mu;

        sum += su2_plaquette_action_at(pu, x, alpha, beta);

        const uint8_t nu_rev = SU2_REV_CH[i];
        if (su2_link_exists(pu, x, nu_rev)) {
            const uint64_t xm_nu = pu->reg_source[x].channels[nu_rev];
            if (xm_nu < pu->total_nodes) {
                sum += su2_plaquette_action_at(pu, xm_nu, alpha, beta);
            }
        }
    }

    return sum;
}

/* ==========================================================================
 * End of ProPhysics_SU2_Dynamics.c
 * ========================================================================== */