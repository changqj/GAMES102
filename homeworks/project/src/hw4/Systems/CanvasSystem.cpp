#include "CanvasSystem.h"

#include "../Components/CanvasData.h"

#include <_deps/imgui/imgui.h>
#include "../ImGuiFileBrowser.h"
#include "../Parametrization/parametrization.h"
#include "spdlog/spdlog.h"
#include "../Curve/curve.h"

#include <fstream>


using namespace Ubpa;

constexpr auto MAX_PLOT_NUM_POINTS = 10000;

imgui_addons::ImGuiFileBrowser file_dialog;

bool enable_edit = false;

int selectedCtrlPoint = -1;

float r = 5.0f;	// 方形点的半径

int modelType = 0;	// 0: 平滑顶点；  1：直线顶点；   2：角部顶点
int selectedRight = -1;


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
				data->ctrlPoints.push_back(CtrlPoint(mouse_pos_in_canvas));
				data->_mx.push_back(0.0f);
				data->_my.push_back(0.0f);
				selectedRight++;
				spdlog::info("Point added at: {}, {}", data->ctrlPoints.back().point[0], data->ctrlPoints.back().point[1]);
				spdlog::info(enable_edit);
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
				data->ctrlPoints.clear();
				std::ifstream in(file_dialog.selected_path);
				float x, y;
				while (in >> x >> y) {
					data->ctrlPoints.push_back(CtrlPoint(ImVec2(x, y)));
					data->_mx.push_back(0.0f);
					data->_my.push_back(0.0f);
				}
				selectedRight = data->ctrlPoints.size() - 1;
				spdlog::info(file_dialog.selected_path);
			}


			if (file_dialog.showFileDialog("Export Data", imgui_addons::ImGuiFileBrowser::DialogMode::SAVE, ImVec2(700, 310), ".txt,.xy,.*")) {
				std::ofstream out(file_dialog.selected_path);

				for (int n = 0; n < data->ctrlPoints.size(); ++n)
					out << data->ctrlPoints[n].point[0] << "\t" << data->ctrlPoints[n].point[1] << std::endl;

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

			if (!data->ctrlPoints.size()) {
				data->isEnd = false;
				enable_edit = false;
			}

			// Context menu (under default mouse threshold)
			ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
			if (data->opt_enable_context_menu && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
				ImGui::OpenPopupContextItem("context");
			if (ImGui::BeginPopup("context")) {
				if (ImGui::MenuItem("Remove one", NULL, false, data->ctrlPoints.size() > 0)) {
					data->ctrlPoints.resize(data->ctrlPoints.size() - 1);
					data->_mx.resize(data->_mx.size() - 1);
					data->_my.resize(data->_my.size() - 1);
					selectedRight--;
				}
				if (ImGui::MenuItem("Remove all", NULL, false, data->ctrlPoints.size() > 0)) {
					data->ctrlPoints.clear();
					data->_mx.clear();
					data->_my.clear();
					selectedRight = -1;
				}
				if (ImGui::MenuItem("Add...", NULL, false)) {
					data->isEnd = false;
					enable_edit = false;
					selectedCtrlPoint = -1;
					selectedRight = data->ctrlPoints.size() - 1;
				}
				if (ImGui::MenuItem("Edit...", NULL, enable_edit, data->isEnd)) {
					// TODO: 显示手柄
					enable_edit = !enable_edit;
				}
				if (ImGui::MenuItem("Smooth...", NULL, modelType == 0, enable_edit && selectedRight!=-1)) {
					// 平滑顶点
					data->parametrizationType = 1;
					spdlog::info("1111111:{}", selectedRight);
					modelType = 0;
				}
				if (ImGui::MenuItem("Straight line...", NULL, modelType == 1, enable_edit && selectedRight != -1)) {
					// 直线顶点
					data->parametrizationType = 2;
					spdlog::info("222222:{}", selectedRight);
					modelType = 1;
				}
				if (ImGui::MenuItem("Corner...", NULL, modelType == 2, enable_edit && selectedRight != -1)) {
					// 角部顶点
					data->parametrizationType = 3;
					spdlog::info("3333333:{}", selectedRight);
					modelType = 2;
				}
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
			if (!enable_edit) {
				for (int n = 0; n < data->ctrlPoints.size(); ++n) {
					draw_list->AddCircleFilled(ImVec2(origin.x + data->ctrlPoints[n].point[0], origin.y + data->ctrlPoints[n].point[1]), 4, IM_COL32(255, 255, 255, 255));
				}
			} else {
				// 判断鼠标是否选中某点
				for (int i = 0; i < data->ctrlPoints.size(); ++i) {
					draw_list->AddQuadFilled(ImVec2(origin.x + data->ctrlPoints[i].point[0] + r, origin.y + data->ctrlPoints[i].point[1] + r),
						ImVec2(origin.x + data->ctrlPoints[i].point[0] + r, origin.y + data->ctrlPoints[i].point[1] - r),
						ImVec2(origin.x + data->ctrlPoints[i].point[0] - r, origin.y + data->ctrlPoints[i].point[1] - r),
						ImVec2(origin.x + data->ctrlPoints[i].point[0] - r, origin.y + data->ctrlPoints[i].point[1] + r), IM_COL32(255, 255, 255, 255));
					if (selectedCtrlPoint < 0 && (mouse_pos_in_canvas - data->ctrlPoints[i].point).norm2() < 40) {
						ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
						if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
							selectedCtrlPoint = i;
						}
						if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
							selectedRight = i;
							spdlog::info("000000:{}", selectedRight);
						}
					}
				}
			}


			if (selectedCtrlPoint > -1 ) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
					//data->ctrlPoints[selectedCtrlPoint].point = mouse_pos_in_canvas;
					
					spdlog::info("bbb:{}", selectedCtrlPoint);

					// 计算曲线
					ImVec2 tempBSP[MAX_PLOT_NUM_POINTS];
					std::vector<Ubpa::pointf2> tempcpoints;
					tempcpoints.clear();
					for (int i = 0; i < data->ctrlPoints.size(); ++i) {
						tempcpoints.push_back(data->ctrlPoints[i].point);
					}
					tempcpoints[selectedCtrlPoint] = mouse_pos_in_canvas;

					Eigen::VectorXf temppara = parametrization(tempcpoints, data->parametrizationType);
					int tempp_index = 0;
					for (int segment_idx = 0; segment_idx < tempcpoints.size() - 1; ++segment_idx) {
						for (float t = temppara[segment_idx]; t <= temppara[segment_idx + 1] && tempp_index < MAX_PLOT_NUM_POINTS; t += 0.002) {
							draw_list->AddCircleFilled(ImVec2(Curve::interpolationBSpline(tempcpoints, t, temppara, segment_idx, &(data->_mx), &(data->_my)) + origin), 1, IM_COL32(255, 255, 255, 255));
						}
						draw_list->AddQuad(ImVec2(origin.x + mouse_pos_in_canvas[0] + r, origin.y + mouse_pos_in_canvas[1] + r),
							ImVec2(origin.x + mouse_pos_in_canvas[0] + r, origin.y + mouse_pos_in_canvas[1] - r),
							ImVec2(origin.x + mouse_pos_in_canvas[0] - r, origin.y + mouse_pos_in_canvas[1] - r),
							ImVec2(origin.x + mouse_pos_in_canvas[0] - r, origin.y + mouse_pos_in_canvas[1] + r), IM_COL32(255, 255, 255, 255));
					}
				}
				spdlog::info("aaa:{}", selectedCtrlPoint);
				if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
					data->ctrlPoints[selectedCtrlPoint].point = mouse_pos_in_canvas;
					selectedCtrlPoint = -1;
					spdlog::info("ccc:{}", selectedCtrlPoint);
				}

			}


			if (data->ctrlPoints.size()) {
				// 计算曲线
				ImVec2 BSP[MAX_PLOT_NUM_POINTS];
				std::vector<Ubpa::pointf2> cpoints;
				cpoints.clear();
				for (int i = 0; i < data->ctrlPoints.size(); ++i) {
					cpoints.push_back(data->ctrlPoints[i].point);
				}
				std::vector<float> _mx = data->_mx;
				std::vector<float> _my = data->_my;

				// 加入if语句是因为当鼠标点击完还没来得及移动的时候，mouse_pos_in_canvas与back()点一样
				// 导致各参数化两点重合时，出现问题，比如弦长参数化会出现分母为0，会呈现出短暂的闪屏现象
				if (!data->isEnd && mouse_pos_in_canvas != data->ctrlPoints.back().point) {
					cpoints.push_back(mouse_pos_in_canvas);
					_mx.push_back(0.0f);
					_my.push_back(0.0f);
				}
				Eigen::VectorXf para = parametrization(cpoints, data->parametrizationType);
				int p_index = 0;
				for (int segment_idx = 0; segment_idx < cpoints.size() - 1; ++segment_idx) {
					for (float t = para[segment_idx]; t <= para[segment_idx + 1] && p_index < MAX_PLOT_NUM_POINTS; t += 0.001) {
						Ubpa::pointf2 bs_point = Curve::interpolationBSpline(cpoints, t, para, segment_idx, &_mx, &_my);
						BSP[p_index++] = ImVec2(bs_point + origin);
					}
				}
				draw_list->AddPolyline(BSP, p_index, IM_COL32(0, 255, 0, 255), false, 2.0f);
			}
			draw_list->PopClipRect();
		}
		ImGui::End();
	});
}

