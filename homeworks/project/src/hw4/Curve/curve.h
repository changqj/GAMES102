#pragma once
#include <UGM/UGM.h>
#include <Eigen/Dense>

/**********************************************************************************
/// @file       curve.h
/// @author     Qingjun Chang
/// @date       2020.11.04
/// @brief      ����������ֵ����������أ�
/// @details    
**********************************************************************************/

#define EPSILON 1E-15
void GaussSeidel(std::vector<float> h, std::vector<float> u, std::vector<float> v, std::vector<float> *M0);
namespace Curve {
	/*
	/// @brief      
	/// @details    
	/// @param[in]  : derivative : ��x,��y,��x,��y
	/// @return     
	/// @attention  Mx.My��ʾ��صĳ�ֵ��������ָ���������Ϊ�˱�֤һ���õĵ�����ʼֵ
	*/
	Ubpa::pointf2 interpolationBSpline(std::vector<Ubpa::pointf2> points, float t0, Eigen::VectorXf t,int segment_index, std::vector<float> *Mx, std::vector<float> *My) {

		int n = points.size();

		if (points.size()==2)
		{
			// ����
			//derivative = (std::make_pair(std::make_pair(0,0),std::make_pair(points[1][0]-points[0][0],points[1][1]-points[0][1])));
			//derivative->push_back(std::make_pair(std::make_pair(0,0),std::make_pair(points[1][0]-points[0][0],points[1][1]-points[0][1])));

			return Ubpa::pointf2((t[1] - t0) / (t[1] - t[0]) * points[0][0] + (t0 - t[0]) / (t[1] - t[0]) * points[1][0], (t[1] - t0) / (t[1] - t[0]) * points[0][1] + (t0 - t[0]) / (t[1] - t[0]) * points[1][1]);
		}

		if (points.size() > 2) {
			std::vector<float> h, u, v_x, v_y;
			h.push_back(t[1] - t[0]);
			for (int i = 1; i < n - 1; ++i) {
				h.push_back(t[i + 1] - t[i]);
				u.push_back(2 * (t[i + 1] - t[i - 1]));
				v_x.push_back(6.0f / h[i] * (points[i + 1][0] - points[i][0]) - 6.0f / h[i - 1] * (points[i][0] - points[i - 1][0]));
				v_y.push_back(6.0f / h[i] * (points[i + 1][1] - points[i][1]) - 6.0f / h[i - 1] * (points[i][1] - points[i - 1][1]));
			}
			GaussSeidel(h, u, v_x, Mx);
			GaussSeidel(h, u, v_y, My);

			// ����


			std::vector<float> _mx = *Mx;
			std::vector<float> _my = *My;



			float x = _mx[segment_index] / (6 * (t[segment_index + 1] - t[segment_index])) * powf(t[segment_index + 1] - t0, 3);
			x += _mx[segment_index + 1] / (6 * (t[segment_index + 1] - t[segment_index])) * powf(t0 - t[segment_index], 3);
			x += (points[segment_index + 1][0] / (t[segment_index + 1] - t[segment_index]) - _mx[segment_index + 1] * (t[segment_index + 1] - t[segment_index]) / 6) * (t0 - t[segment_index]);
			x += (points[segment_index][0] / (t[segment_index + 1] - t[segment_index]) - _mx[segment_index] * (t[segment_index + 1] - t[segment_index]) / 6) * (t[segment_index + 1] - t0);
			float y = _my[segment_index] / (6 * (t[segment_index + 1] - t[segment_index])) * powf(t[segment_index + 1] - t0, 3);
			y += _my[segment_index + 1] / (6 * (t[segment_index + 1] - t[segment_index])) * powf(t0 - t[segment_index], 3);
			y += (points[segment_index + 1][1] / (t[segment_index + 1] - t[segment_index]) - _my[segment_index + 1] * (t[segment_index + 1] - t[segment_index]) / 6) * (t0 - t[segment_index]);
			y += (points[segment_index][1] / (t[segment_index + 1] - t[segment_index]) - _my[segment_index] * (t[segment_index + 1] - t[segment_index]) / 6) * (t[segment_index + 1] - t0);

			return Ubpa::pointf2(x, y);
		}

		return points.back();
	}
}


/*
/// @brief      ��˹-���¶�������
/// @details    
/// @param[in]  : 
/// @return     
/// @attention  
*/
void GaussSeidel(std::vector<float> h, std::vector<float> u, std::vector<float> v, std::vector<float>* M) {
	std::vector<float> _M0 = *M;

	while (true) {
		for (int i = 0; i < u.size(); ++i) {
			(*M)[i + 1] = (v[i] - h[i] * (*M)[i] - h[i + 1] * _M0[i + 2]) / u[i];
		}

		// ����ֹͣ����
		if ((Eigen::VectorXf::Map((*M).data(), (*M).size()) - Eigen::VectorXf::Map(_M0.data(), _M0.size())).norm() < EPSILON) {
			break;
		}
		_M0 = *M;
	}
}