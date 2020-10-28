#pragma once

#include <UGM/UGM.h>

struct CanvasData {
	std::vector<Ubpa::pointf2> points;
	Ubpa::valf2 scrolling{ 0.f,0.f };
	bool opt_enable_grid{ true };
	bool opt_enable_context_menu{ true };
	bool adding_line{ false };

	bool enable_IP{ true };
	bool enable_IG{ true };
	bool enable_ALS{ true };
	bool enable_ARR{ true };

	int order_als = 1;
	float lambda = 1.0f;
	float sigma = 0.1f;
	int order_arr = 1;
};

#include "details/CanvasData_AutoRefl.inl"
