#ifndef PARCO_MATRIX_UTILS
#define PARCO_MATRIX_UTILS

#include "Defs.h"

/// <summary>
/// Computes block size for optimized
/// transpose functions depending on the
/// given cache line size
/// </summary>
/// <param name="N">Size of one dimension</param>
/// <param name="CACHE_LINE">Cache line size</param>
/// <returns>Block size</returns>
uint32_t ComputeBlockSize(uint32_t N, uint32_t CACHE_LINE);

/// <summary>
/// Transposes 4x4 block using SSE and
/// unaligned loads
/// </summary>
/// <param name="src">Source ptr</param>
/// <param name="dst">Dest ptr</param>
/// <param name="row">Curr row</param>
/// <param name="col">Curr col</param>
/// <param name="N">Size of rows and cols</param>
void Transpose4x4(MatType const* src, MatType* dst,
	uint32_t row, uint32_t col, uint32_t N);

/// <summary>
/// Transposes 4x4 block using SSE and
/// aligned loads
/// </summary>
/// <param name="src">Source ptr</param>
/// <param name="dst">Dest ptr</param>
/// <param name="row">Curr row</param>
/// <param name="col">Curr col</param>
/// <param name="N">Size of rows and cols</param>
void Transpose4x4_Aligned(MatType const* src, MatType* dst,
	uint32_t row, uint32_t col, uint32_t N);

/// <summary>
/// Transpose matrix by blocks without
/// using SSE but still using tiling
/// </summary>
/// <param name="M">Source matrix</param>
/// <param name="T">Dest matrix</param>
/// <param name="N">N</param>
/// <param name="BLOCK_SIZE">The block size</param>
void BlockTranspose_NoSSE(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE);

/// <summary>
/// Transpose matrix by blocks without
/// using SSE but still using tiling,
/// while using OMP
/// </summary>
/// <param name="M">Source matrix</param>
/// <param name="T">Dest matrix</param>
/// <param name="N">N</param>
/// <param name="BLOCK_SIZE">The block size</param>
void BlockTranspose_NoSSE_OMP(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE);

/// <summary>
/// Transposes the matrix in a cache oblivious fashion.
/// Depending on the matrix size, uses SSE or 
/// normal blocked transpose
/// </summary>
/// <param name="M">Source matrix</param>
/// <param name="T">Dest matrix</param>
/// <param name="N">N</param>
/// <param name="N_rem">Remaining matrix size in recursion</param>
/// <param name="col_offset">Global column offset</param>
/// <param name="row_offset">Global row offset</param>
void matTransposeCacheObliviousImp(MatType const* M, MatType* T, uint32_t N,
	uint32_t N_rem, uint32_t col_offset, uint32_t row_offset);

/// <summary>
/// Same as above using openmp
/// </summary>
/// <param name="M">Source matrix</param>
/// <param name="T">Dest matrix</param>
/// <param name="N">N</param>
/// <param name="N_rem">Remaining matrix size in recursion</param>
/// <param name="col_offset">Global column offset</param>
/// <param name="row_offset">Global row offset</param>
void matTransposeCacheObliviousImpOMP(MatType const* M, MatType* T, uint32_t N,
	uint32_t N_rem, uint32_t col_offset, uint32_t row_offset);

/// <summary>
/// Transpose matrix by blocks while
/// using SSE and tiling.
/// Based on the Aligned template parameter,
/// it statically switches between aligned and
/// unaligned loads
/// </summary>
/// <typeparam name="Aligned">If the matrices are aligned to 16 bytes boundaries</typeparam>
/// <param name="M">Source matrix</param>
/// <param name="T">Dest matrix</param>
/// <param name="N">N</param>
/// <param name="BLOCK_SIZE">The block size</param>
template <bool Aligned>
void BlockTranspose_SSE(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE);

/// <summary>
/// See above, but with OMP
/// </summary>
/// <typeparam name="Aligned">If the matrices are aligned to 16 bytes boundaries</typeparam>
/// <param name="M">Source matrix</param>
/// <param name="T">Dest matrix</param>
/// <param name="N">N</param>
/// <param name="BLOCK_SIZE">The block size</param>
template <bool Aligned>
void BlockTranspose_SSE_OMP(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE);

#endif // !PARCO_MATRIX_UTILS
