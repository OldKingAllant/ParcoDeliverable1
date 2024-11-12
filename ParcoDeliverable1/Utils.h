#ifndef PARCO_UTILS
#define PARCO_UTILS

#include "Defs.h"

void InitRand();

Matrix CreateRandomMatrix(uint32_t N);

void PrintMatrix(MatType* mat, uint32_t N);

int IsSameMatrix(MatType const* M, MatType const* T, uint32_t N);

#endif // !PARCO_UTILS
