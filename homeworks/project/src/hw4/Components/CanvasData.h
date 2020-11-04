#pragma once

#include <UGM/UGM.h>

struct CanvasData {
	std::vector<Ubpa::pointf2> points;	// 型值点的具体坐标x-y
	Ubpa::valf2 scrolling{ 0.f,0.f };
	bool opt_enable_grid{ true };
	bool opt_enable_context_menu{ true };

	int parametrizationType = 0;

	bool importData{ false };
	bool exportData{ false };

	bool isEnd{ false };

	std::vector<float> leftDerivativeX;		// x(t_i)的左导数
	std::vector<float> rightDerivativeX;	// x(t_i)的右导数
	std::vector<float> leftDerivativeY;		// y(t_i)的左导数
	std::vector<float> rightDerivativeY;	// y(t_i)的右导数

};


#include "details/CanvasData_AutoRefl.inl"
