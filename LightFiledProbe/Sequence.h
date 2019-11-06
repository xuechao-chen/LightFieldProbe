#pragma once
#include <vector>
#include <fstream>
#include <iomanip>
#include <string>
#include <iostream>

# define PI           3.14159265358979323846

class Sequence {
public:
	Sequence(unsigned d = 0, unsigned a = 0);
	virtual ~Sequence();
	
	unsigned dimension;
	unsigned amount;
	std::vector<std::vector<double>> points;
	std::vector <std::vector<double>> sphere_points;

	void samplingOnSphere();
	void saveToFile(std::string file_name, bool is_sphere=false);
};