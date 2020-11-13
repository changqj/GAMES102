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
constexpr auto SCALE = 20.0f;

imgui_addons::ImGuiFileBrowser file_dialog;

bool enable_edit = false;

int selectedCtrlPoint = -1;

float r = 5.0f;	// 方形点的半径

std::vector<int> modelType;	// 0: 平滑顶点；  1：直线顶点；   2：角部顶点
int selectedRight = -1;

bool validDerivative = false;
int leftOrRight = -1;	// 0: 表示激活当前点左导数手柄 1:表示激活右导数手柄


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

void drawQuad(ImDrawList* draw_list, Ubpa::pointf2 center, float r, bool isFilled, ImU32 col) {
	isFilled ? draw_list->AddQuadFilled(ImVec2(center[0] + r, center[1] + r), ImVec2(center[0] + r, center[1] - r), ImVec2(center[0] - r, center[1] - r), ImVec2(center[0] - r, center[1] + r), col) : draw_list->AddQuad(ImVec2(center[0] + r, center[1] + r), ImVec2(center[0] + r, center[1] - r), ImVec2(center[0] - r, center[1] - r), ImVec2(center[0] - r, center[1] + r), col);
}

void drawHandel(ImDrawList* draw_list, Ubpa::pointf2 center, std::pair<Ubpa::pointf2, Ubpa::pointf2> derivative, ImVec2 origin, float r, ImU32 col1, ImU32 col2, bool isFirst = false, bool isLast = false) {
	Ubpa::pointf2 d;
	// 除了最后一个点，都画右导数
	if (!isLast) {
		d = ImVec2(derivative.second[0] / SCALE + center[0] + origin.x, derivative.second[1] / SCALE + center[1] + origin.y);
		drawQuad(draw_list, d, r, false, col1);
		draw_list->AddLine(center + origin, d, col2);
	}

	// 除了第一个点，都画左导数
	if (!isFirst) {
		d = ImVec2(-derivative.first[0] / SCALE + center[0] + origin.x, -derivative.first[1] / SCALE + center[1] + origin.y);
		drawQuad(draw_list, d, r, false, col1);
		draw_list->AddLine(d, center + origin, col2);
	}
}

// D1 要更改的那个
Ubpa::pointf2 Straight_line(Ubpa::pointf2 center, Ubpa::pointf2 D, Ubpa::pointf2 D1) {
	float l,x, y;
	if ((D - center).norm() < EPSILON)
		return D1;
	spdlog::info("d1-c:{}", (D1 - center).norm()*((D1 - center).norm()));
	spdlog::info("d-c:{}", (D1 - center).norm2());
	l = (D1 - center).norm()/(D-center).norm();
	//spdlog::info("l:{}", l);

	x = l * (D[0] - center[0]) + center[0];
	y = l * (D[1] - center[1]) + center[1];

	return Ubpa::pointf2(x, y);
}

void CanvasSystem::OnUpdate(Ubpa::UECS::Schedule& schedule) {
	spdlog::set_pattern("[%H:%M:%S] %v");
	//spdlog::set_pattern("%+"); // back to default format
	
	schedule.RegisterCommand([](Ubpa::UECS::World* w) {
		bool enable_handel = true;
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
				ImGui::BulletText("Mouse Left: click to add points.");
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
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { 
				data->isEnd = true;
				modelType[0] = 2;  // 首尾顶点模式都为角部顶点
				modelType.back() = 2;
			}	// 双击结束
			if (is_hovered && !data->isEnd && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				data->points.push_back(mouse_pos_in_canvas);
				data->_mx.push_back(0.0f);
				data->_my.push_back(0.0f);
				data->derivative.push_back(std::make_pair(Ubpa::pointf2(0.0f, 0.0f), Ubpa::pointf2(0.0f, 0.0f)));
				modelType.push_back(0);
				selectedRight++;
				spdlog::info("Point added at: {}, {}", data->points.back()[0], data->points.back()[1]);
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
				data->points.clear();
				validDerivative = false;
				modelType.clear();
				std::ifstream in(file_dialog.selected_path);
				float x, y;
				while (in >> x >> y) {
					data->points.push_back(ImVec2(x, y));
					data->_mx.push_back(0.0f);
					data->_my.push_back(0.0f);
					data->derivative.push_back(std::make_pair(Ubpa::pointf2(0.0f, 0.0f), Ubpa::pointf2(0.0f, 0.0f)));
					modelType.push_back(0);
				}
				selectedRight = data->points.size() - 1;
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

			if (!data->points.size()) {
				data->isEnd = false;
				enable_edit = false;
			}

			// Context menu (under default mouse threshold)
			ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
			if (data->opt_enable_context_menu && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
				ImGui::OpenPopupContextItem("context");
			if (ImGui::BeginPopup("context")) {
				if (ImGui::MenuItem("Remove one", NULL, false, data->points.size() > 0)) {
					data->points.resize(data->points.size() - 1);
					data->_mx.resize(data->_mx.size() - 1);
					data->_my.resize(data->_my.size() - 1);
					data->derivative.resize(data->derivative.size() - 1);
					modelType.resize(modelType.size() - 1);
					if (selectedRight == data->points.size()) {
						selectedRight--;
					}
				}
				if (ImGui::MenuItem("Remove all", NULL, false, data->points.size() > 0)) {
					data->points.clear();
					data->_mx.clear();
					data->_my.clear();
					data->derivative.clear();
					modelType.clear();
					validDerivative = false;
					selectedRight = -1;
				}
				if (ImGui::MenuItem("Add...", NULL, false)) {
					data->isEnd = false;
					enable_edit = false;
					selectedCtrlPoint = -1;
					selectedRight = data->points.size()-1;
				}
				if (ImGui::MenuItem("Edit...", NULL, enable_edit, data->isEnd)) {
					// TODO: 显示手柄
					validDerivative = true;
					enable_edit = !enable_edit;
				}
				if (ImGui::MenuItem("Smooth...", NULL, modelType[selectedRight] == 0, enable_edit && selectedRight>0 && selectedRight != data->points.size() - 1)) {
					// 平滑顶点
					spdlog::info("1111111:{}", selectedRight);
					
					// 如果左边手柄激活 
					if (!leftOrRight && selectedRight!=0) {
						data->derivative[selectedRight].second = data->derivative[selectedRight].first;
					}
					if (leftOrRight && selectedRight != data->points.size() - 1) {
						data->derivative[selectedRight].first = data->derivative[selectedRight].second;
					}
					modelType[selectedRight] = 0;
				}
				if (ImGui::MenuItem("Straight line...", NULL, modelType[selectedRight] == 1, enable_edit && selectedRight > 0 && selectedRight!=data->points.size()-1)) {
					// 直线顶点
					spdlog::info("222222:{}", selectedRight);
					// 如果左边手柄激活 
					if (!leftOrRight && selectedRight != 0) {
						data->derivative[selectedRight].second = Straight_line(data->points[selectedRight], data->derivative[selectedRight].first, data->derivative[selectedRight].second);
					}
					if (leftOrRight && selectedRight != data->points.size() - 1) {
						data->derivative[selectedRight].first = Straight_line(data->points[selectedRight], data->derivative[selectedRight].second, data->derivative[selectedRight].first);
					}
					modelType[selectedRight] = 1;
				}
				if (ImGui::MenuItem("Corner...", NULL, modelType[selectedRight] == 2, enable_edit && selectedRight != -1)) {
					// 角部顶点
					spdlog::info("3333333:{}", selectedRight);
					modelType[selectedRight] = 2;
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
				for (int n = 0; n < data->points.size(); ++n) {
					draw_list->AddCircleFilled(ImVec2(origin.x + data->points[n][0], origin.y + data->points[n][1]), 4, IM_COL32(255, 255, 255, 255));
				}
			} else {
				// 判断鼠标是否选中某点
				for (int i = 0; i < data->points.size(); ++i) {
					drawQuad(draw_list, ImVec2(origin.x + data->points[i][0], origin.y + data->points[i][1]), r-1, selectedRight == i, IM_COL32(255, 255, 255, 255));
					if (selectedCtrlPoint < 0 && (mouse_pos_in_canvas - data->points[i]).norm2() < 20) {
						ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
						if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
							selectedCtrlPoint = i;
							selectedRight = i;
						}
						if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { selectedRight = i; }
					}
				}
			}

			// 跑断鼠标是否指向手柄点
			if (enable_edit && selectedRight > -1 && selectedCtrlPoint == -1) {
				if ((mouse_pos_in_canvas - pointf2(-data->derivative[selectedRight].first[0] / SCALE + data->points[selectedRight][0], -data->derivative[selectedRight].first[1] / SCALE + data->points[selectedRight][1])).norm2() < 60) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
					leftOrRight = 0;
				}
				if ((mouse_pos_in_canvas - pointf2(data->derivative[selectedRight].second[0] / SCALE + data->points[selectedRight][0], data->derivative[selectedRight].second[1] / SCALE + data->points[selectedRight][1])).norm2() < 60) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
					leftOrRight = 1;
				}
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
					spdlog::info("----> handle drag");
					if (leftOrRight == 0) {
						data->derivative[selectedRight].first = Ubpa::pointf2(SCALE * (data->points[selectedRight][0] - mouse_pos_in_canvas[0]), SCALE * (data->points[selectedRight][1] - mouse_pos_in_canvas[1]));
						if (modelType[selectedRight] == 0) {
							// 平滑
							data->derivative[selectedRight].second = data->derivative[selectedRight].first;
						}
						if (modelType[selectedRight] == 1) {
							// 直线
							data->derivative[selectedRight].second = Straight_line(data->points[selectedRight], data->derivative[selectedRight].first, data->derivative[selectedRight].second);
						}
						validDerivative = true;
						spdlog::info("aaaaa");
					}
					if (leftOrRight == 1) {
						data->derivative[selectedRight].second = Ubpa::pointf2(SCALE * (-data->points[selectedRight][0] + mouse_pos_in_canvas[0]), SCALE * (-data->points[selectedRight][1] + mouse_pos_in_canvas[1]));
						if (modelType[selectedRight] == 0) {
							data->derivative[selectedRight].first = data->derivative[selectedRight].second;
						}
						if (modelType[selectedRight] == 1) {
							data->derivative[selectedRight].first = Straight_line(data->points[selectedRight], data->derivative[selectedRight].second, data->derivative[selectedRight].first);
						}
						validDerivative = true;
					}
				}
			}

			if (selectedCtrlPoint > -1 ) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				// 拖动实心点的事件
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
					// 计算曲线
					std::vector<Ubpa::pointf2> tempcpoints = data->points;
					tempcpoints[selectedCtrlPoint] = mouse_pos_in_canvas;
					Eigen::VectorXf temppara = parametrization(tempcpoints, data->parametrizationType);
					for (int segment_idx = 0; segment_idx < tempcpoints.size() - 1; ++segment_idx) {
						for (float t = temppara[segment_idx]; t <= temppara[segment_idx + 1]; t += 0.001) {
							draw_list->AddCircleFilled(ImVec2(Curve::interpolationBSpline(tempcpoints, t, temppara, segment_idx, &(data->_mx), &(data->_my), &(data->derivative), true) + origin), 1, IM_COL32(255, 255, 255, 255));
						}
					}
					drawQuad(draw_list, ImVec2(origin.x + mouse_pos_in_canvas[0], origin.y + mouse_pos_in_canvas[1]), r-1, true, IM_COL32(255, 255, 255, 255));
					// 画代表一阶导的交互手柄
					enable_handel = false;
					drawHandel(draw_list, mouse_pos_in_canvas, data->derivative[selectedRight], origin, r+3, IM_COL32(255, 255, 0, 255), IM_COL32(255, 255, 255, 255), selectedRight == 0, selectedRight == data->points.size() - 1);
				}
				spdlog::info("aaa:{}", selectedCtrlPoint);
				if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
					data->points[selectedCtrlPoint] = mouse_pos_in_canvas;
					selectedCtrlPoint = -1;
				}

			}


			if (data->points.size()) {
				// 计算曲线
				ImVec2 BSP[MAX_PLOT_NUM_POINTS];
				
				std::vector<float> _mx = data->_mx;
				std::vector<float> _my = data->_my;
				std::vector< Ubpa::pointf2> cpoints = data->points;
				std::vector<std::pair<Ubpa::pointf2, Ubpa::pointf2>> _derivative = data->derivative;

				// 加入if语句是为了避免当鼠标点击完还没来得及移动的时候，mouse_pos_in_canvas与back()点一样
				// 导致各参数化两点重合时，出现问题，比如弦长参数化会出现分母为0，会呈现出短暂的闪屏现象
				bool added = false;
				if (!data->isEnd && mouse_pos_in_canvas != cpoints.back()) {
					cpoints.push_back(mouse_pos_in_canvas);
					_mx.push_back(0.0f);
					_my.push_back(0.0f);
					_derivative.push_back(std::make_pair(Ubpa::pointf2(0.0f, 0.0f), Ubpa::pointf2(0.0f, 0.0f)));
					added = true;
				}
				Eigen::VectorXf para = parametrization(cpoints, data->parametrizationType);
				int p_index = 0;
				for (int segment_idx = 0; segment_idx < cpoints.size() - 1; ++segment_idx) {
					for (float t = para[segment_idx]; t <= para[segment_idx + 1] && p_index < MAX_PLOT_NUM_POINTS; t += 0.001) {
						Ubpa::pointf2 bs_point = Curve::interpolationBSpline(cpoints, t, para, segment_idx, &_mx, &_my, &_derivative, validDerivative);
						BSP[p_index++] = ImVec2(bs_point + origin);
					}
				}
				if (added) {
					_mx.pop_back();
					_my.pop_back();
					_derivative.pop_back();
				}
				data->_mx = _mx;
				data->_my = _my;
				data->derivative = _derivative;
				draw_list->AddPolyline(BSP, p_index, IM_COL32(0, 255, 0, 255), false, 1.0f);
				if (enable_edit && enable_handel) {
					drawHandel(draw_list, data->points[selectedRight], data->derivative[selectedRight], origin, r+3, IM_COL32(255, 255, 0, 255), IM_COL32(255, 255, 255, 255), selectedRight == 0, selectedRight == data->points.size() - 1);
				}
			}
			draw_list->PopClipRect();
		}
		ImGui::End();
	});
}

