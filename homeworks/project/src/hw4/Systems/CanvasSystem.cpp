#include "CanvasSystem.h"

#include "../Components/CanvasData.h"

#include <_deps/imgui/imgui.h>
#include "../ImGuiFileBrowser.h"

#include "../Parametrization/parametrization.h"

#include "spdlog/spdlog.h"

#include <fstream>


using namespace Ubpa;

constexpr auto MAX_PLOT_NUM_POINTS = 10000;

Eigen::VectorXf parametrization(std::vector<Ubpa::pointf2>, int);
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
				ImGui::BulletText("Ctrl+O: Import Data");
				ImGui::BulletText("Ctrl+S: Export Data");
				ImGui::BulletText("Tab: change the parameterization method.");
				ImGui::Separator();
				
			}

			ImGui::Checkbox("Enable grid", &data->opt_enable_grid); ImGui::SameLine(200);
			ImGui::Checkbox("Enable context menu", &data->opt_enable_context_menu);
			
			ImGui::Separator();

			ImGui::Text("Parametrization Type: ");

			ImGui::RadioButton("chord", &data->parametrizationType, 0); ImGui::SameLine();
			ImGui::BeginChild("id1", ImVec2(30, 20)); ImGui::EndChild(); ImGui::SameLine();
			ImGui::RadioButton("centripetal", &data->parametrizationType, 1); ImGui::SameLine();
			ImGui::BeginChild("id2", ImVec2(30, 20)); ImGui::EndChild(); ImGui::SameLine();
			ImGui::RadioButton("uniform", &data->parametrizationType, 2); ImGui::SameLine();
			ImGui::BeginChild("id3", ImVec2(30, 20)); ImGui::EndChild(); ImGui::SameLine();
			ImGui::RadioButton("Foley", &data->parametrizationType, 3);

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

			// add a point
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { data->isEnd = true; }	// 双击结束
			if (is_hovered && !data->isEnd && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				data->points.push_back(mouse_pos_in_canvas);
				spdlog::info("Point added at: {}, {}", data->points.back()[0], data->points.back()[1]);
			}

			if (!data->isEnd && data->points.size()) {
				// 计算曲线

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

				for (int n = 0; n < data->points.size(); ++n)
					out << data->points[n][0] << "\t" << data->points[n][1] << std::endl;

				spdlog::info(file_dialog.selected_path);
			}

			if (ImGui::IsKeyPressed(9)) { ++data->parametrizationType %= 4; }	// 按下键盘 Tab


			// Pan (we use a zero mouse threshold when there's no context menu)
			// You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
			const float mouse_threshold_for_pan = data->opt_enable_context_menu ? -1.0f : 0.0f;
			if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouse_threshold_for_pan)) {
				data->scrolling[0] += io.MouseDelta.x;
				data->scrolling[1] += io.MouseDelta.y;
			}

			if (!data->points.size()) { data->isEnd = false; }

			// Context menu (under default mouse threshold)
			ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
			if (data->opt_enable_context_menu && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
				ImGui::OpenPopupContextItem("context");
			if (ImGui::BeginPopup("context")) {
				if (ImGui::MenuItem("Remove one", NULL, false, data->points.size() > 0)) { data->points.resize(data->points.size() - 1); }
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

			// 画数据点
			for (int n = 0; n < data->points.size(); ++n) {
				draw_list->AddCircleFilled(ImVec2(origin.x + data->points[n][0], origin.y + data->points[n][1]), 4, IM_COL32(255, 255, 255, 255));
			}
			draw_list->PopClipRect();


			if (data->points.size() > 2) {
				
			}
		}
		ImGui::End();
	});
}


Eigen::VectorXf parametrization(std::vector<Ubpa::pointf2> points, int parametrizationType) {
	//parametrization
	switch (parametrizationType) {
	case 0:
		return Parametrization::chordParameterization(points);
	case 1:
		return Parametrization::centripetalParameterization(points);
	case 2:
		return Parametrization::uniformParameterization(points.size());
	case 3:
		return Parametrization::FoleyParameterization(points);
	default:
		return Parametrization::chordParameterization(points);
	}
}