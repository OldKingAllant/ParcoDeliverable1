#include "Matrix_manip.h"
#include "Matrix_utils.h"

#include <algorithm>

#include <omp.h>

///////////////////////////////////////////////////////
//SYMMETRY CHECKS

//Linear, "unoptimized" function
//for checking if the matrix is symmetrical
bool checkSym(MatType* M, uint32_t N) {
	for (uint32_t row_idx = 0; row_idx < N; row_idx++) {
		//Do a little optimization that will cut read/compare
		//in half, by skipping the left-hand side of the matrix
		//plus the main diagonal
		//(when row_idx=col_idx we are on the main diagonal,
		//values will always be equal)
		for (uint32_t col_idx = row_idx + 1; col_idx < N; col_idx++) {
			if (M[row_idx * N + col_idx] != M[col_idx * N + row_idx]) {
				return false;
			}
		}
	}

	return true;
}

/*
* Improved symmetry check, using the same approach as
* the improved transpose without using sse instructions.
*
* The only thing that comes to my mind with regard to
* using vector instructions in this algorithm, is
* transposing 4x4 blocks by using the same approach
* as Transpose4x4 and then using memcmp between
* the transposed block and its counterpart
* on the other side of the main diagonal.
* Ultimately, this would take more/same
* time than/as the current implementation
*
* And even if an implementation that
* uses sse is faster than this,
* the difference would be 0 most
* of the time, since this function
* quits as soon as it finds one
* element that does not match
*/
bool checkSymImp(MatType* M, uint32_t N) {
	uint32_t BLOCK_SIZE = ComputeBlockSize(N, CACHE_LINE_SIZE);

	for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
		for (uint32_t col_idx = row_idx + 1; col_idx < N; col_idx += BLOCK_SIZE) {

			uint32_t row_bound = std::min(row_idx + BLOCK_SIZE, N);
			uint32_t col_bound = std::min(col_idx + BLOCK_SIZE, N);

			for (uint32_t row_block = row_idx; row_block < row_bound; row_block++) {
				for (uint32_t col_block = col_idx; col_block < col_bound; col_block++) {
					if (M[row_block * N + col_block] != M[col_block * N + row_block]) return false;
				}
			}

		}
	}

	return true;
}

bool checkSymOMP(MatType* M, uint32_t N) {
	uint32_t BLOCK_SIZE = ComputeBlockSize(N, CACHE_LINE_SIZE);

	uint32_t num_errors = 0;

#pragma omp parallel reduction(+:num_errors)
	{
		for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
#pragma omp for collapse(2) schedule(auto)
			for (uint32_t col_idx = row_idx + 1; col_idx < N; col_idx += BLOCK_SIZE) {

				for (uint32_t row_block = row_idx; row_block < std::min(row_idx + BLOCK_SIZE, N); row_block++) {
					for (uint32_t col_block = col_idx; col_block < std::min(col_idx + BLOCK_SIZE, N); col_block++) {
						if (M[row_block * N + col_block] != M[col_block * N + row_block]) ++num_errors;
					}
				}

			}
		}
	}

	return num_errors == 0;
}

//////////////////////////////////////////
//TRANSPOSE

//Unoptimized matrix transpose
void matTranspose(MatType const* M, MatType* T, uint32_t N) {
	for (uint32_t row_idx = 0; row_idx < N; row_idx++) {
		for (uint32_t col_idx = 0; col_idx < N; col_idx++) {
			T[col_idx * N + row_idx] = M[row_idx * N + col_idx];
		}
	}
	//Alternative approach: do col major, 
	//keep write line longer in the cache
	/*for (uint32_t col_idx = 0; col_idx < N; col_idx++) {
		for (uint32_t row_idx = 0; row_idx < N; row_idx++) {
			T[col_idx * N + row_idx] = M[row_idx * N + col_idx];
		}
	}*/
}

/// Do transposition by dividing matrix in blocks of non-fixed size.
/// Fixed size blocks would lead to loop unrolling by the compiler,
/// which is good, but would not work for matrices with size
/// which is not divisible by the block size
/// 
/// What we want to do is to compute the block size on the fly,
/// with max bound equal to the cache line size divided by 
/// the size of the matrix type (float or double)
/// 
/// 
/// Matrices with N being a prime number (unlikely)
/// will result in massive slowdown since the block
/// size will be equal to 1. Alternatively, the implementation
/// could use a big block size until the boundary of the matrix is
/// reached, and the remaining elements are copied one by one
void matTransposeImp(MatType const* M, MatType* T, uint32_t N) {
	uint32_t BLOCK_SIZE = ComputeBlockSize(N, CACHE_LINE_SIZE);

	if (BLOCK_SIZE % 4 == 0) {
		//Block size is perfectly divisible by 4,
		//use block size, divide block size by 4
		//and use sse enhanced transposition
		if ((unsigned long long)(M) % 16 == 0 && (unsigned long long)(T) % 16 == 0) {
			//Matrix is aligned to 16 byte boundary
			//Each 4x4 block will also be aligned
			//Use aligned version of sse matrix transpose
			BlockTranspose_SSE<true>(M, T, N, BLOCK_SIZE);
		}
		else {
			BlockTranspose_SSE<false>(M, T, N, BLOCK_SIZE);
		}
	}
	else {
		BlockTranspose_NoSSE(M, T, N, BLOCK_SIZE);
	}
}

void matTransposeOMP(MatType const* M, MatType* T, uint32_t N) {
	uint32_t BLOCK_SIZE = ComputeBlockSize(N, CACHE_LINE_SIZE);

	if (BLOCK_SIZE % 4 == 0) {
		if ((unsigned long long)(M) % 16 == 0 && (unsigned long long)(T) % 16 == 0) {
			BlockTranspose_SSE_OMP<true>(M, T, N, BLOCK_SIZE);
		}
		else {
			BlockTranspose_SSE_OMP<false>(M, T, N, BLOCK_SIZE);
		}
	}
	else {
		BlockTranspose_NoSSE_OMP(M, T, N, BLOCK_SIZE);
	}
}

void matTransposeCacheOblivious(MatType const* M, MatType* T, uint32_t N) {
	matTransposeCacheObliviousImp(M, T, N, N, 0, 0);
}

void matTransposeCacheObliviousOMP(MatType const* M, MatType* T, uint32_t N) {
#pragma omp parallel
#pragma omp single nowait
	{
		matTransposeCacheObliviousImpOMP(M, T, N, N, 0, 0);
	}
}

void matTransposeFinal(MatType const* M, MatType* T, uint32_t N) {
	using Dispatch = void(*)(MatType const* M, MatType* T, uint32_t N);

	static const Dispatch jmp_table[] =
	{
		matTransposeImp,
		matTransposeCacheOblivious,
		matTransposeOMP,
		matTransposeCacheObliviousOMP
	};

	uint32_t index = uint32_t(N >= 512) << 1;
	index |= uint32_t((N & (N - 1)) == 0);

	jmp_table[index](M, T, N);
}