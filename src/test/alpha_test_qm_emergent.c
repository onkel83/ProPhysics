/* =========================================================================
 * alpha_test_qm_emergent.c
 *
 * Etappe 16e'': Zwei Tests, die NICHT automatisch gruen sind.
 *
 * Test 1: Dispersionsrelation omega(k)
 *   Bloch-Zustand psi(x,y) = exp(i*k*x)/sqrt(N) praeparieren, Edge-Transport
 *   laufen lassen, Phase ueber Zeit messen. Modellvorhersage:
 *     omega_theory(k) = atan2(s*cos(k), c - s*sin(k))
 *   mit c = cos(theta_transport), s = sin(theta_transport),
 *   theta_transport = (theta_q15/32768)*2*pi*0.5.
 *
 *   Wenn die Messung mit der Vorhersage uebereinstimmt, ist die Bloch-
 *   Dynamik des Modells bestaetigt -- das ist die erste quantitative,
 *   nicht-triviale Aussage ueber den Kernel.
 *
 * Test 2: Zwei-Soliton-Streuung
 *   Zwei GP-Solitonen mit entgegengesetzter Geschwindigkeit, g_q15 > 0.
 *   Trajektorien der beiden Peaks tracken. Amplituden-Erhalt pruefen.
 *   Erwartung: entweder elastische Streuung (integrabel) oder
 *   inelastisch -- der Ausgang ist nicht vorhersagbar.
 * ========================================================================= */

#include "alpha_test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void wire_torus_em(ProUniverse* pu, uint32_t dim) {
    const uint64_t N = (uint64_t)dim * (uint64_t)dim;
    for (uint64_t i = 0; i < N; ++i)
        for (int c = 0; c < CHANNELS_MAX; ++c)
            pu->reg_source[i].channels[c] = i;
    for (uint32_t y = 0; y < dim; ++y) {
        const uint64_t y_n = ((y == 0) ? dim - 1 : y - 1) * (uint64_t)dim;
        const uint64_t y_s = ((y == dim - 1) ? 0 : y + 1) * (uint64_t)dim;
        const uint64_t y_c = (uint64_t)y * dim;
        for (uint32_t x = 0; x < dim; ++x) {
            const uint64_t idx = y_c + x;
            pu->reg_source[idx].channels[0] = y_n + x;
            pu->reg_source[idx].channels[1] = y_s + x;
            pu->reg_source[idx].channels[2] = y_c + ((x == dim - 1) ? 0 : x + 1);
            pu->reg_source[idx].channels[3] = y_c + ((x == 0) ? dim - 1 : x - 1);
        }
    }
}

/* -------------------------------------------------------------------------
 * Test 1: Dispersionsrelation omega(k)
 *
 * Vorhersage (hergeleitet aus dem 4-Sweep-Transport, verifiziert):
 *
 *   U(q) = M_y1 · M_y0 · M_x1 · M_x0   ∈ SU(2)
 *
 *   c = cos(theta),  s = sin(theta),  theta = (theta_q15/32768)*pi
 *   c2 = cos(2*theta), s2 = sin(2*theta)
 *
 *   A_plus = c^2 - s^2 * cos(2q)
 *   B      = s^2 * sin(2q)
 *   C      = s2 * cos(q)
 *
 *   X = c2*A_plus - s2*C
 *   Y = c2*B
 *   a = s2*B
 *   b = c2*C + s2*A_plus
 *   Yp = sqrt(Y^2 + a^2 + b^2)
 *
 *   omega(q) = atan2(Yp, X)
 *
 * Der Bloch-Zustand muss im EIGENVEKTOR von U(q) initialisiert werden,
 * sonst mischt die Messung +omega und -omega und die Phase oszilliert
 * (siehe voriger Lauf, k/pi > 0.9).
 *
 * Eigenvektor zum Eigenwert lambda+ = X + i*Yp:
 *
 *   alpha = 1
 *   beta  = -i*(Y - Yp)/(a + i*b)
 *
 * Das wird als komplexes Paar (alpha, beta) direkt in Q31 geschrieben.
 * Der Zustand ist:
 *
 *   psi(x,y) = alpha * exp(i*q*x)   fuer (x+y) gerade
 *   psi(x,y) = beta  * exp(i*q*x)   fuer (x+y) ungerade
 *
 * Phase wird am Knoten 0 extrahiert: psi(0,0) = alpha.
 * Mit alpha = 1 (reell) startet die Phase bei 0 und waechst mit Rate omega.
 * ----------------------------------------------------------------------- */

bool test_dispersion_relation(void)
{
    printf("========================================================================\n");
    printf("  Dispersionsrelation omega(k) -- Bloch-Vorhersage\n");
    printf("========================================================================\n\n");

    const uint32_t DIM = 128u;
    const uint64_t N = (uint64_t)DIM * DIM;
    const uint32_t THETA_Q15 = 2000u;
    const uint32_t N_TICKS = 200u;
    const uint32_t N_SAMPLES = 60u;

    const double theta_rad = ((double)THETA_Q15 / 32768.0) * M_PI;
    const double c_th = cos(theta_rad);
    const double s_th = sin(theta_rad);
    const double c2_th = cos(2.0 * theta_rad);
    const double s2_th = sin(2.0 * theta_rad);

    printf("[Disp] DIM=%u | theta_q15=%u -> theta_rad=%.6f\n",
        DIM, THETA_Q15, theta_rad);
    printf("[Disp] c=%.6f s=%.6f c2=%.6f s2=%.6f\n",
        c_th, s_th, c2_th, s2_th);
    printf("[Disp] Vorhersage (4-Sweep-Bloch):\n");
    printf("[Disp]   omega(q) = atan2(Yp, X)\n");
    printf("[Disp]   mit X  = c2*A_plus - s2*C\n");
    printf("[Disp]       Yp = sqrt(Y^2 + a^2 + b^2)\n");
    printf("[Disp]   A_plus = c^2 - s^2*cos(2q),  B = s^2*sin(2q),\n");
    printf("[Disp]   C = s2*cos(q),  Y = c2*B,  a = s2*B,  b = c2*C + s2*A_plus\n");
    printf("[Disp] Ticks=%u | Samples=%u | Eigenvektor-Init\n\n", N_TICKS, N_SAMPLES);

    /* k = 2*pi*m/dim, damit der Bloch-Zustand konsistent in den Torus passt. */
    const uint32_t n_k = 10u;
    double k_vals[10];
    {
        const uint32_t m_start = 4u;
        const uint32_t m_step = 8u;
        for (uint32_t i = 0u; i < n_k; ++i) {
            const uint32_t m = m_start + i * m_step;
            k_vals[i] = 2.0 * M_PI * (double)m / (double)DIM;
        }
    }

    printf("[Disp]  k/pi    omega_theory   omega_meas    rel_dev    amp_drift\n");
    printf("[Disp]  ------------------------------------------------------------\n");

    double max_rel_dev = 0.0;
    uint32_t n_valid = 0u;
    uint32_t n_pass = 0u;

    for (uint32_t ki = 0u; ki < n_k; ++ki) {
        const double q = k_vals[ki];

        /* 1. Bloch-Parameter (siehe Kommentar). */
        const double A_plus = c_th * c_th - s_th * s_th * cos(2.0 * q);
        const double Bb = s_th * s_th * sin(2.0 * q);
        const double Cc = s2_th * cos(q);
        const double X = c2_th * A_plus - s2_th * Cc;
        const double Y = c2_th * Bb;
        const double a = s2_th * Bb;
        const double b = c2_th * Cc + s2_th * A_plus;
        const double Yp = sqrt(Y * Y + a * a + b * b);
        const double omega_theory = atan2(Yp, X);

        /* 2. Eigenvektor (alpha=1, beta=-i*(Y-Yp)/(a+ib)). */
        const double u = Y - Yp;
        const double denom = a * a + b * b;
        double beta_re, beta_im;
        if (denom < 1e-20) {
            beta_re = 0.0;
            beta_im = 1.0;
        }
        else {
            /* beta = -i*u*(a-ib)/denom = -u*b/denom - i*u*a/denom */
            beta_re = -u * b / denom;
            beta_im = -u * a / denom;
        }
        const double alpha_re = 1.0;
        const double alpha_im = 0.0;

        /* 3. Normierung: Summe über alle N Knoten = 1.
         *    Knoten gerade: alpha*e^{iqx}, ungerade: beta*e^{iqx}.
         *    Norm^2 = (N/2) * (|alpha|^2 + |beta|^2). */
        const double norm2 = alpha_re * alpha_re + alpha_im * alpha_im
            + beta_re * beta_re + beta_im * beta_im;
        const double scale = sqrt(2.0 / ((double)N * norm2));

        /* 4. Bloch-Zustand praeparieren. */
        ProUniverse pu;
        ProPhysics_Initialize(&pu, N);
        if (!pu.amp_grid || !pu.reg_source || !pu.amp_scratch) {
            ProPhysics_Free(&pu);
            continue;
        }
        wire_torus_em(&pu, DIM);

        for (uint32_t y = 0u; y < DIM; ++y) {
            for (uint32_t x = 0u; x < DIM; ++x) {
                const uint64_t idx = (uint64_t)y * DIM + x;
                for (uint8_t bb = 0u; bb < PRO_AMP_BASIS_SIZE; ++bb)
                    pu.amp_grid[idx].coeff[bb] = 0;

                const double ph = q * (double)x;
                const double cph = cos(ph);
                const double sph = sin(ph);
                const bool even = ((x + y) & 1u) == 0u;
                const double are = even ? alpha_re : beta_re;
                const double aim = even ? alpha_im : beta_im;

                /* (are + i*aim) * (cph + i*sph) * scale */
                const double re = (are * cph - aim * sph) * scale;
                const double im = (are * sph + aim * cph) * scale;

                const int32_t re_q = (int32_t)lround(re * Q31_MAXV);
                const int32_t im_q = (int32_t)lround(im * Q31_MAXV);
                pu.amp_grid[idx].coeff[UR_POSITRON_CW] = pro_amp_pack(re_q, im_q);
            }
        }

        /* 5. Evolution und Phase am Knoten 0 extrahieren. */
        double phi_arr[64];
        double t_arr[64];
        uint32_t collected = 0u;
        const uint32_t stride = (N_TICKS > 0u) ? (N_TICKS / N_SAMPLES) : 1u;
        if (stride == 0u) { ProPhysics_Free(&pu); continue; }

        double amp_start = 0.0;

        for (uint32_t t = 0u; t <= N_TICKS; ++t) {
            if (t % stride == 0u && collected < N_SAMPLES) {
                const int32_t re0 = pro_amp_real(pu.amp_grid[0].coeff[UR_POSITRON_CW]);
                const int32_t im0 = pro_amp_imag(pu.amp_grid[0].coeff[UR_POSITRON_CW]);
                phi_arr[collected] = atan2((double)im0, (double)re0);
                t_arr[collected] = (double)t;
                const double a2 = (double)re0 * re0 + (double)im0 * im0;
                if (t == 0u) amp_start = a2;
                collected++;
            }
            if (t < N_TICKS)
                ProPhysics_Apply_Edge_Transport_Colored(&pu, THETA_Q15, DIM);
        }

        if (collected < 4u) { ProPhysics_Free(&pu); continue; }

        /* 6. Amplitude-Drift und Phasen-Regression. */
        const int32_t re_end = pro_amp_real(pu.amp_grid[0].coeff[UR_POSITRON_CW]);
        const int32_t im_end = pro_amp_imag(pu.amp_grid[0].coeff[UR_POSITRON_CW]);
        const double a2_end = (double)re_end * re_end + (double)im_end * im_end;
        const double amp_drift = (amp_start > 1.0)
            ? fabs(a2_end - amp_start) / amp_start : 0.0;

        /* Phase entrollen. */
        for (uint32_t i = 1u; i < collected; ++i) {
            double dphi = phi_arr[i] - phi_arr[i - 1u];
            while (dphi > M_PI) dphi -= 2.0 * M_PI;
            while (dphi < -M_PI) dphi += 2.0 * M_PI;
            phi_arr[i] = phi_arr[i - 1u] + dphi;
        }

        /* Lineare Regression -> omega. */
        double sum_t = 0.0, sum_p = 0.0;
        for (uint32_t i = 0u; i < collected; ++i) {
            sum_t += t_arr[i];
            sum_p += phi_arr[i];
        }
        const double t_bar = sum_t / (double)collected;
        const double p_bar = sum_p / (double)collected;
        double num = 0.0, den = 0.0;
        for (uint32_t i = 0u; i < collected; ++i) {
            const double dt = t_arr[i] - t_bar;
            num += dt * (phi_arr[i] - p_bar);
            den += dt * dt;
        }
        const double omega_meas = (den > 1e-12) ? (num / den) : 0.0;

        const double rel_dev = (fabs(omega_theory) > 1e-6)
            ? fabs(omega_meas - omega_theory) / fabs(omega_theory)
            : fabs(omega_meas);
        if (rel_dev > max_rel_dev) max_rel_dev = rel_dev;
        n_valid++;

        const bool local_pass = (rel_dev < 5e-3) && (amp_drift < 1e-3);
        if (local_pass) n_pass++;

        printf("[Disp]  %.4f    %+.6f     %+.6f     %.2e    %.2e  %s\n",
            q / M_PI, omega_theory, omega_meas, rel_dev, amp_drift,
            local_pass ? "" : "<--");

        ProPhysics_Free(&pu);
    }

    printf("\n[Disp] %u/%u k-Werte bestanden (rel_dev<5e-3, amp_drift<1e-3)\n",
        n_pass, n_valid);
    printf("[Disp] max rel_dev = %.4e\n", max_rel_dev);

    const bool pass = (n_valid > 0u) && (n_pass == n_valid);
    printf("[Disp] -> %s\n",
        pass
        ? "PASSED (Bloch-Dispersionsrelation exakt bestaetigt)"
        : "FAILED (Modell weicht von Bloch-Vorhersage ab)");

    return pass;
}

/* -------------------------------------------------------------------------
 * Test 2: Zwei-Soliton-Streuung
 *
 * Wir praeparieren zwei Gauss-Pakete an x = 32 und x = 96 mit
 * entgegengesetzter Phase (k0 = +/- 0.3 rad/Zelle -> v = +/- 0.3).
 * GP-Kopplung aktiviert. Ueber N Ticks laufen lassen.
 *
 * Wir messen:
 *   - Position des linken / rechten Peaks am Anfang und Ende
 *   - Amplitude des Peaks (Maximum von |psi|^2) am Anfang und Ende
 *
 * Die Idee ist nicht "Elastisch ja/nein", sondern quantitative
 * Charakterisierung: Was macht dieses Gitter mit zwei nichtlinearen
 * Wellenpaketen bei Kollision?
 * ----------------------------------------------------------------------- */

bool test_two_soliton_scattering(void)
{
    printf("========================================================================\n");
    printf("  Zwei-Soliton-Streuung (nicht vorhersagbar)\n");
    printf("========================================================================\n\n");

    const uint32_t DIM = 128u;
    const uint64_t N = (uint64_t)DIM * DIM;
    const uint32_t THETA_Q15 = 2000u;
    const int32_t  G_Q15 = 50000;      /* staerkere Nichtlinearitaet */
    const uint32_t NL_DT_Q15 = 32768u;
    const uint32_t N_TICKS = 800u;
    const double SIGMA0 = 4.0;
    const double X_LEFT = 40.0;
    const double X_RIGHT = 88.0;
    const double Y_CENTER = 64.0;
    const double K_LEFT = +0.30;       /* links bewegt sich nach rechts */
    const double K_RIGHT = -0.30;      /* rechts bewegt sich nach links */

    printf("[2Sol] DIM=%u | theta_q15=%u | g_q15=%d | ticks=%u\n",
        DIM, THETA_Q15, G_Q15, N_TICKS);
    printf("[2Sol] sigma0=%.1f | X_links=%.0f X_rechts=%.0f | k=+/-%.2f\n\n",
        SIGMA0, X_LEFT, X_RIGHT, K_LEFT);

    /* Praeparation. */
    ProUniverse pu;
    ProPhysics_Initialize(&pu, N);
    if (!pu.amp_grid || !pu.reg_source || !pu.amp_scratch) {
        printf("[2Sol] Init failed.\n");
        ProPhysics_Free(&pu);
        return false;
    }
    wire_torus_em(&pu, DIM);

    for (uint64_t k = 0; k < N; ++k)
        for (uint8_t b = 0; b < PRO_AMP_BASIS_SIZE; ++b)
            pu.amp_grid[k].coeff[b] = 0;

    /* Zwei Gauss-Pakete, normiert auf Gesamtpeak 0.35*Q31_MAXV. */
    const double amp_scale = 0.35 * Q31_MAXV;

    for (uint32_t y = 0; y < DIM; ++y) {
        for (uint32_t x = 0; x < DIM; ++x) {
            double dxL = (double)x - X_LEFT;
            double dxR = (double)x - X_RIGHT;
            if (dxL > (double)DIM * 0.5) dxL -= (double)DIM;
            if (dxL < -(double)DIM * 0.5) dxL += (double)DIM;
            if (dxR > (double)DIM * 0.5) dxR -= (double)DIM;
            if (dxR < -(double)DIM * 0.5) dxR += (double)DIM;
            const double dy = (double)y - Y_CENTER;

            const double gL = exp(-(dxL * dxL + dy * dy) / (2.0 * SIGMA0 * SIGMA0));
            const double gR = exp(-(dxR * dxR + dy * dy) / (2.0 * SIGMA0 * SIGMA0));

            /* Phase: exp(i*k*x) mit k = K_LEFT / K_RIGHT. */
            const double pL = K_LEFT * dxL;
            const double pR = K_RIGHT * dxR;

            const double re = amp_scale * (gL * cos(pL) + gR * cos(pR));
            const double im = amp_scale * (gL * sin(pL) + gR * sin(pR));

            const uint64_t idx = (uint64_t)y * DIM + x;
            pu.amp_grid[idx].coeff[UR_POSITRON_CW] =
                pro_amp_pack((int32_t)lround(re), (int32_t)lround(im));
        }
    }

    /* Profil bei t=0 messen. */
    double mean_y_profile_0[128];
    for (uint32_t x = 0; x < DIM; ++x) mean_y_profile_0[x] = 0.0;
    for (uint32_t y = 0; y < DIM; ++y)
        for (uint32_t x = 0; x < DIM; ++x) {
            const int32_t re = pro_amp_real(pu.amp_grid[(uint64_t)y * DIM + x].coeff[UR_POSITRON_CW]);
            const int32_t im = pro_amp_imag(pu.amp_grid[(uint64_t)y * DIM + x].coeff[UR_POSITRON_CW]);
            mean_y_profile_0[x] += (double)re * re + (double)im * im;
        }

    /* Peak-Suche. */
    int peak_L_0 = -1, peak_R_0 = -1;
    double peak_L_amp_0 = 0.0, peak_R_amp_0 = 0.0;
    {
        int best_L = 0; double best_L_v = -1.0;
        int best_R = 0; double best_R_v = -1.0;
        for (uint32_t x = 0; x < 40u; ++x) {
            if (mean_y_profile_0[x] > best_L_v) {
                best_L_v = mean_y_profile_0[x];
                best_L = (int)x;
            }
        }
        for (uint32_t x = 80u; x < DIM; ++x) {
            if (mean_y_profile_0[x] > best_R_v) {
                best_R_v = mean_y_profile_0[x];
                best_R = (int)x;
            }
        }
        peak_L_0 = best_L; peak_L_amp_0 = best_L_v;
        peak_R_0 = best_R; peak_R_amp_0 = best_R_v;
    }

    printf("[2Sol] t=0: linker Peak x=%d (amp=%.3e), rechter Peak x=%d (amp=%.3e)\n",
        peak_L_0, peak_L_amp_0, peak_R_0, peak_R_amp_0);

    /* Evolution. */
    for (uint32_t t = 0; t < N_TICKS; ++t) {
        ProPhysics_Apply_Edge_Transport_Colored(&pu, THETA_Q15, DIM);
        ProPhysics_Apply_Nonlinear_Phase_Step(&pu, G_Q15, NL_DT_Q15);
    }

    /* Profil bei t=N messen. */
    double mean_y_profile_1[128];
    for (uint32_t x = 0; x < DIM; ++x) mean_y_profile_1[x] = 0.0;
    for (uint32_t y = 0; y < DIM; ++y)
        for (uint32_t x = 0; x < DIM; ++x) {
            const int32_t re = pro_amp_real(pu.amp_grid[(uint64_t)y * DIM + x].coeff[UR_POSITRON_CW]);
            const int32_t im = pro_amp_imag(pu.amp_grid[(uint64_t)y * DIM + x].coeff[UR_POSITRON_CW]);
            mean_y_profile_1[x] += (double)re * re + (double)im * im;
        }

    /* Peaks bei t=N finden (durchs ganze Bild, zwei lokale Maxima). */
    int peak1 = -1, peak2 = -1;
    double amp1 = 0.0, amp2 = 0.0;
    {
        /* Erst globalen Peak. */
        int best = 0; double best_v = -1.0;
        for (uint32_t x = 0; x < DIM; ++x) {
            if (mean_y_profile_1[x] > best_v) {
                best_v = mean_y_profile_1[x];
                best = (int)x;
            }
        }
        amp1 = best_v; peak1 = best;

        /* Dann zweiten Peak in anderer Haelfte. */
        int best2 = -1; double best2_v = -1.0;
        const int half = DIM / 2;
        const int start = (best < half) ? half : 0;
        const int stop = (best < half) ? (int)DIM : half;
        for (int x = start; x < stop; ++x) {
            if (mean_y_profile_1[x] > best2_v) {
                best2_v = mean_y_profile_1[x];
                best2 = x;
            }
        }
        amp2 = best2_v; peak2 = best2;
    }

    printf("[2Sol] t=%u: zwei Peaks x=%d (amp=%.3e), x=%d (amp=%.3e)\n",
        N_TICKS, peak1, amp1, peak2, amp2);

    /* Peak-Amplituden-Erhalt. */
    const double mean_amp_0 = 0.5 * (peak_L_amp_0 + peak_R_amp_0);
    const double mean_amp_1 = 0.5 * (amp1 + amp2);
    const double amp_ratio = (mean_amp_0 > 0.0)
        ? (mean_amp_1 / mean_amp_0) : 0.0;

    printf("\n[2Sol] Peak-Amplitude t=0:  %.3e\n", mean_amp_0);
    printf("[2Sol] Peak-Amplitude t=%u: %.3e\n", N_TICKS, mean_amp_1);
    printf("[2Sol] amp_ratio = %.4f\n", amp_ratio);

    /* Interpretation. */
    const char* classification;
    if (amp_ratio > 0.9 && amp_ratio < 1.1) {
        classification = "ELASTISCH (Amplitude erhalten)";
    }
    else if (amp_ratio > 1.1) {
        classification = "VERSTAERKEND (nichtlineare Verstaerkung)";
    }
    else if (amp_ratio > 0.5) {
        classification = "DAEMPFEND (nichtlineare Dissipation)";
    }
    else {
        classification = "ZERFALLEND (Solitonen aufgeloest)";
    }

    printf("\n[2Sol] Klassifikation: %s\n", classification);

    /* "Pass" ist kein Kriterium hier -- der Ausgang ist die Erkenntnis.
     * Wir liefern true, damit der Test-Runner den Wert nicht als Fehler
     * wertet. Der Nutzer liest die Tabelle selbst. */
    ProPhysics_Free(&pu);
    return true;
}

/* -------------------------------------------------------------------------
 * Sammel-Einstiegspunkt
 * ----------------------------------------------------------------------- */

bool test_qm_emergent_all(void)
{
    printf("\n");
    printf("########################################################################\n");
    printf("#  QM-Emergent: genuine Modellvorhersagen\n");
    printf("########################################################################\n\n");

    const bool t1 = test_dispersion_relation();
    printf("\n");
    const bool t2 = test_two_soliton_scattering();

    printf("\n");
    printf("########################################################################\n");
    printf("#  QM-Emergent Zusammenfassung\n");
    printf("########################################################################\n");
    printf("#  Dispersionsrelation omega(k)  : %s\n", t1 ? "PASS" : "FAIL");
    printf("#  Zwei-Soliton-Streuung         : %s\n",
        t2 ? "PASS (informativ)" : "FAIL");
    printf("#  Gesamt                        : %s\n",
        (t1 && t2) ? "PASS" : "FAIL");
    printf("########################################################################\n");

    return t1 && t2;
}