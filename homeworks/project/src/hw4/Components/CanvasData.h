#pragma once

#include <UGM/UGM.h>

struct CanvasData {
	std::vector<Ubpa::pointf2> points;
	std::vector<std::pair<Ubpa::pointf2, Ubpa::pointf2>> derivative;
	Ubpa::valf2 scrolling{ 0.f,0.f };
	bool opt_enable_grid{ false };
	bool opt_enable_context_menu{ true };

	int parametrizationType = 0;

	bool importData{ false };
	bool exportData{ false };

	bool isEnd{ false };

	std::vector<float> _mx;
	std::vector<float> _my;
};

#include "details/CanvasData_AutoRefl.inl"
