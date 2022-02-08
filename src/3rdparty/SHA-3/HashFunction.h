#pragma once

class HashFunction
{
public:
	HashFunction(void);
	virtual ~HashFunction(void);

	virtual void addData(uint8_t input) = 0;
	virtual void addData(const uint8_t *input, unsigned int off, unsigned int len) = 0;
};

