#ifndef _LOOKA_STR2ID_HPP
#define _LOOKA_STR2ID_HPP
#include "looka_types.hpp"

const uint32_t kFingerPrintSeed = 19861202;
uint64_t MurmurHash64A(const void* key, int len, uint32_t seed);
uint64_t Fingerprint(const std::string& str);
GlobalDocID ComputeGlobalDocID(const std::string& str);
TokenID ComputeTokenID(const std::string& token);

#endif //_LOOKA_STR2ID_HPP
