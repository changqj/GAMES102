#pragma once
#include <UGM/UGM.h>
#include <Eigen/Dense>
//#include "spdlog/spdlog.h"  // ����ʹ��
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
	Ubpa::pointf2 interpolationBSpline(std::vector<Ubpa::pointf2> points, float t0, Eigen::VectorXf t,int segment_index, std::vector<float> *Mx, std::vector<float> *My, std::vector<std::pair<Ubpa::pointf2, Ubpa::pointf2>> *derivative,bool validDerivative) {

		int n = points.size();

		if (!validDerivative) {
			if (points.size() == 2) {
				// ����
				(*derivative)[0].second = Ubpa::pointf2(points[1][0] - points[0][0], points[1][1] - points[0][1]);
				(*derivative)[1].first = Ubpa::pointf2(points[1][0] - points[0][0], points[1][1] - points[0][1]);

				return Ubpa::pointf2((t[1] - t0) / (t[1] - t[0]) * points[0][0] + (t0 - t[0]) / (t[1] - t[0]) * points[1][0], (t[1] - t0) / (t[1] - t[0]) * points[0][1] + (t0 - t[0]) / (t[1] - t[0]) * points[1][1]);
			}

			if (points.size() > 2) {
				float x, y;
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

				// һ�׵���
				auto derivativeFun = [=](int i, int model)
					-> Ubpa::pointf2 {
					if (model == 2) {
						return Ubpa::pointf2(-(*Mx)[i] * h[i] / 3.0 - (*Mx)[i + 1] * h[i] / 6.0 - points[i][0] / h[i] + points[i + 1][0] / h[i], -(*My)[i] * h[i] / 3.0 - (*My)[i + 1] * h[i] / 6.0 - points[i][1] / h[i] + points[i + 1][1] / h[i]);
					}
					if (model == 1) {
						return Ubpa::pointf2(h[i - 1] * (*Mx)[i - 1] / 6.0f + (*Mx)[i] * h[i - 1] / 3.0f - points[i - 1][0] / h[i - 1] + points[i][0] / h[i - 1], h[i - 1] * (*My)[i - 1] / 6.0f + (*My)[i] * h[i - 1] / 3.0f - points[i - 1][1] / h[i - 1] + points[i][1] / h[i - 1]);
					}
				};

				// ����
				(*derivative)[segment_index].second = derivativeFun(segment_index, 2);
				(*derivative)[segment_index + 1].first = derivativeFun(segment_index + 1, 1);

				std::vector<float> _mx = *Mx;
				std::vector<float> _my = *My;

				auto fun = [=](std::vector<float>& _m, int i, float yi_1, float yi)
					-> float {
					return _m[i] / (6 * (h[i])) * powf(t[i + 1] - t0, 3) + _m[i + 1] / (6 * (h[i])) * powf(t0 - t[i], 3) + (yi_1 / (h[i]) - _m[i + 1] * (h[i]) / 6) * (t0 - t[i]) + (yi / (h[i]) - _m[i] * (h[i]) / 6) * (t[i + 1] - t0);
				};

				x = fun(_mx, segment_index, points[segment_index + 1][0], points[segment_index][0]);
				y = fun(_my, segment_index, points[segment_index + 1][1], points[segment_index][1]);
				return Ubpa::pointf2(x, y);
			}

			return points.back();
		}

		if (validDerivative)
		{
			float x, y;
			auto A = [=](int i) -> Eigen::Matrix4f {
				Eigen::Matrix4f a;
				a << 1, t[i], t[i] * t[i], t[i] * t[i] * t[i],
					1, t[i + 1], t[i + 1] * t[i + 1], t[i + 1] * t[i + 1] * t[i + 1],
					0, 1, 2 * t[i], 3 * t[i] * t[i],
					0, 1, 2 * t[i + 1], 3 * t[i + 1] * t[i + 1];
				return a;
			};
			auto B = [=](int i, int xy) -> Eigen::Vector4f {
				Eigen::Vector4f b;
				b << points[i][xy],
					points[i + 1][xy],
					(*derivative)[i].second[xy],
					(*derivative)[i + 1].first[xy];
				return b;
			};

			// ��ά�������Eigen������Ӱ�콻���ӳ�
			Eigen::Vector4f ax = A(segment_index).colPivHouseholderQr().solve(B(segment_index, 0));
			Eigen::Vector4f ay = A(segment_index).colPivHouseholderQr().solve(B(segment_index, 1));

			x = ax[0] + ax[1] * t0 + ax[2] * t0 * t0 + ax[3] * t0 * t0 * t0;
			y = ay[0] + ay[1] * t0 + ay[2] * t0 * t0 + ay[3] * t0 * t0 * t0;
			return Ubpa::pointf2(x, y);
		}
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