#pragma once
#include <UGM/UGM.h>
#include <Eigen/Dense>

namespace Parametrization {
	Eigen::VectorXf chord(std::vector<Ubpa::pointf2> points) {

		int n = points.size();
		Eigen::VectorXf y = Eigen::VectorXf::Zero(n);

		for (int i = 1; i < n; ++i)
			y[i] = y[i - 1] + (points[i] - points[i - 1]).norm();

		y = y / y[n - 1];

		return y;
	}
}