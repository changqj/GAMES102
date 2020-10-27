#pragma once
#include <UGM/UGM.h>
#include <Eigen/Dense>

namespace Fitting {

	Eigen::VectorXf Approximation_LeastSquare(std::vector<Ubpa::pointf2> points, int order = 3)
	{
		int n = points.size();

		Eigen::MatrixXf normal_equation = Eigen::MatrixXf::Zero(n, order + 1);

		Eigen::VectorXf y = Eigen::VectorXf::Zero(n);

		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j <= order; ++j)
				normal_equation(i, j) = pow((points[i][0]), j);
			y(i) = points[i][1];
		}

		//return (normal_equation.transpose() * normal_equation).colPivHouseholderQr().solve(normal_equation.transpose() * y);
		return (normal_equation.transpose() * normal_equation).inverse()*(normal_equation.transpose() * y);
	}
}