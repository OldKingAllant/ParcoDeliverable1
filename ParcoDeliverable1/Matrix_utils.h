#ifndef PARCO_MATRIX_UTILS
#define PARCO_MATRIX_UTILS

#include "Defs.h"

uint32_t ComputeBlockSize(uint32_t N, uint32_t CACHE_LINE);

void Transpose4x4(MatType const* src, MatType* dst,
	uint32_t row, uint32_t col, uint32_t N);

void Transpose4x4_Aligned(MatType const* src, MatType* dst,
	uint32_t row, uint32_t col, uint32_t N);

void BlockTranspose_NoSSE(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE);

void BlockTranspose_NoSSE_OMP(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE);

void matTransposeCacheObliviousImp(MatType const* M, MatType* T, uint32_t N,
	uint32_t N_rem, uint32_t col_offset, uint32_t row_offset);

void matTransposeCacheObliviousImpOMP(MatType const* M, MatType* T, uint32_t N,
	uint32_t N_rem, uint32_t col_offset, uint32_t row_offset);

template <bool Aligned>
void BlockTranspose_SSE(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE);

template <bool Aligned>
void BlockTranspose_SSE_OMP(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE);

#endif // !PARCO_MATRIX_UTILS
