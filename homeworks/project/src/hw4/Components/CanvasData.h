#pragma once

#include <UGM/UGM.h>

struct CtrlPoint {
	Ubpa::pointf2 point;
	float leftDerivativeX = 0.0f;
	float rightDerivativeX = 0.0f;
	float leftDerivativeY = 0.0f;
	float rightDerivativeY = 0.0f;
	CtrlPoint(const Ubpa::pointf2 p) { point = p; }
	CtrlPoint() { point = Ubpa::pointf2(); }
};

struct CanvasData {
	std::vector<CtrlPoint> ctrlPoints;
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
