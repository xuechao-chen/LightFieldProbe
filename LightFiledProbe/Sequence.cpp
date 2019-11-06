#include "Sequence.h"

Sequence::Sequence(unsigned d, unsigned a)
	: dimension(d), amount(a) {
	points.reserve(amount);
}

Sequence::~Sequence() {
}

void Sequence::saveToFile(std::string file_name, bool is_sphere) {
	std::ofstream outfile;
	outfile.exceptions(std::ifstream::badbit | std::ifstream::failbit);

	try {
		outfile.open(file_name);
		if (!is_sphere) {
			for (const auto &point : points) {
				for (const auto &i : point) {
					outfile << std::setprecision(12) << std::setiosflags(std::ios::fixed) << i << " ";
				}
				outfile << std::endl;
			}
		}
		else {
			for (const auto &point : sphere_points) {
				for (const auto &i : point) {
					outfile << std::setprecision(12) << std::setiosflags(std::ios::fixed) << i << " ";
				}
				outfile << std::endl;
			}
		}
	}
	catch (std::exception e) {
		std::cerr << "打开文件失败" << std::endl;
		exit(-1);
	}
}

// 将点坐标映射到球面上
void Sequence::samplingOnSphere() {
	double phi, t;
	double temp;

	if (2 == dimension) {
		sphere_points.reserve(amount);
		for (int i = 0; i != amount; ++i) {
			phi = points[i][0] * 2 * PI;
			t = points[i][1] * 2 - 1;
			//t = points[i][1];
			temp = sqrt(1 - pow(t, 2));

			sphere_points.push_back(std::vector<double>{temp * cos(phi), temp * sin(phi), t});
		}
	} else {
		std::cerr << "维度必须等于2" << std::endl;
	}
}
