#pragma once

#include <UGM/UGM.h>

struct CanvasData {
	std::vector<Ubpa::pointf2> points;	// ��ֵ��ľ�������x-y
	Ubpa::valf2 scrolling{ 0.f,0.f };
	bool opt_enable_grid{ true };
	bool opt_enable_context_menu{ true };

	int parametrizationType = 0;

	bool importData{ false };
	bool exportData{ false };

	bool isEnd{ false };

	std::vector<float> leftDerivativeX;		// x(t_i)������
	std::vector<float> rightDerivativeX;	// x(t_i)���ҵ���
	std::vector<float> leftDerivativeY;		// y(t_i)������
	std::vector<float> rightDerivativeY;	// y(t_i)���ҵ���

};


#include "details/CanvasData_AutoRefl.inl"
