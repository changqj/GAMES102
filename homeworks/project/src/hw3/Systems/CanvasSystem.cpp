#include "CanvasSystem.h"

#include "../Components/CanvasData.h"

#include <_deps/imgui/imgui.h>

#include"../Fitting/fitting.h"
#include "../Parametrization/parametrization.h"

#include "spdlog/spdlog.h"


using namespace Ubpa;

#define MAX_PLOT_NUM_POINTS 10000

void plot_IP(ImVec2*, CanvasData*, int&, const ImVec2);
void plot_IG(ImVec2*, CanvasData*, int&, const ImVec2, float);
void plot_AL(ImVec2*, CanvasData*, int&, const ImVec2, int);
void plot_AR(ImVec2*, CanvasData*, int&, const ImVec2, int, float);

void CanvasSystem::OnUpdate(Ubpa::UECS::Schedule& schedule) {
	schedule.RegisterCommand([](Ubpa::UECS::World* w) {
		auto data = w->entityMngr.GetSingleton<CanvasData>();

		if (!data)
			return;

		if (ImGui::Begin("Canvas")) {
			ImGui::Checkbox("Enable grid", &data->opt_enable_grid);
			ImGui::Checkbox("Enable context menu", &data->opt_enable_context_menu);
			ImGui::Text("Mouse Left: drag to add lines, click to add points,\nMouse Right: drag to scroll, click for context menu.");
			ImGui::Separator();
			ImGui::Checkbox("Lagrange", &data->enable_IP);
			ImGui::SameLine(200);
			ImGui::Checkbox("Gauss", &data->enable_IG);
			ImGui::SameLine(310);

			ImGui::BeginChild("sigma_id", ImVec2(200, 30));
			ImGui::SliderFloat("sigma", &data->sigma, 0.01f, 1.0f);
			ImGui::EndChild();

			ImGui::Checkbox("Least Squares", &data->enable_ALS);
			ImGui::SameLine(200);
			ImGui::Checkbox("Ridge Regression", &data->enable_ARR);
			ImGui::BeginChild("order_als_id", ImVec2(100, 30));
			ImGui::InputInt("order_als", &data->order_als);
			data->order_als = data->order_als < 1 ? 1 : data->order_als;
			data->order_als = data->order_als > 10 ? 10 : data->order_als;
			ImGui::EndChild();
			ImGui::SameLine(200);
			ImGui::BeginChild("order_arr_id", ImVec2(100, 30));
			ImGui::InputInt("order_arr", &data->order_arr);
			// to limit to the range 1-10
			data->order_arr = data->order_arr < 1 ? 1 : data->order_arr;
			data->order_arr = data->order_arr > 10 ? 10 : data->order_arr;
			ImGui::EndChild();
			ImGui::SameLine(310);
			ImGui::BeginChild("lambda_id", ImVec2(200, 30));
			ImGui::SliderFloat("lambda", &data->lambda, 0.0f, 10.0f);
			ImGui::EndChild();

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
			if (!io.KeyCtrl && is_hovered && !data->adding_line && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
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
					draw_list->AddCircleFilled(ImVec2(origin.x + data->points[n][0], origin.y + data->points[n][1]), 8, IM_COL32(255, 255, 255, 255));
				}
			}
			draw_list->PopClipRect();

			if (data->points.size() > 2) {
				// IP
				if (data->enable_IP) {
					int p_index = 0;
					ImVec2 IP[MAX_PLOT_NUM_POINTS];
					plot_IP(IP, data, p_index, origin);
					draw_list->AddPolyline(IP, p_index, IM_COL32(0, 255, 0, 255), false, 2.0f);
					draw_list->AddText(ImVec2(canvas_p1.x - 120, canvas_p1.y - 20), IM_COL32(255, 255, 255, 255), "Lagrange");
					draw_list->AddLine(ImVec2(canvas_p1.x - 175, canvas_p1.y - 13), ImVec2(canvas_p1.x - 125, canvas_p1.y - 13), IM_COL32(0, 255, 0, 255), 2.0f);
				}

				// IG
				if (data->enable_IG) {
					int p_index = 0;
					ImVec2 IG[MAX_PLOT_NUM_POINTS];
					plot_IG(IG, data, p_index, origin, data->sigma);
					draw_list->AddPolyline(IG, p_index, IM_COL32(0, 255, 255, 255), false, 2.0f);
					draw_list->AddText(ImVec2(canvas_p1.x - 120, canvas_p1.y - 20 - data->enable_IP * 20), IM_COL32(255, 255, 255, 255), "Gauss Base");
					draw_list->AddLine(ImVec2(canvas_p1.x - 175, canvas_p1.y - 13 - data->enable_IP * 20), ImVec2(canvas_p1.x - 125, canvas_p1.y - 13 - data->enable_IP * 20), IM_COL32(0, 255, 255, 255), 2.0f);
				}

				// AL
				if (data->enable_ALS) {
					int p_index = 0;
					ImVec2 AL[MAX_PLOT_NUM_POINTS];
					plot_AL(AL, data, p_index, origin, data->order_als);
					draw_list->AddPolyline(AL, p_index, IM_COL32(217, 84, 19, 255), false, 2.0f);
					draw_list->AddText(ImVec2(canvas_p1.x - 120, canvas_p1.y - 20 - (data->enable_IP + data->enable_IG) * 20), IM_COL32(255, 255, 255, 255), "Least Square");
					draw_list->AddLine(ImVec2(canvas_p1.x - 175, canvas_p1.y - 13 - (data->enable_IP + data->enable_IG) * 20), ImVec2(canvas_p1.x - 125, canvas_p1.y - 13 - (data->enable_IP + data->enable_IG) * 20), IM_COL32(217, 84, 19, 255), 2.0f);
				}

				// AR
				if (data->enable_ARR) {
					int p_index = 0;
					ImVec2 AR[MAX_PLOT_NUM_POINTS];
					plot_AR(AR, data, p_index, origin, data->order_arr, data->lambda);
					draw_list->AddPolyline(AR, p_index, IM_COL32(128, 91, 236, 255), false, 2.0f);
					draw_list->AddText(ImVec2(canvas_p1.x - 120, canvas_p1.y - 20 - (data->enable_IP + data->enable_IG + data->enable_ALS) * 20), IM_COL32(255, 255, 255, 255), "Ridge Regression");
					draw_list->AddLine(ImVec2(canvas_p1.x - 175, canvas_p1.y - 13 - (data->enable_IP + data->enable_IG + data->enable_ALS) * 20), ImVec2(canvas_p1.x - 125, canvas_p1.y - 13 - (data->enable_IP + data->enable_IG + data->enable_ALS) * 20), IM_COL32(128, 91, 236, 255), 2.0f);
				}
			}
		}
		ImGui::End();
	});


}


void plot_IP(ImVec2* p, CanvasData* data, int& p_index, const ImVec2 origin) {
	std::vector<Ubpa::pointf2> points;
	for (int n = 0; n < data->points.size(); n += 2)
		points.push_back(data->points[n] + origin);

	//parametrization
	Eigen::VectorXf t = Parametrization::centripetalParameterization(points);

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

void plot_IG(ImVec2* p, CanvasData* data, int& p_index, const ImVec2 origin, float sigma = 1.0f) {
	std::vector<Ubpa::pointf2> points;
	for (int n = 0; n < data->points.size(); n += 2)
		points.push_back(data->points[n] + origin);

	//parametrization
	Eigen::VectorXf t = Parametrization::centripetalParameterization(points);

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

void plot_AL(ImVec2* p, CanvasData* data, int& p_index, const ImVec2 origin, int order = 1) {
	std::vector<Ubpa::pointf2> points;
	for (int n = 0; n < data->points.size(); n += 2)
		points.push_back(data->points[n] + origin);

	//parametrization
	Eigen::VectorXf t = Parametrization::centripetalParameterization(points);

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

void plot_AR(ImVec2* p, CanvasData* data, int& p_index, const ImVec2 origin, int order = 1, float lambda = 0.2f) {
	std::vector<Ubpa::pointf2> points;
	for (int n = 0; n < data->points.size(); n += 2)
		points.push_back(data->points[n] + origin);

	//parametrization
	Eigen::VectorXf t = Parametrization::centripetalParameterization(points);

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