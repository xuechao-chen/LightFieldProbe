#pragma once
#include <vector>

class Prime {
public:
	Prime(unsigned n = 0) : amount(n) {}
	~Prime();

	std::vector<unsigned> getPrimeSequence();
private:
	std::vector<unsigned> primes;
	unsigned amount;

	bool isPrime(unsigned n);
};

