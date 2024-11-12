#ifndef PARCO_MANIP
#define PARCO_MANIP

#include "Defs.h"

bool checkSym(MatType* M, uint32_t N);

bool checkSymImp(MatType* M, uint32_t N);

bool checkSymOMP(MatType* M, uint32_t N);

void matTranspose(MatType const* M, MatType* T, uint32_t N);

void matTransposeImp(MatType const* M, MatType* T, uint32_t N);

void matTransposeOMP(MatType const* M, MatType* T, uint32_t N);

void matTransposeCacheOblivious(MatType const* M, MatType* T, uint32_t N);

void matTransposeCacheObliviousOMP(MatType const* M, MatType* T, uint32_t N);

#endif // !PARCO_MANIP
