#pragma once
#include <UGM/UGM.h>
#include <Eigen/Dense>

/**********************************************************************************
/// @file       curve.h
/// @author     Qingjun Chang
/// @date       2020.11.04
/// @brief      三次样条插值函数（三弯矩）
/// @details    
**********************************************************************************/

namespace Curve {
	/*
	/// @brief      
	/// @details    
	/// @param[in]  : 
	/// @return     
	/// @attention  
	*/
	Ubpa::pointf2 interpolationBSpline(std::vector<Ubpa::pointf2> points, float t0, Eigen::VectorXf t,int segment_index) {

		int n = points.size();

		if (points.size() > 2) {
			Eigen::MatrixXf A = Eigen::MatrixXf::Zero(n - 2, n - 2);
			Eigen::VectorXf vx = Eigen::VectorXf::Zero(n - 2);
			Eigen::VectorXf vy = Eigen::VectorXf::Zero(n - 2);

			for (int i = 1; i < n - 2; ++i) {
				A(i, i - 1) = t[i + 1] - t[i];	// h_i
				A(i - 1, i) = t[i + 1] - t[i];
			}
			for (int i = 0; i < n - 2; ++i) {
				A(i, i) = 2 * (t[i + 2] - t[i]);
				vx[i] = 6.0f / (t[i + 2] - t[i + 1]) * (points[i + 2][0] - points[i + 1][0]) - 6.0f / (t[i + 1] - t[i]) * (points[i + 1][0] - points[i][0]);
				vy[i] = 6.0f / (t[i + 2] - t[i + 1]) * (points[i + 2][1] - points[i + 1][1]) - 6.0f / (t[i + 1] - t[i]) * (points[i + 1][1] - points[i][1]);
			}

			Eigen::VectorXf Mx = Eigen::VectorXf::Zero(n);
			Eigen::VectorXf My = Eigen::VectorXf::Zero(n);
			Mx.block(1, 0, n - 2, 1) = A.colPivHouseholderQr().solve(vx);
			My.block(1, 0, n - 2, 1) = A.colPivHouseholderQr().solve(vy);

			float x = Mx[segment_index] / (6 * (t[segment_index + 1] - t[segment_index])) * powf(t[segment_index + 1] - t0, 3);
			x += Mx[segment_index + 1] / (6 * (t[segment_index + 1] - t[segment_index])) * powf(t0 - t[segment_index], 3);
			x += (points[segment_index + 1][0] / (t[segment_index + 1] - t[segment_index]) - Mx[segment_index + 1] * (t[segment_index + 1] - t[segment_index]) / 6) * (t0 - t[segment_index]);
			x += (points[segment_index][0] / (t[segment_index + 1] - t[segment_index]) - Mx[segment_index] * (t[segment_index + 1] - t[segment_index]) / 6) * (t[segment_index + 1] - t0);
			float y = My[segment_index] / (6 * (t[segment_index + 1] - t[segment_index])) * powf(t[segment_index + 1] - t0, 3);
			y += My[segment_index + 1] / (6 * (t[segment_index + 1] - t[segment_index])) * powf(t0 - t[segment_index], 3);
			y += (points[segment_index + 1][1] / (t[segment_index + 1] - t[segment_index]) - My[segment_index + 1] * (t[segment_index + 1] - t[segment_index]) / 6) * (t0 - t[segment_index]);
			y += (points[segment_index][1] / (t[segment_index + 1] - t[segment_index]) - My[segment_index] * (t[segment_index + 1] - t[segment_index]) / 6) * (t[segment_index + 1] - t0);

			return Ubpa::pointf2(x, y);
		}
	}
}