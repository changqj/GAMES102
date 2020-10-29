#include "CanvasSystem.h"

#include "../Components/CanvasData.h"

#include <_deps/imgui/imgui.h>
#include "../ImGuiFileBrowser.h"

#include"../Fitting/fitting.h"
#include "../Parametrization/parametrization.h"

#include "spdlog/spdlog.h"

#include <fstream>


using namespace Ubpa;

#define MAX_PLOT_NUM_POINTS 10000

void plot_IP(ImVec2*, CanvasData*, int&, const ImVec2, int);
void plot_IG(ImVec2*, CanvasData*, int&, const ImVec2, float, int);
void plot_AL(ImVec2*, CanvasData*, int&, const ImVec2, int, int);
void plot_AR(ImVec2*, CanvasData*, int&, const ImVec2, int, float, int);
imgui_addons::ImGuiFileBrowser file_dialog;

void CanvasSystem::OnUpdate(Ubpa::UECS::Schedule& schedule) {
	spdlog::set_pattern("[%H:%M:%S] %v");
	//spdlog::set_pattern("%+"); // back to default format

	schedule.RegisterCommand([](Ubpa::UECS::World* w) {
		auto data = w->entityMngr.GetSingleton<CanvasData>();

		if (!data)
			return;

		if (ImGui::Begin("Canvas")) {
			if (ImGui::CollapsingHeader("File")) {
				if (ImGui::MenuItem("Import data", "Ctrl+O")) { data->importData = true; }
				if (ImGui::MenuItem("Export data", "Ctrl+S")) { data->exportData = true; }
			}
			//ImGui::SameLine();
			if (ImGui::CollapsingHeader("Help")) {
				ImGui::Text("ABOUT THIS DEMO:");
				ImGui::BulletText("GAMES-102");
				ImGui::Separator();

				ImGui::Text("USER GUIDE:");
				ImGui::BulletText("Mouse Left: drag to add lines, click to add points.");
				ImGui::BulletText("Mouse Right: drag to scroll, click for context menu.");
				ImGui::Separator();
				
			}

			ImGui::Checkbox("Enable grid", &data->opt_enable_grid); ImGui::SameLine(200);
			ImGui::Checkbox("Enable context menu", &data->opt_enable_context_menu);
			
			ImGui::Separator();

			ImGui::Checkbox("Lagrange", &data->enable_IP); ImGui::SameLine(200);
			ImGui::Checkbox("Gauss", &data->enable_IG); ImGui::SameLine(310);

			ImGui::BeginChild("sigma_id", ImVec2(200, 22));
			ImGui::SliderFloat("sigma", &data->sigma, 0.01f, 1.0f, "sigma = %.3f");
			ImGui::EndChild(); ImGui::SameLine(550);
			ImGui::Text("Parametrization Type: ");

			ImGui::Checkbox("Least Squares", &data->enable_ALS); ImGui::SameLine(200);
			ImGui::Checkbox("Ridge Regression", &data->enable_ARR); ImGui::SameLine(550);
			ImGui::RadioButton("chord", &data->parametrizationType, 0); ImGui::SameLine(630);
			ImGui::RadioButton("centripetal", &data->parametrizationType, 1);

			ImGui::BeginChild("order_als_id", ImVec2(100, 22));
			ImGui::InputInt("order_als", &data->order_als);
			data->order_als = data->order_als < 1 ? 1 : data->order_als;
			data->order_als = data->order_als > 50 ? 50 : data->order_als;
			ImGui::EndChild(); ImGui::SameLine(200);
			ImGui::BeginChild("order_arr_id", ImVec2(100, 22));
			ImGui::InputInt("order_arr", &data->order_arr);
			// to limit to the range 1-10
			data->order_arr = data->order_arr < 1 ? 1 : data->order_arr;
			data->order_arr = data->order_arr > 50 ? 50 : data->order_arr;
			ImGui::EndChild(); ImGui::SameLine(310);
			ImGui::BeginChild("lambda_id", ImVec2(200, 22));
			ImGui::SliderFloat("lambda", &data->lambda, 0.0f, 10.0f, "lambda = %.3f");
			ImGui::EndChild(); ImGui::SameLine(550);
			ImGui::RadioButton("uniform", &data->parametrizationType, 2); ImGui::SameLine(630);
			ImGui::RadioButton("Foley", &data->parametrizationType, 3);

			// Typically you would use a BeginChild()/EndChild() pair to benefit from a clipping region + own scrolling.
			// Here we demonstrate that this can be replaced by simple offsetting + custom drawing + PushClipRect/PopClipRect() calls.
			// To use a child window instead we could use, e.g:
			//      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));      // Disable padding
			//      ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(50, 50, 50, 255));  // Set a background color
			//      ImGui::BeginChild("canvas", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoMove);
			//      ImGui::PopStyleColor();
			//      ImGui::PopStyleVar();
			//      [...]
			//      ImGui::EndChild();

			// Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
			ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
			ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
			if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
			if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
			ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

			// Draw border and background color
			ImGuiIO& io = ImGui::GetIO();
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
			draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

			// This will catch our interactions
			ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
			const bool is_hovered = ImGui::IsItemHovered(); // Hovered
			const bool is_active = ImGui::IsItemActive();   // Held
			const ImVec2 origin(canvas_p0.x + data->scrolling[0], canvas_p0.y + data->scrolling[1]); // Lock scrolled origin
			const pointf2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

			// Add first and second point
			if (is_hovered && !data->adding_line && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				data->points.push_back(mouse_pos_in_canvas);
				data->points.push_back(mouse_pos_in_canvas);
				data->adding_line = true;
			}
			if (data->adding_line) {
				data->points.back() = mouse_pos_in_canvas;
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
					data->adding_line = false;
				spdlog::info("Point added at: {}, {}", data->points.back()[0], data->points.back()[1]);
			}

			if (data->importData || (io.KeyCtrl && ImGui::IsKeyPressed(79))) {
				ImGui::OpenPopup("Import Data");
				data->importData = false;
			}
			if (data->exportData || (io.KeyCtrl && ImGui::IsKeyPressed(83))) {
				ImGui::OpenPopup("Export Data");
				data->exportData = false;
			}

			if (file_dialog.showFileDialog("Import Data", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".txt,.xy,.*")) {
				data->points.clear();
				std::ifstream in(file_dialog.selected_path);
				float x, y;
				while (in >> x >> y) {
					data->points.push_back(ImVec2(x, y));
					data->points.push_back(ImVec2(x, y));
				}

				spdlog::info(file_dialog.selected_path);
			}


			if (file_dialog.showFileDialog("Export Data", imgui_addons::ImGuiFileBrowser::DialogMode::SAVE, ImVec2(700, 310), ".txt,.xy,.*")) {
				std::ofstream out(file_dialog.selected_path);
				
				for (int n = 0; n < data->points.size(); n += 2)
					out << data->points[n][0] << "\t" << data->points[n][1] << std::endl;

				spdlog::info(file_dialog.selected_path);
			}

			if (io.KeyCtrl && ImGui::IsKeyPressed(38)) { data->sigma += 0.001; }	// 按下键盘 Ctrl+↑
			if (io.KeyCtrl && ImGui::IsKeyPressed(40)) { data->sigma -= 0.001; }	// 按下键盘 Ctrl+↓

			if (io.KeyShift && ImGui::IsKeyPressed(38)) { data->lambda += 0.001; }	// 按下键盘 Shift+↑
			if (io.KeyShift && ImGui::IsKeyPressed(40)) { data->lambda -= 0.001; }	// 按下键盘 Shift+↓

			if (io.KeyAlt && ImGui::IsKeyPressed(38)) { data->order_als++; data->order_arr++; }	// 按下键盘 Alt+↑
			if (io.KeyAlt && ImGui::IsKeyPressed(40)) { data->order_als--; data->order_arr--; }	// 按下键盘 Alt+↓

			if (io.KeyAlt && ImGui::IsKeyPressed(80)) { data->enable_IP = !data->enable_IP; }	// 按下键盘 Alt+P
			if (io.KeyAlt && ImGui::IsKeyPressed(71)) { data->enable_IG = !data->enable_IG; }	// 按下键盘 Alt+G
			if (io.KeyAlt && ImGui::IsKeyPressed(76)) { data->enable_ALS = !data->enable_ALS; }	// 按下键盘 Alt+L
			if (io.KeyAlt && ImGui::IsKeyPressed(82)) { data->enable_ARR = !data->enable_ARR; }	// 按下键盘 Alt+R

			if (ImGui::IsKeyPressed(9)) { ++data->parametrizationType %= 4; }	// 按下键盘 Tab



			// Pan (we use a zero mouse threshold when there's no context menu)
			// You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
			const float mouse_threshold_for_pan = data->opt_enable_context_menu ? -1.0f : 0.0f;
			if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouse_threshold_for_pan)) {
				data->scrolling[0] += io.MouseDelta.x;
				data->scrolling[1] += io.MouseDelta.y;
			}

			// Context menu (under default mouse threshold)
			ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
			if (data->opt_enable_context_menu && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
				ImGui::OpenPopupContextItem("context");
			if (ImGui::BeginPopup("context")) {
				if (data->adding_line)
					data->points.resize(data->points.size() - 2);
				data->adding_line = false;
				if (ImGui::MenuItem("Remove one", NULL, false, data->points.size() > 0)) { data->points.resize(data->points.size() - 2); }
				if (ImGui::MenuItem("Remove all", NULL, false, data->points.size() > 0)) { data->points.clear(); }
				ImGui::EndPopup();
			}

			// Draw grid + all lines in the canvas
			draw_list->PushClipRect(canvas_p0, canvas_p1, true);
			if (data->opt_enable_grid) {
				const float GRID_STEP = 64.0f;
				for (float x = fmodf(data->scrolling[0], GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
					draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
				for (float y = fmodf(data->scrolling[1], GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
					draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
			}
			for (int n = 0; n < data->points.size(); n += 2)
				draw_list->AddLine(ImVec2(origin.x + data->points[n][0], origin.y + data->points[n][1]), ImVec2(origin.x + data->points[n + 1][0], origin.y + data->points[n + 1][1]), IM_COL32(255, 255, 0, 255), 2.0f);


			for (int n = 0; n < data->points.size(); n += 2) {
				if (data->points[n][0] == data->points[n + 1][0] && data->points[n][1] == data->points[n + 1][1]) {
					draw_list->AddCircleFilled(ImVec2(origin.x + data->points[n][0], origin.y + data->points[n][1]), 2, IM_COL32(255, 255, 255, 255));
				}
			}
			draw_list->PopClipRect();

			if (data->points.size() > 2) {
				// IP
				if (data->enable_IP) {
					int p_index = 0;
					ImVec2 IP[MAX_PLOT_NUM_POINTS];
					plot_IP(IP, data, p_index, origin, data->parametrizationType);
					draw_list->AddPolyline(IP, p_index, IM_COL32(0, 255, 0, 255), false, 2.0f);
					draw_list->AddText(ImVec2(canvas_p1.x - 120, canvas_p1.y - 20), IM_COL32(255, 255, 255, 255), "Lagrange");
					draw_list->AddLine(ImVec2(canvas_p1.x - 175, canvas_p1.y - 13), ImVec2(canvas_p1.x - 125, canvas_p1.y - 13), IM_COL32(0, 255, 0, 255), 2.0f);
				}

				// IG
				if (data->enable_IG) {
					int p_index = 0;
					ImVec2 IG[MAX_PLOT_NUM_POINTS];
					plot_IG(IG, data, p_index, origin, data->sigma, data->parametrizationType);
					draw_list->AddPolyline(IG, p_index, IM_COL32(0, 255, 255, 255), false, 2.0f);
					draw_list->AddText(ImVec2(canvas_p1.x - 120, canvas_p1.y - 20 - data->enable_IP * 20), IM_COL32(255, 255, 255, 255), "Gauss Base");
					draw_list->AddLine(ImVec2(canvas_p1.x - 175, canvas_p1.y - 13 - data->enable_IP * 20), ImVec2(canvas_p1.x - 125, canvas_p1.y - 13 - data->enable_IP * 20), IM_COL32(0, 255, 255, 255), 2.0f);
				}

				// AL
				if (data->enable_ALS) {
					int p_index = 0;
					ImVec2 AL[MAX_PLOT_NUM_POINTS];
					plot_AL(AL, data, p_index, origin, data->order_als, data->parametrizationType);
					draw_list->AddPolyline(AL, p_index, IM_COL32(217, 84, 19, 255), false, 2.0f);
					draw_list->AddText(ImVec2(canvas_p1.x - 120, canvas_p1.y - 20 - (data->enable_IP + data->enable_IG) * 20), IM_COL32(255, 255, 255, 255), "Least Square");
					draw_list->AddLine(ImVec2(canvas_p1.x - 175, canvas_p1.y - 13 - (data->enable_IP + data->enable_IG) * 20), ImVec2(canvas_p1.x - 125, canvas_p1.y - 13 - (data->enable_IP + data->enable_IG) * 20), IM_COL32(217, 84, 19, 255), 2.0f);
				}

				// AR
				if (data->enable_ARR) {
					int p_index = 0;
					ImVec2 AR[MAX_PLOT_NUM_POINTS];
					plot_AR(AR, data, p_index, origin, data->order_arr, data->lambda, data->parametrizationType);
					draw_list->AddPolyline(AR, p_index, IM_COL32(128, 91, 236, 255), false, 2.0f);
					draw_list->AddText(ImVec2(canvas_p1.x - 120, canvas_p1.y - 20 - (data->enable_IP + data->enable_IG + data->enable_ALS) * 20), IM_COL32(255, 255, 255, 255), "Ridge Regression");
					draw_list->AddLine(ImVec2(canvas_p1.x - 175, canvas_p1.y - 13 - (data->enable_IP + data->enable_IG + data->enable_ALS) * 20), ImVec2(canvas_p1.x - 125, canvas_p1.y - 13 - (data->enable_IP + data->enable_IG + data->enable_ALS) * 20), IM_COL32(128, 91, 236, 255), 2.0f);
				}
			}
		}
		ImGui::End();
	});
}

void plot_IP(ImVec2* p, CanvasData* data, int& p_index, const ImVec2 origin, int parametrizationType) {
	std::vector<Ubpa::pointf2> points;
	for (int n = 0; n < data->points.size(); n += 2)
		points.push_back(data->points[n] + origin);

	//parametrization
	Eigen::VectorXf t;
	switch (parametrizationType) {
	case 0:
		t = Parametrization::chordParameterization(points);
		break;
	case 1:
		t = Parametrization::centripetalParameterization(points);
		break;
	case 2:
		t = Parametrization::uniformParameterization(points.size());
		break;
	case 3:
		t = Parametrization::FoleyParameterization(points);
		break;
	default:
		t = Parametrization::chordParameterization(points);
	}

	std::vector<Ubpa::pointf2> tx, ty;
	for (int i = 0; i < points.size(); ++i) {
		tx.push_back(Ubpa::pointf2(t[i], points[i][0]));
		ty.push_back(Ubpa::pointf2(t[i], points[i][1]));
	}

	// Interpolation: Polynomial Base Function
	Eigen::VectorXf coefficients_IP_x = Fitting::Interpolation_PolynomialBaseFunction(tx);
	Eigen::VectorXf coefficients_IP_y = Fitting::Interpolation_PolynomialBaseFunction(ty);

	p_index = 0;
	for (float t_i = 0; t_i <= 1 && p_index < MAX_PLOT_NUM_POINTS; t_i += 0.001) {
		float fx = 0, fy = 0;
		for (int j = 0; j < coefficients_IP_x.size(); ++j) {
			fx += coefficients_IP_x[j] * powf(t_i, j);
			fy += coefficients_IP_y[j] * powf(t_i, j);
		}
		p[p_index++] = ImVec2(fx, fy);
	}
}

void plot_IG(ImVec2* p, CanvasData* data, int& p_index, const ImVec2 origin, float sigma, int parametrizationType) {
	std::vector<Ubpa::pointf2> points;
	for (int n = 0; n < data->points.size(); n += 2)
		points.push_back(data->points[n] + origin);

	//parametrization
	Eigen::VectorXf t;
	switch (parametrizationType) {
	case 0:
		t = Parametrization::chordParameterization(points);
		break;
	case 1:
		t = Parametrization::centripetalParameterization(points);
		break;
	case 2:
		t = Parametrization::uniformParameterization(points.size());
		break;
	case 3:
		t = Parametrization::FoleyParameterization(points);
		break;
	default:
		t = Parametrization::chordParameterization(points);
	}

	std::vector<Ubpa::pointf2> tx, ty;
	for (int i = 0; i < points.size(); ++i) {
		tx.push_back(Ubpa::pointf2(t[i], points[i][0]));
		ty.push_back(Ubpa::pointf2(t[i], points[i][1]));
	}

	// Interpolation: Gauss Base Function
	Eigen::VectorXf coefficients_IG_x = Fitting::Interpolation_GaussBaseFunction(tx, sigma);
	Eigen::VectorXf coefficients_IG_y = Fitting::Interpolation_GaussBaseFunction(ty, sigma);

	p_index = 0;
	for (float t_i = 0; t_i <= 1 && p_index < MAX_PLOT_NUM_POINTS; t_i += 0.001) {
		float fx = coefficients_IG_x[0], fy = coefficients_IG_y[0];
		for (int j = 1; j < coefficients_IG_x.size(); ++j) {
			fx += coefficients_IG_x[j] * expf(-(t_i - t[j - 1]) * (t_i - t[j - 1]) / (2 * sigma * sigma));
			fy += coefficients_IG_y[j] * expf(-(t_i - t[j - 1]) * (t_i - t[j - 1]) / (2 * sigma * sigma));
		}
		p[p_index++] = ImVec2(fx, fy);
	}
}

void plot_AL(ImVec2* p, CanvasData* data, int& p_index, const ImVec2 origin, int order, int parametrizationType) {
	std::vector<Ubpa::pointf2> points;
	for (int n = 0; n < data->points.size(); n += 2)
		points.push_back(data->points[n] + origin);

	//parametrization
	Eigen::VectorXf t;
	switch (parametrizationType) {
	case 0:
		t = Parametrization::chordParameterization(points);
		break;
	case 1:
		t = Parametrization::centripetalParameterization(points);
		break;
	case 2:
		t = Parametrization::uniformParameterization(points.size());
		break;
	case 3:
		t = Parametrization::FoleyParameterization(points);
		break;
	default:
		t = Parametrization::chordParameterization(points);
	}

	std::vector<Ubpa::pointf2> tx, ty;
	for (int i = 0; i < points.size(); ++i) {
		tx.push_back(Ubpa::pointf2(t[i], points[i][0]));
		ty.push_back(Ubpa::pointf2(t[i], points[i][1]));
	}

	// Approximation: Least Square
	Eigen::VectorXf coefficients_AL_x = Fitting::Approximation_LeastSquare(tx, order);
	Eigen::VectorXf coefficients_AL_y = Fitting::Approximation_LeastSquare(ty, order);

	p_index = 0;
	for (float t_i = 0; t_i <= 1 && p_index < MAX_PLOT_NUM_POINTS; t_i += 0.001) {
		float fx = 0, fy = 0;
		for (int j = 0; j < coefficients_AL_x.size(); ++j) {
			fx += coefficients_AL_x[j] * powf(t_i, j);
			fy += coefficients_AL_y[j] * powf(t_i, j);
		}
		p[p_index++] = ImVec2(fx, fy);
	}
}

void plot_AR(ImVec2* p, CanvasData* data, int& p_index, const ImVec2 origin, int order, float lambda, int parametrizationType) {
	std::vector<Ubpa::pointf2> points;
	for (int n = 0; n < data->points.size(); n += 2)
		points.push_back(data->points[n] + origin);

	//parametrization
	Eigen::VectorXf t;
	switch (parametrizationType) {
	case 0:
		t = Parametrization::chordParameterization(points);
		break;
	case 1:
		t = Parametrization::centripetalParameterization(points);
		break;
	case 2:
		t = Parametrization::uniformParameterization(points.size());
		break;
	case 3:
		t = Parametrization::FoleyParameterization(points);
		break;
	default:
		t = Parametrization::chordParameterization(points);
	}

	std::vector<Ubpa::pointf2> tx, ty;
	for (int i = 0; i < points.size(); ++i) {
		tx.push_back(Ubpa::pointf2(t[i], points[i][0]));
		ty.push_back(Ubpa::pointf2(t[i], points[i][1]));
	}

	// Approximation: Ridge Regression
	Eigen::VectorXf coefficients_AR_x = Fitting::Approximation_RidgeRegression(tx, order, lambda);
	Eigen::VectorXf coefficients_AR_y = Fitting::Approximation_RidgeRegression(ty, order, lambda);

	p_index = 0;
	for (float t_i = 0; t_i <= 1 && p_index < MAX_PLOT_NUM_POINTS; t_i += 0.001) {
		float fx = 0, fy = 0;
		for (int j = 0; j < coefficients_AR_x.size(); ++j) {
			fx += coefficients_AR_x[j] * powf(t_i, j);
			fy += coefficients_AR_y[j] * powf(t_i, j);
		}
		p[p_index++] = ImVec2(fx, fy);
	}
}