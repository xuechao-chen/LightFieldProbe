#pragma once
#include "Sequence.h"
#include "Prime.h"

class Hammersley :
	public Sequence
{
public:
	Hammersley(unsigned d = 0, unsigned a = 0);
	Hammersley(std::vector<unsigned> b, unsigned a);
	virtual ~Hammersley();
private:
	std::vector<unsigned> bases;

	double radicalInversion(unsigned base, unsigned index);
	std::vector<double> getHammersleyPoint(unsigned index);
	void calcSequence();
};

