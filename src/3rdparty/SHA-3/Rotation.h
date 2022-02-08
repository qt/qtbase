#ifndef ROTATION_H_
#define ROTATION_H_

#include <cstdint>

// As we're not using assembly, we can't use the native rotation instructions
//  replace it with a small inline
static inline uint64_t rotateLeft(uint64_t x, int n)
{
	const unsigned int mask = (8*sizeof(x) - 1);  // assumes width is a power of 2.

	// assert ( (c<=mask) &&"rotate by type width or more");
	n &= mask;
	return (x << n) | (x >> ((-n)&mask));
}

static inline uint64_t rotateRight(uint64_t x, int n)
{
	const unsigned int mask = (8 * sizeof(x) - 1);  // assumes width is a power of 2.

	// assert ( (c<=mask) &&"rotate by type width or more");
	n &= mask;
	return (x >> n) | (x << ((-n)&mask));

}

#endif //ROTATION_H_

