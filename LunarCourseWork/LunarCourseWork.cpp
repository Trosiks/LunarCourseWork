#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "xlsxwriter.h"
#include "math_model.h"

void write_trajectory_data(lxw_workbook* wb, lxw_worksheet* ws, const std::vector<State>& traj, const std::vector<double>& t_hist, const Params& p, const std::string& title) {
    worksheet_write_string(ws, 0, 0, "t, s", NULL);
    worksheet_write_string(ws, 0, 1, "m, kg", NULL);
    worksheet_write_string(ws, 0, 2, "Vx, m/s", NULL);
    worksheet_write_string(ws, 0, 3, "Vy, m/s", NULL);
    worksheet_write_string(ws, 0, 4, "x, km", NULL);
    worksheet_write_string(ws, 0, 5, "y, km", NULL);
    worksheet_write_string(ws, 0, 6, "h, km", NULL);
    worksheet_write_string(ws, 0, 7, "V, m/s", NULL);
    worksheet_write_string(ws, 0, 8, "r, km", NULL);
    worksheet_write_string(ws, 0, 9, "vartheta, deg", NULL);
    worksheet_write_string(ws, 0, 10, "theta_c, deg", NULL);
    worksheet_write_string(ws, 0, 11, "alpha, deg", NULL);
    worksheet_write_string(ws, 0, 12, "phi, deg", NULL);

    for (size_t i = 0; i < traj.size(); ++i) {
        double r = std::sqrt(traj[i].x * traj[i].x + (RM + traj[i].y) * (RM + traj[i].y));
        double h = (r - RM) / 1000.0;
        double V = std::sqrt(traj[i].Vx * traj[i].Vx + traj[i].Vy * traj[i].Vy);

        double vartheta_rad = getTheta(t_hist[i], p);
        double theta_c_rad = atan2(traj[i].Vy, traj[i].Vx);
        double alpha_rad = vartheta_rad - theta_c_rad;
        double phi_rad = atan2(-RM * traj[i].x, RM * (RM + traj[i].y));

        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 0, t_hist[i], NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 1, traj[i].m, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 2, traj[i].Vx, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 3, traj[i].Vy, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 4, traj[i].x / 1000.0, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 5, traj[i].y / 1000.0, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 6, h, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 7, V, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 8, r / 1000.0, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 9, vartheta_rad * 180.0 / PI, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 10, theta_c_rad * 180.0 / PI, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 11, alpha_rad * 180.0 / PI, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 12, phi_rad * 180.0 / PI, NULL);
    }

    std::string size_str = std::to_string(traj.size() + 1);

    lxw_chart* chart_h = workbook_add_chart(wb, LXW_CHART_LINE);
    chart_add_series(chart_h, ("='" + title + "'!$A$1:$A$" + size_str).c_str(), ("='" + title + "'!$G$1:$G$" + size_str).c_str());
    chart_title_set_name(chart_h, ("h(t) - " + title).c_str());
    worksheet_insert_chart(ws, CELL("O2"), chart_h);

    lxw_chart* chart_v = workbook_add_chart(wb, LXW_CHART_LINE);
    chart_add_series(chart_v, ("='" + title + "'!$A$1:$A$" + size_str).c_str(), ("='" + title + "'!$H$1:$H$" + size_str).c_str());
    chart_title_set_name(chart_v, ("V(t) - " + title).c_str());
    worksheet_insert_chart(ws, CELL("O20"), chart_v);
}

void write_report_table(lxw_workbook* wb, lxw_worksheet* ws, const std::vector<State>& traj, const std::vector<double>& t_hist, const Params& p, const std::string& title) {
    worksheet_write_string(ws, 0, 0, "t, s", NULL);
    worksheet_write_string(ws, 0, 1, "m, kg", NULL);
    worksheet_write_string(ws, 0, 2, "Vx, m/s", NULL);
    worksheet_write_string(ws, 0, 3, "Vy, m/s", NULL);
    worksheet_write_string(ws, 0, 4, "x, km", NULL);
    worksheet_write_string(ws, 0, 5, "y, km", NULL);
    worksheet_write_string(ws, 0, 6, "h, km", NULL);
    worksheet_write_string(ws, 0, 7, "V, m/s", NULL);
    worksheet_write_string(ws, 0, 8, "r, km", NULL);
    worksheet_write_string(ws, 0, 9, "vartheta, deg", NULL);
    worksheet_write_string(ws, 0, 10, "theta_c, deg", NULL);
    worksheet_write_string(ws, 0, 11, "alpha, deg", NULL);
    worksheet_write_string(ws, 0, 12, "phi, deg", NULL);

    auto find_idx = [&](double target) {
        size_t best = 0;
        double min_diff = 1e9;
        for (size_t i = 0; i < t_hist.size(); ++i) {
            double diff = std::abs(t_hist[i] - target);
            if (diff < min_diff) { min_diff = diff; best = i; }
        }
        return best;
        };

    std::vector<size_t> indices;
    indices.push_back(0);
    indices.push_back(find_idx(TV));
    indices.push_back(find_idx(p.t1));
    indices.push_back(find_idx(p.t2));
    indices.push_back(traj.size() - 1);

    for (size_t k = 0; k < indices.size(); ++k) {
        size_t i = indices[k];
        double r = std::sqrt(traj[i].x * traj[i].x + (RM + traj[i].y) * (RM + traj[i].y));
        double h = (r - RM) / 1000.0;
        double V = std::sqrt(traj[i].Vx * traj[i].Vx + traj[i].Vy * traj[i].Vy);

        double vartheta_rad = getTheta(t_hist[i], p);
        double theta_c_rad = atan2(traj[i].Vy, traj[i].Vx);
        double alpha_rad = vartheta_rad - theta_c_rad;
        double phi_rad = atan2(-RM * traj[i].x, RM * (RM + traj[i].y));

        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 0, t_hist[i], NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 1, traj[i].m, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 2, traj[i].Vx, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 3, traj[i].Vy, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 4, traj[i].x / 1000.0, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 5, traj[i].y / 1000.0, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 6, h, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 7, V, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 8, r / 1000.0, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 9, vartheta_rad * 180.0 / PI, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 10, theta_c_rad * 180.0 / PI, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 11, alpha_rad * 180.0 / PI, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(k + 1), 12, phi_rad * 180.0 / PI, NULL);
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    std::string excel_filename = "Result_72_2.xlsx";    // файл
    std::string target_group = "72";    // группа
    int target_variant = 2;     // вариант

    double W_ist = 0, P1 = 0, P2 = 0, h11 = 0, h12 = 0, h21 = 0, h22 = 0;
    bool found = false;

    std::ifstream file("variants.txt");
    if (!file.is_open()) {
        std::cout << "Error: Cannot open variants.txt." << std::endl;
        return 1;
    }

    std::string line;
    std::string current_group = "";

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) line = line.substr(3);

        size_t start = line.find("[");
        size_t end = line.find("]");
        if (start != std::string::npos && end != std::string::npos) {
            current_group = line.substr(start + 1, end - start - 1);
            continue;
        }

        if (current_group == target_group) {
            std::stringstream ss(line);
            int var;
            if (ss >> var && var == target_variant) {
                if (ss >> W_ist >> P1 >> P2 >> h11 >> h12 >> h21 >> h22) {
                    P1 *= 1000.0;
                    P2 *= 1000.0;
                    h11 *= 1000.0;
                    h12 *= 1000.0;
                    h21 *= 1000.0;
                    h22 *= 1000.0;
                    found = true;
                    break;
                }
            }
        }
    }
    file.close();

    if (!found) {
        std::cout << "Error: Variant " << target_variant << " for group " << target_group << " not found." << std::endl;
        return 1;
    }

    double theta_dot1_init = -0.003;
    double theta_2_init = -0.1;

    double t1_arr[4] = { 300.0, 300.0, 300.0, 300.0 };
    double t2_arr[4] = { 800.0, 800.0, 800.0, 800.0 };

    lxw_workbook* workbook = workbook_new(excel_filename.c_str());
    lxw_worksheet* ws_res = workbook_add_worksheet(workbook, "Results");
    lxw_worksheet* ws_deriv = workbook_add_worksheet(workbook, "Derivatives");

    worksheet_write_string(ws_res, 0, 0, "Param", NULL);
    worksheet_write_string(ws_res, 0, 1, "h11", NULL);
    worksheet_write_string(ws_res, 0, 2, "h12", NULL);
    worksheet_write_string(ws_res, 0, 3, "h21", NULL);
    worksheet_write_string(ws_res, 0, 4, "h22", NULL);

    worksheet_write_string(ws_res, 1, 0, "t1", NULL);
    worksheet_write_string(ws_res, 2, 0, "t2", NULL);

    std::vector<Params> params(4);
    double h_arr[4] = { h11, h12, h21, h22 };
    double P1_arr[4] = { P1, P1, P2, P2 };
    double P2_arr[4] = { P1, P1, P2, P2 };

    for (int i = 0; i < 4; ++i) {
        params[i].W = W_ist;
        params[i].P1 = P1_arr[i];
        params[i].P2 = P2_arr[i];
        params[i].h = h_arr[i];
        params[i].t1 = t1_arr[i];
        params[i].t2 = t2_arr[i];
        params[i].theta_dot1 = theta_dot1_init;
        params[i].theta_2 = theta_2_init;
        params[i].m0 = 3000.0;
    }

    worksheet_write_string(ws_res, 5, 0, "theta_dot1", NULL);
    worksheet_write_string(ws_res, 6, 0, "theta_2", NULL);
    worksheet_write_string(ws_res, 7, 0, "Fuel_cost", NULL);

    for (int i = 0; i < 4; ++i) {
        std::cout << "Optimizing variant " << i + 1 << " of 4..." << std::endl;

        find_initial_guess(params[i]);
        optimize(params[i]);

        double cost = calcMass(params[i]);

        worksheet_write_number(ws_res, 1, i + 1, params[i].t1, NULL);
        worksheet_write_number(ws_res, 2, i + 1, params[i].t2, NULL);
        worksheet_write_number(ws_res, 5, i + 1, params[i].theta_dot1, NULL);
        worksheet_write_number(ws_res, 6, i + 1, params[i].theta_2, NULL);
        worksheet_write_number(ws_res, 7, i + 1, cost, NULL);
    }

    std::cout << "Calculating derivatives..." << std::endl;
    worksheet_write_string(ws_deriv, 0, 0, "Param", NULL);
    worksheet_write_string(ws_deriv, 0, 1, "h21 (P2)", NULL);
    worksheet_write_string(ws_deriv, 0, 2, "h22 (P2)", NULL);

    worksheet_write_string(ws_deriv, 1, 0, "d_mpg_d_mkonstr", NULL);
    worksheet_write_number(ws_deriv, 1, 1, diff_mass_pg_mkonstr(params[2]), NULL);
    worksheet_write_number(ws_deriv, 1, 2, diff_mass_pg_mkonstr(params[3]), NULL);

    worksheet_write_string(ws_deriv, 2, 0, "d_mpg_d_W", NULL);
    worksheet_write_number(ws_deriv, 2, 1, diff_mass_pg_W(params[2]), NULL);
    worksheet_write_number(ws_deriv, 2, 2, diff_mass_pg_W(params[3]), NULL);

    std::string names[4] = { "Traj_h11", "Traj_h12", "Traj_h21", "Traj_h22" };
    std::string report_names[4] = { "Report_h11", "Report_h12", "Report_h21", "Report_h22" };

    for (int i = 0; i < 4; ++i) {
        State s = { 0, 0, 0, 0, params[i].m0 };
        double t_end;
        std::vector<State> traj;
        std::vector<double> time_history;
        integrate(params[i], s, t_end, &traj, &time_history);

        lxw_worksheet* ws_data = workbook_add_worksheet(workbook, names[i].c_str());
        write_trajectory_data(workbook, ws_data, traj, time_history, params[i], names[i]);

        lxw_worksheet* ws_report = workbook_add_worksheet(workbook, report_names[i].c_str());
        write_report_table(workbook, ws_report, traj, time_history, params[i], report_names[i]);
    }

    workbook_close(workbook);
    std::cout << "File created: " << excel_filename << std::endl;

    return 0;
}