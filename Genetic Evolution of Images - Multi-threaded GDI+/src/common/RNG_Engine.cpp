#include "common/RNG_Engine.h"

void RNG_ENGINE::Seed(unsigned int Seed)
{
	Twister.seed(Seed);
	Twister.discard(1000000);
}

unsigned int RNG_ENGINE::RandomFromOne(unsigned int Limit)
{
	Distribution.param(std::uniform_int_distribution<unsigned int>::param_type(1, Limit));
	return Distribution(Twister);
}

unsigned int RNG_ENGINE::RandomFromZero(unsigned int Limit)
{
	Distribution.param(std::uniform_int_distribution<unsigned int>::param_type(0, Limit));
	return Distribution(Twister);
} 