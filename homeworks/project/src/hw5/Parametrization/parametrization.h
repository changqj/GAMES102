#pragma once
#include <UGM/UGM.h>
#include <Eigen/Dense>


/**********************************************************************************
/// @file       parametrization.h
/// @author     Qingjun Chang
/// @date       2020.10.28
/// @brief      包含各种参数化方法
/// @details    
**********************************************************************************/

#define PI 3.1415926535
namespace Parametrization {
	/*
	/// @brief      弦长参数化
	/// @details    t_{i+1}-t_i = \Vert p_{i+1}-p_i\Vert
	/// @param[in]  points: 
	/// @return     参数化结果
	/// @attention  
	*/
	Eigen::VectorXf chordParameterization(std::vector<Ubpa::pointf2> points) {
		int n = points.size();
		Eigen::VectorXf y = Eigen::VectorXf::Zero(n);
		for (int i = 1; i < n; ++i)
			y[i] = y[i - 1] + (points[i] - points[i - 1]).norm();
		y /= y[n - 1];
		return y;
	}

	/*
	/// @brief      中心参数化
	/// @details    t_{i+1}-t_i = \sqrt{\Vert p_{i+1}-p_i\Vert}
	/// @param[in]  points: 
	/// @return     
	/// @attention  
	*/
	Eigen::VectorXf centripetalParameterization(std::vector<Ubpa::pointf2> points) {
		int n = points.size();
		Eigen::VectorXf y = Eigen::VectorXf::Zero(n);
		for (int i = 1; i < n; ++i)
			y[i] = y[i - 1] + sqrt((points[i] - points[i - 1]).norm());
		y /= y[n - 1];
		return y;
	}

	/*
	/// @brief      均匀参数化
	/// @details    t_{i+1}-t_i = k
	/// @param[in]  numOfPoints: 
	/// @return     
	/// @attention  
	*/
	Eigen::VectorXf uniformParameterization(int numOfPoints) {
		Eigen::VectorXf y = Eigen::VectorXf::Zero(numOfPoints);
		for (int i = 0; i < numOfPoints; ++i)
			y[i] = i;
		y /= y[numOfPoints - 1];
		return y;
	}


	/*
	/// @brief      Foley参数化
	/// @details    https://dergipark.org.tr/en/download/article-file/401586 公式(19)
	/// @param[in]  points: 
	/// @return     
	/// @attention  
	*/
	Eigen::VectorXf FoleyParameterization(std::vector<Ubpa::pointf2> points) {
		int n = points.size();
		Eigen::VectorXf y = Eigen::VectorXf::Zero(n);
		if (n == 2) { y[1] = 1; return y; }
		Eigen::VectorXf dist = Eigen::VectorXf::Zero(n-1);
		Eigen::VectorXf alpha = Eigen::VectorXf::Zero(n-1);
		for (int i = 0; i < n - 1; ++i) {
			dist[i] = (points[i + 1] - points[i]).norm();
		}
		for (int i = 1; i < n - 1; ++i) {
			float cosvalue = (points[i - 1] - points[i]).cos_theta(points[i + 1] - points[i]);
			cosvalue = std::min(cosvalue, 1.0f);
			cosvalue = std::max(cosvalue, -1.0f);
			alpha[i] = std::min(PI - acos(cosvalue), PI / 2);
		}
		y[1] = dist[0] * (1 + 1.5 * alpha[1] * dist[1] / (dist[0] + dist[1]));
		for (int i = 2; i < n - 1; ++i) {
			y[i] = y[i - 1] + dist[i - 1] * (1 + 1.5 * alpha[i - 1] * dist[i - 2] / (dist[i - 2] + dist[i - 1]) + 1.5 * alpha[i] * dist[i] / (dist[i - 1] + dist[i]));
		}
		y[n - 1] = y[n - 2] + dist[n - 2] * (1 + 1.5 * alpha[n - 2] * dist[n - 3] / (dist[n - 3] + dist[n - 2]));
		y /= y[n - 1];
		return y;
	}
}