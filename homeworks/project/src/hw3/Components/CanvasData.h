#pragma once

#include <UGM/UGM.h>

struct CanvasData {
	std::vector<Ubpa::pointf2> points;
	Ubpa::valf2 scrolling{ 0.f,0.f };
	bool opt_enable_grid{ true };
	bool opt_enable_context_menu{ true };
	bool adding_line{ false };

	bool enable_IP{ false };
	bool enable_IG{ true };
	bool enable_ALS{ true };
	bool enable_ARR{ false };

	bool enable_RBF{ false };
	int layer = 10;
	float learning_rate = 0.002f;
	int step_num = 10001;

	int order_als = 1;
	float lambda = 1.0f;
	float sigma = 0.1f;
	int order_arr = 1;

	int parametrizationType = 0;

	bool importData{ false };
	bool exportData{ false };

	std::string python_interpreter = "";
};

#include "details/CanvasData_AutoRefl.inl"
