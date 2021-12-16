#pragma once
#include <random>

class RNG_ENGINE
{
	std::mt19937 Twister;
	std::uniform_int_distribution<unsigned int> Distribution;

public:
	void Seed(unsigned int);
	unsigned int RandomFromOne(unsigned int);
	unsigned int RandomFromZero(unsigned int);
};