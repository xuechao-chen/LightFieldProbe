#include "Prime.h"


Prime::~Prime() {
}

// 判断n是否为质数
bool Prime::isPrime(unsigned n) {
	bool is_prime = false;
	int sqrt_of_n = static_cast<int>(sqrt(n));
	unsigned i;

	// 将n分别除以2至根号n，若能整除，则不是质数
	for (i = 2; i <= sqrt_of_n; ++i) {
		if (!(n % i))	break;
	}

	// 没有数可以整除n，n为质数
	if (i == 1 + sqrt_of_n)	is_prime = true;

	return is_prime;
}

// 获得包含前n个质数的向量
std::vector<unsigned> Prime::getPrimeSequence() {
	unsigned n = 2;
	unsigned count = amount;

	while (count) {
		if (isPrime(n)) {
			primes.push_back(n);
			--count;
		}
		++n;
	}

	return primes;
}
