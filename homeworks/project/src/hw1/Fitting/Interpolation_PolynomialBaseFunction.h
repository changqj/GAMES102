#pragma once
#include <UGM/UGM.h>
#include <Eigen/Dense>

namespace Fitting {
	Eigen::VectorXf Interpolation_PolynomialBaseFunction(
		std::vector<Ubpa::pointf2> points) {
		int n = points.size();

		Eigen::MatrixXf normal_equation = Eigen::MatrixXf::Zero(n,n);
		Eigen::VectorXf y = Eigen::VectorXf::Zero(n);
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j)
				normal_equation(i, j) = pow((points[i][0]), j);
			y(i) = points[i][1];
		}
		return normal_equation.inverse() * y;
	}
}