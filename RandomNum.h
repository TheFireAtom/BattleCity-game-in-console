#ifndef RANDOM_NUM_H
#define RANDOM_NUM_H

#include <cstdint>
#include <chrono>

class XorShift32 {
private:
	uint32_t state;

public:
	explicit XorShift32();
	uint32_t randNum();
	uint32_t rangeNum(uint32_t min, uint32_t max);
};

#endif // RANDOM_NUM_H