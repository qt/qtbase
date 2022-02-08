#pragma once

#include "stdafx.h"
#include "HashFunction.h"

// State structure
class KeccakBase : public HashFunction
{
public:
	KeccakBase(unsigned int len);
	KeccakBase(const KeccakBase& other);
	virtual ~KeccakBase();
	KeccakBase& operator=(const KeccakBase& other);
	virtual std::vector<unsigned char> digest() = 0;
	virtual void addPadding() = 0;
	void reset();
	void keccakf();
	void addData(uint8_t input) override;
	void addData(const uint8_t *input, unsigned int off, unsigned int len) override;
	void processBuffer();
protected:
	uint64_t *A;
	unsigned int blockLen;
	uint8_t *buffer;
	unsigned int bufferLen;
	unsigned int length;
};

class Sha3 : public KeccakBase
{
public:
	Sha3(unsigned int len);
	Sha3(const Sha3& other);
	Sha3& operator=(const Sha3& other);
	std::vector<unsigned char> digest() override;
	void addPadding() override;
private:
};

class Keccak : public KeccakBase
{
public:
	Keccak(unsigned int len);
	Keccak(const Keccak& other);
	Keccak& operator=(const Keccak& other);
	std::vector<unsigned char> digest() override;
	void addPadding() override;
private:
};

class Shake : public KeccakBase
{
public:
	Shake(unsigned int len, unsigned int d_);
	Shake(const Shake& other);
	Shake& operator=(const Shake& other);
	std::vector<unsigned char> digest() override;
	void addPadding() override;
private:
	unsigned int d;
};
