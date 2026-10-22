#pragma once
#include <cmath>
#include <vector>
#include <iostream>

const double PI = 3.14159265358979323846;
const double MU = 4.903e12;
const double RM = 1738000.0;
const double TV = 14.0;
const double DT = 0.1;
const double MKONSTR = 615.0;

struct State {
    double x, y, Vx, Vy, m;
};

struct Params {
    double W;
    double P1;
    double P2;
    double h;
    double t1, t2;
    double theta_dot1, theta_2;
    double m0;
};

inline double getTheta(double t, const Params& p) {
    if (t <= TV) return PI / 2.0;
    if (t <= p.t1) return PI / 2.0 + p.theta_dot1 * (t - TV);
    if (t < p.t2) return 0.0;
    return p.theta_2;
}

inline double getThrust(double t, const Params& p) {
    if (t <= p.t1) return p.P1;
    if (t < p.t2) return 0.0;
    return p.P2;
}

inline State derivatives(const State& s, double t, const Params& p) {
    double theta = getTheta(t, p);
    double thrust = getThrust(t, p);
    double r = std::sqrt(s.x * s.x + (RM + s.y) * (RM + s.y));
    if (r < RM) r = RM;

    double gx = -MU * s.x / std::pow(r, 3);
    double gy = -MU * (RM + s.y) / std::pow(r, 3);
    return {
        s.Vx,
        s.Vy,
        (thrust / s.m) * std::cos(theta) + gx,
        (thrust / s.m) * std::sin(theta) + gy,
        -thrust / p.W
    };
}

inline void RK4Step(State& s, double& t, double dt, const Params& p) {
    State k1 = derivatives(s, t, p);
    State s2 = { s.x + 0.5 * dt * k1.x, s.y + 0.5 * dt * k1.y, s.Vx + 0.5 * dt * k1.Vx, s.Vy + 0.5 * dt * k1.Vy, s.m + 0.5 * dt * k1.m };
    State k2 = derivatives(s2, t + 0.5 * dt, p);
    State s3 = { s.x + 0.5 * dt * k2.x, s.y + 0.5 * dt * k2.y, s.Vx + 0.5 * dt * k2.Vx, s.Vy + 0.5 * dt * k2.Vy, s.m + 0.5 * dt * k2.m };
    State k3 = derivatives(s3, t + 0.5 * dt, p);
    State s4 = { s.x + dt * k3.x, s.y + dt * k3.y, s.Vx + dt * k3.Vx, s.Vy + dt * k3.Vy, s.m + dt * k3.m };
    State k4 = derivatives(s4, t + dt, p);

    s.x += dt * (k1.x + 2 * k2.x + 2 * k3.x + k4.x) / 6.0;
    s.y += dt * (k1.y + 2 * k2.y + 2 * k3.y + k4.y) / 6.0;
    s.Vx += dt * (k1.Vx + 2 * k2.Vx + 2 * k3.Vx + k4.Vx) / 6.0;
    s.Vy += dt * (k1.Vy + 2 * k2.Vy + 2 * k3.Vy + k4.Vy) / 6.0;
    s.m += dt * (k1.m + 2 * k2.m + 2 * k3.m + k4.m) / 6.0;
    t += dt;
}

inline double integrate(const Params& p, State& s, double& t_end, std::vector<State>* trajectory = nullptr, std::vector<double>* time_history = nullptr) {
    double t = 0.0;
    double Vk = std::sqrt(MU / (RM + p.h));
    double V = std::sqrt(s.Vx * s.Vx + s.Vy * s.Vy);

    if (trajectory) {
        trajectory->clear();
        if (time_history) time_history->clear();
        trajectory->push_back(s);
        if (time_history) time_history->push_back(t);
    }

    double dt = DT;
    bool rhythmic_step = false;

    while (V < Vk && t < 2000.0) {
        double dt_current = dt;

        if (t < p.t1 && t + dt_current > p.t1) {
            dt_current = p.t1 - t;
            rhythmic_step = true;
        }
        else if (t < p.t2 && t + dt_current > p.t2) {
            dt_current = p.t2 - t;
            rhythmic_step = true;
        }

        State s_prev = s;
        double t_prev = t;

        RK4Step(s, t, dt_current, p);
        V = std::sqrt(s.Vx * s.Vx + s.Vy * s.Vy);

        if (V >= Vk) {
            double V_prev = std::sqrt(s_prev.Vx * s_prev.Vx + s_prev.Vy * s_prev.Vy);
            double denom = V - V_prev;
            double alpha = (denom > 1e-12) ? (Vk - V_prev) / denom : 1.0;

            s.x = s_prev.x + alpha * (s.x - s_prev.x);
            s.y = s_prev.y + alpha * (s.y - s_prev.y);
            s.Vx = s_prev.Vx + alpha * (s.Vx - s_prev.Vx);
            s.Vy = s_prev.Vy + alpha * (s.Vy - s_prev.Vy);
            s.m = s_prev.m + alpha * (s.m - s_prev.m);
            t = t_prev + alpha * (t - t_prev);
            V = Vk;
        }

        if (trajectory) {
            trajectory->push_back(s);
            if (time_history) time_history->push_back(t);
        }

        if (rhythmic_step) {
            dt = DT - dt_current;
            rhythmic_step = false;
        }
        else {
            dt = DT;
        }
    }
    t_end = t;
    return p.m0 - s.m;
}

inline void residuals(const State& s, double h, double& dr, double& dtheta) {
    double r = std::sqrt(s.x * s.x + (RM + s.y) * (RM + s.y));
    double V = std::sqrt(s.Vx * s.Vx + s.Vy * s.Vy);

    if (V < 1e-3) {
        dr = 1e6;
        dtheta = 1e6;
        return;
    }

    dtheta = std::asin((s.x * s.Vx + (RM + s.y) * s.Vy) / (r * V));
    dr = r - (RM + h);
}

inline double calcMass(const Params& p) {
    State s = { 0, 0, 0, 0, p.m0 };
    double t_end;
    return integrate(p, s, t_end);
}

inline void solveBVP(Params& p, int max_iters = 50) {
    double eps_r = 1e-3;
    double eps_theta = 1e-5;
    double step = 1e-5;

    for (int iter = 0; iter < max_iters; ++iter) {
        State s0 = { 0, 0, 0, 0, p.m0 };
        double t_end0;
        integrate(p, s0, t_end0);
        double dr0, dtheta0;
        residuals(s0, p.h, dr0, dtheta0);

        if (std::abs(dr0) < eps_r && std::abs(dtheta0) < eps_theta) break;
        if (std::isnan(dr0) || std::isnan(dtheta0)) break;

        Params p1 = p; p1.theta_dot1 += step;
        State s1 = { 0, 0, 0, 0, p.m0 };
        double t_end1;
        integrate(p1, s1, t_end1);
        double dr1, dtheta1;
        residuals(s1, p.h, dr1, dtheta1);

        Params p2 = p; p2.theta_2 += step;
        State s2 = { 0, 0, 0, 0, p.m0 };
        double t_end2;
        integrate(p2, s2, t_end2);
        double dr2, dtheta2;
        residuals(s2, p.h, dr2, dtheta2);

        double J11 = (dr1 - dr0) / step;
        double J12 = (dr2 - dr0) / step;
        double J21 = (dtheta1 - dtheta0) / step;
        double J22 = (dtheta2 - dtheta0) / step;

        double det = J11 * J22 - J12 * J21;
        if (std::abs(det) < 1e-12) det = (det > 0 ? 1e-12 : -1e-12);

        double d_theta_dot1 = -(J22 * dr0 - J12 * dtheta0) / det;
        double d_theta_2 = -(-J21 * dr0 + J11 * dtheta0) / det;

        double max_delta = 0.05;
        if (std::abs(d_theta_dot1) > max_delta) d_theta_dot1 = (d_theta_dot1 > 0 ? 1.0 : -1.0) * max_delta;
        if (std::abs(d_theta_2) > max_delta) d_theta_2 = (d_theta_2 > 0 ? 1.0 : -1.0) * max_delta;

        p.theta_dot1 += d_theta_dot1;
        p.theta_2 += d_theta_2;
    }
}

inline void find_initial_guess(Params& p) {
    double best_mass = 1e9;
    double best_t1 = p.t1;
    double best_t2 = p.t2;

    auto evaluate_point = [&](double t1, double t2) {
        Params temp = p;
        temp.t1 = t1;
        temp.t2 = t2;
        solveBVP(temp, 30);

        State s = { 0, 0, 0, 0, temp.m0 };
        double t_end;
        integrate(temp, s, t_end);
        double dr, dtheta;
        residuals(s, temp.h, dr, dtheta);

        if (std::abs(dr) < 10.0 && std::abs(dtheta) < 0.01) {
            double cost = temp.m0 - s.m;
            if (cost < best_mass && cost > 0) {
                best_mass = cost;
                best_t1 = t1;
                best_t2 = t2;
            }
        }
        };

    std::cout << "  Grid search (minimizing fuel)..." << std::endl;
    for (double t1 = 200; t1 <= 650; t1 += 50) {
        for (double t2 = t1 + 100; t2 <= 1600; t2 += 100) {
            evaluate_point(t1, t2);
        }
    }

    std::cout << "  Best coarse point: t1=" << best_t1 << ", t2=" << best_t2 << ", Fuel=" << best_mass << std::endl;

    double coarse_t1 = best_t1;
    double coarse_t2 = best_t2;
    best_mass = 1e9;

    for (double t1 = coarse_t1 - 40; t1 <= coarse_t1 + 40; t1 += 10) {
        for (double t2 = coarse_t2 - 80; t2 <= coarse_t2 + 80; t2 += 20) {
            evaluate_point(t1, t2);
        }
    }

    p.t1 = best_t1;
    p.t2 = best_t2;
    solveBVP(p, 50);
    std::cout << "  Auto-found start: t1=" << p.t1 << ", t2=" << p.t2 << ", Fuel=" << best_mass << std::endl;
}

inline void optimize(Params& p) {
    double alpha = 200.0;
    double C1 = 0.6;
    double C2 = 2.0;
    double eps_extr = 1e-2;
    double step = 1e-3;

    solveBVP(p);
    double g0 = calcMass(p);

    for (int iter = 0; iter < 100; ++iter) {
        Params p1p = p; p1p.t1 += step;
        Params p1m = p; p1m.t1 -= step;
        Params p2p = p; p2p.t2 += step;
        Params p2m = p; p2m.t2 -= step;

        solveBVP(p1p); solveBVP(p1m); solveBVP(p2p); solveBVP(p2m);

        double g1p = calcMass(p1p);
        double g1m = calcMass(p1m);
        double g2p = calcMass(p2p);
        double g2m = calcMass(p2m);

        double grad1 = (g1p - g1m) / (2.0 * step);
        double grad2 = (g2p - g2m) / (2.0 * step);

        if (std::sqrt(grad1 * grad1 + grad2 * grad2) < eps_extr) {
            break;
        }

        Params p_pp = p; p_pp.t1 += step; p_pp.t2 += step;
        Params p_pm = p; p_pm.t1 += step; p_pm.t2 -= step;
        Params p_mp = p; p_mp.t1 -= step; p_mp.t2 += step;
        Params p_mm = p; p_mm.t1 -= step; p_mm.t2 -= step;

        solveBVP(p_pp); solveBVP(p_pm); solveBVP(p_mp); solveBVP(p_mm);

        double gpp = calcMass(p_pp);
        double gpm = calcMass(p_pm);
        double gmp = calcMass(p_mp);
        double gmm = calcMass(p_mm);

        double H11 = (g1p - 2.0 * g0 + g1m) / (step * step);
        double H22 = (g2p - 2.0 * g0 + g2m) / (step * step);
        double H12 = (gpp - gpm - gmp + gmm) / (4.0 * step * step);

        double alpha_i = alpha;
        bool improved = false;

        for (int inner = 0; inner < 50; ++inner) {
            double det = (H11 + alpha_i) * (H22 + alpha_i) - H12 * H12;
            if (std::abs(det) < 1e-10) det = 1e-10;
            double dt1 = -((H22 + alpha_i) * grad1 - H12 * grad2) / det;
            double dt2 = -(-H12 * grad1 + (H11 + alpha_i) * grad2) / det;

            Params p_new = p;
            p_new.t1 += dt1;
            p_new.t2 += dt2;
            solveBVP(p_new);
            double g_new = calcMass(p_new);

            if (g_new < g0) {
                p = p_new;
                g0 = g_new;
                alpha = C1 * alpha_i;
                improved = true;
                std::cout << "  Iter: " << iter << " | Fuel: " << g0 << " kg" << std::endl;
                break;
            }
            else {
                alpha_i = C2 * alpha_i;
            }
        }

        if (!improved) {
            break;
        }
    }
}

inline double diff_mass_pg_mkonstr(const Params& p) {
    double delta_m = 10.0;
    Params p_plus = p; p_plus.m0 += delta_m;
    optimize(p_plus);
    double m_pg_plus = p_plus.m0 - calcMass(p_plus) - (MKONSTR + delta_m);

    Params p_base = p;
    optimize(p_base);
    double m_pg_base = p_base.m0 - calcMass(p_base) - MKONSTR;

    return (m_pg_plus - m_pg_base) / delta_m;
}

inline double diff_mass_pg_W(const Params& p) {
    double delta_W = 10.0;
    Params p_plus = p; p_plus.W += delta_W;
    optimize(p_plus);
    double m_pg_plus = p_plus.m0 - calcMass(p_plus) - MKONSTR;

    Params p_base = p;
    optimize(p_base);
    double m_pg_base = p_base.m0 - calcMass(p_base) - MKONSTR;

    return (m_pg_plus - m_pg_base) / delta_W;
}