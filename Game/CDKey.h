#pragma once

class CDKeyChecker
{
public:
	CDKeyChecker();
	bool check();

private:
	int permutations_[13];
	int permutationsInv_[13];
	bool readRegistry();
	bool writeRegistry(const char* key);
	const char* encode(const char* key);
	const char* decode(const char* key);
};
