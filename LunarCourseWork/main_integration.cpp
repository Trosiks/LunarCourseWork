#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "xlsxwriter.h"
#include "math_model.h"

void write_trajectory_data(lxw_workbook* wb, lxw_worksheet* ws, const std::vector<State>& traj, const std::vector<double>& t_hist, const Params& p, const std::string& title) {
    worksheet_write_string(ws, 0, 0, "t", NULL);
    worksheet_write_string(ws, 0, 1, "h (km)", NULL);
    worksheet_write_string(ws, 0, 2, "V (m/s)", NULL);
    worksheet_write_string(ws, 0, 3, "theta (grad)", NULL);
    worksheet_write_string(ws, 0, 4, "x (km)", NULL);
    worksheet_write_string(ws, 0, 5, "y (km)", NULL);

    for (size_t i = 0; i < traj.size(); ++i) {
        double r = std::sqrt(traj[i].x * traj[i].x + (RM + traj[i].y) * (RM + traj[i].y));
        double h = (r - RM) / 1000.0;
        double V = std::sqrt(traj[i].Vx * traj[i].Vx + traj[i].Vy * traj[i].Vy);

        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 0, t_hist[i], NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 1, h, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 2, V, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 3, getTheta(t_hist[i], p) * 180.0 / PI, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 4, traj[i].x / 1000.0, NULL);
        worksheet_write_number(ws, static_cast<lxw_row_t>(i + 1), 5, traj[i].y / 1000.0, NULL);
    }

    std::string size_str = std::to_string(traj.size() + 1);

    lxw_chart* chart_h = workbook_add_chart(wb, LXW_CHART_LINE);
    chart_add_series(chart_h, ("='" + title + "'!$A$1:$A$" + size_str).c_str(), ("='" + title + "'!$B$1:$B$" + size_str).c_str());
    chart_title_set_name(chart_h, ("h(t) - " + title).c_str());
    worksheet_insert_chart(ws, CELL("H2"), chart_h);

    lxw_chart* chart_v = workbook_add_chart(wb, LXW_CHART_LINE);
    chart_add_series(chart_v, ("='" + title + "'!$A$1:$A$" + size_str).c_str(), ("='" + title + "'!$C$1:$C$" + size_str).c_str());
    chart_title_set_name(chart_v, ("V(t) - " + title).c_str());
    worksheet_insert_chart(ws, CELL("H20"), chart_v);

    lxw_chart* chart_traj = workbook_add_chart(wb, LXW_CHART_SCATTER_STRAIGHT);
    chart_add_series(chart_traj, ("='" + title + "'!$E$1:$E$" + size_str).c_str(), ("='" + title + "'!$F$1:$F$" + size_str).c_str());
    chart_title_set_name(chart_traj, ("Trajectory y(x) - " + title).c_str());
    worksheet_insert_chart(ws, CELL("H38"), chart_traj);
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    std::string excel_filename = "Stage0_Integration_Result.xlsx";
    std::string target_group = "72";
    int target_variant = 18;

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
        std::cout << "Error: Variant not found." << std::endl;
        return 1;
    }

    double t1_arr[4] = { 350.123, 390.0, 520.0, 550.0 };
    double t2_arr[4] = { 630.0, 840.0, 1060.0, 1570.0 };
    double theta_dot_arr[4] = { -0.005128851, -0.004731561, -0.003718726, -0.003590298 };
    double theta2_arr[4] = { -0.243267529, -0.378412759, -0.534278015, -0.873624229 };

    lxw_workbook* workbook = workbook_new(excel_filename.c_str());
    lxw_worksheet* ws_res = workbook_add_worksheet(workbook, "Integration_Results");

    worksheet_write_string(ws_res, 0, 0, "Param", NULL);
    worksheet_write_string(ws_res, 0, 1, "h11", NULL);
    worksheet_write_string(ws_res, 0, 2, "h12", NULL);
    worksheet_write_string(ws_res, 0, 3, "h21", NULL);
    worksheet_write_string(ws_res, 0, 4, "h22", NULL);

    worksheet_write_string(ws_res, 1, 0, "t1 (s)", NULL);
    worksheet_write_string(ws_res, 2, 0, "t2 (s)", NULL);
    worksheet_write_string(ws_res, 3, 0, "theta_dot1", NULL);
    worksheet_write_string(ws_res, 4, 0, "theta_2", NULL);
    worksheet_write_string(ws_res, 5, 0, "Final Altitude (km)", NULL);
    worksheet_write_string(ws_res, 6, 0, "Final Velocity (m/s)", NULL);

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
        params[i].theta_dot1 = theta_dot_arr[i];
        params[i].theta_2 = theta2_arr[i];
        params[i].m0 = 3000.0;

        worksheet_write_number(ws_res, 1, i + 1, params[i].t1, NULL);
        worksheet_write_number(ws_res, 2, i + 1, params[i].t2, NULL);
        worksheet_write_number(ws_res, 3, i + 1, params[i].theta_dot1, NULL);
        worksheet_write_number(ws_res, 4, i + 1, params[i].theta_2, NULL);
    }

    std::string names[4] = { "Traj_h11", "Traj_h12", "Traj_h21", "Traj_h22" };
    for (int i = 0; i < 4; ++i) {
        std::cout << "Integrating trajectory " << i + 1 << " of 4..." << std::endl;

        State s = { 0, 0, 0, 0, params[i].m0 };
        double t_end;
        std::vector<State> traj;
        std::vector<double> time_history;

        integrate(params[i], s, t_end, &traj, &time_history);

        double r = std::sqrt(s.x * s.x + (RM + s.y) * (RM + s.y));
        double h_final = (r - RM) / 1000.0;
        double V_final = std::sqrt(s.Vx * s.Vx + s.Vy * s.Vy);

        worksheet_write_number(ws_res, 5, i + 1, h_final, NULL);
        worksheet_write_number(ws_res, 6, i + 1, V_final, NULL);

        lxw_worksheet* ws_data = workbook_add_worksheet(workbook, names[i].c_str());
        write_trajectory_data(workbook, ws_data, traj, time_history, params[i], names[i]);
    }

    workbook_close(workbook);
    std::cout << "File created: " << excel_filename << std::endl;

    return 0;
}