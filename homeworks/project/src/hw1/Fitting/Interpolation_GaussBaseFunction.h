#pragma once
#include <UGM/UGM.h>
#include <Eigen/Dense>

namespace Fitting {

	float GaussBaseFunction(float x, float xi, float sigma) {
		return expf(-(x - xi) * (x - xi) / (2 * sigma * sigma));
	}

	Eigen::VectorXf Interpolation_GaussBaseFunction(std::vector<Ubpa::pointf2> points, float sigma = 1) {
		int n = points.size();

		Eigen::MatrixXf normal_equation = Eigen::MatrixXf::Ones(n + 1, n + 1);

		Eigen::VectorXf y = Eigen::VectorXf::Ones(n + 1);

		for (int i = 0; i < n; ++i) {
			normal_equation(i, 0) = 1;
			for (int j = 1; j < n + 1; ++j)
				normal_equation(i, j) = GaussBaseFunction(points[i][0], points[j - 1][0], sigma);
			y(i) = points[i][1];
		}

		//return normal_equation.colPivHouseholderQr().solve(y);
		return normal_equation.inverse() * y;
	}
}