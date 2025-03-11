#include "RandomNum.h"

XorShift32::XorShift32() {
	uint32_t seed = static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	state = seed;
}

uint32_t XorShift32::randNum() {
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return state;
}

uint32_t XorShift32::rangeNum(uint32_t min, uint32_t max) {
	return (min + (randNum() % (max - min + 1)));
}
