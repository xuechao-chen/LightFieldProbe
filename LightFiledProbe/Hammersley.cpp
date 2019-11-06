#include "Hammersley.h"


Hammersley::Hammersley(unsigned d, unsigned a) : Sequence(d, a) {
	Prime p(dimension - 1);
	bases = p.getPrimeSequence();
	calcSequence();
}

Hammersley::Hammersley(std::vector<unsigned> b, unsigned a) : Sequence(b.size() + 1, a) {
	bases = b;
	calcSequence();
}


Hammersley::~Hammersley()
{
}

// 计算索引index在基为base的radical inversion
double Hammersley::radicalInversion(unsigned base, unsigned index) {
	int weight = 1;					// 最高位的权重
	double result = 0;

	// 将index在base进制下逆序
	for (; index; index /= base) {
		result = result * base + index % base;
		weight *= base;
	}

	// 将结果移到小数点右侧
	return result / weight;
}

// 计算d维空间中索引为index在基为base的Hammersley点
std::vector<double> Hammersley::getHammersleyPoint(unsigned index) {
	std::vector<double> point;

	// 第一维是index除以点的总数
	point.push_back(static_cast<double>(index) / static_cast<double>(amount));

	// 依次计算出该点每一维的坐标值
	for (const auto &p : bases) {
		point.push_back(radicalInversion(p, index));
	}

	return point;
}

void Hammersley::calcSequence()
{
	for (unsigned i = 0; i < amount; ++i) {
		points.push_back(getHammersleyPoint(i));
	}
}