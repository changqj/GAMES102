#pragma once
#include <UGM/UGM.h>
#include <Eigen/Dense>

namespace Fitting {
	Eigen::VectorXf Interpolation_PolynomialBaseFunction(
		std::vector<Ubpa::pointf2> points) {
		int n = points.size();

		Eigen::MatrixXf normal_equation = Eigen::MatrixXf::Zero(n, n);
		Eigen::VectorXf y = Eigen::VectorXf::Zero(n);
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j)
				normal_equation(i, j) = pow((points[i][0]), j);
			y(i) = points[i][1];
		}
		return normal_equation.inverse() * y;
	}

	float GaussBaseFunction(float x, float xi, float sigma) {
		return expf(-(x - xi) * (x - xi) / (2 * sigma * sigma));
	}

	Eigen::VectorXf Interpolation_GaussBaseFunction(std::vector<Ubpa::pointf2> points, float sigma = 1) {
		int n = points.size();

		Eigen::MatrixXf normal_equation = Eigen::MatrixXf::Zero(n + 1, n + 1);

		Eigen::VectorXf y = Eigen::VectorXf::Zero(n + 1);

		for (int i = 0; i < n; ++i) {
			normal_equation(i, 0) = 1;
			for (int j = 1; j < n + 1; ++j)
				normal_equation(i, j) = GaussBaseFunction(points[i][0], points[j - 1][0], sigma);
			y(i) = points[i][1];
		}
		normal_equation(n, 0) = 1;
		for (int j = 1; j < n + 1; ++j)
			normal_equation(n, j) = GaussBaseFunction((points[n - 1][0] + points[n - 2][0]) / 2.0f, points[j - 1][0], sigma);
		y(n) = (points[n - 1][1] + points[n - 2][1]) / 2.0f;


		//return normal_equation.colPivHouseholderQr().solve(y);
		return normal_equation.inverse() * y;
	}

	Eigen::VectorXf Approximation_LeastSquare(std::vector<Ubpa::pointf2> points, int order = 3) {
		int n = points.size();

		Eigen::MatrixXf normal_equation = Eigen::MatrixXf::Zero(n, order + 1);

		Eigen::VectorXf y = Eigen::VectorXf::Zero(n);

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j <= order; ++j)
				normal_equation(i, j) = pow((points[i][0]), j);
			y(i) = points[i][1];
		}

		//return (normal_equation.transpose() * normal_equation).colPivHouseholderQr().solve(normal_equation.transpose() * y);
		return (normal_equation.transpose() * normal_equation).inverse() * (normal_equation.transpose() * y);
	}

	Eigen::VectorXf Approximation_RidgeRegression(std::vector<Ubpa::pointf2> points, int order = 3, float lambda = 0.5) {
		int n = points.size();

		Eigen::MatrixXf normal_equation = Eigen::MatrixXf::Zero(n, order + 1);

		Eigen::VectorXf y = Eigen::VectorXf::Zero(n);

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j <= order; ++j)
				normal_equation(i, j) = pow((points[i][0]), j);
			y(i) = points[i][1];
		}
		Eigen::MatrixXf I;
		I.setIdentity(order + 1, order + 1);
		//return (normal_equation.transpose() * normal_equation + I * lambda).colPivHouseholderQr().solve(normal_equation.transpose() * y);
		return (normal_equation.transpose() * normal_equation + I * lambda).inverse() * (normal_equation.transpose() * y);
	}
}