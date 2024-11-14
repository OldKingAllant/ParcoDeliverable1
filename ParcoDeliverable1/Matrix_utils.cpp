#include "Matrix_utils.h"

#include <xmmintrin.h>
#include <immintrin.h>

#include <algorithm>

#include <omp.h>

uint32_t ComputeBlockSize(uint32_t N, uint32_t CACHE_LINE) {
	uint32_t curr_block_sz = 1;
	uint32_t block_sz = 1;

	//Set max block size as (CACHE_LINE_SIZE / 4) * 1.5
	uint32_t upper_bound = uint32_t(float(CACHE_LINE / sizeof(MatType)) * 1.5f);

	while (curr_block_sz <= N && curr_block_sz <= upper_bound) {
		if (N % curr_block_sz == 0) block_sz = curr_block_sz; //accept only 
															  //block sizes that divide N perfectly
		curr_block_sz++;
	}

	return block_sz;
}

void Transpose4x4(MatType const* src, MatType* dst,
	uint32_t row, uint32_t col, uint32_t N) {
	__m128 row1{}, row2{}, row3{}, row4{};
	__m128 t1{}, t2{}, t3{}, t4{};

	//Load the entire 4x4 block by using unaligned
	//packed float loads
	row1 = _mm_loadu_ps(&src[row * N + col]);
	row2 = _mm_loadu_ps(&src[(row + 1) * N + col]);
	row3 = _mm_loadu_ps(&src[(row + 2) * N + col]);
	row4 = _mm_loadu_ps(&src[(row + 3) * N + col]);

	//For each row:
	//Select two elements from position N 
	//(where N is the destination row number)
	//from alternating rows
	t1 = _mm_shuffle_ps(row1, row3, 0b00000000);
	//Select the other two elements from the 
	//the remaining rows
	t2 = _mm_shuffle_ps(row2, row4, 0b00000000);
	//Blend the two vectors together, by
	//using an alternating pattern for selection
	t1 = _mm_blend_ps(t1, t2, 0b1010);

	t2 = _mm_shuffle_ps(row1, row3, 0b00010001);
	t3 = _mm_shuffle_ps(row2, row4, 0b01000100);
	t2 = _mm_blend_ps(t2, t3, 0b1010);

	t3 = _mm_shuffle_ps(row1, row3, 0b00100010);
	t4 = _mm_shuffle_ps(row2, row4, 0b10001000);
	t3 = _mm_blend_ps(t3, t4, 0b1010);

	t4 = _mm_shuffle_ps(row1, row3, 0b00110011);
	row1 = _mm_shuffle_ps(row2, row4, 0b11001100);
	t4 = _mm_blend_ps(t4, row1, 0b1010);

	/*
	To why we are using alternating rows in the shuffle:
	https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#text=_mm_shuffle_&ig_expand=6047

	In short: Adjacent elements are taken from the same
	row, so we cannot get, for example, elements 0 and 0
	of the first two rows and put them as elements 0, 1
	in the transposed row

	Step by step approach:

	We need to transpose
	0 1 2 3
	4 5 6 7
	8 9 a b
	c d e f

	1) Start by loading all 4 rows into xmm registers by using _mm_loadu_ps
	2) First row shuffle:
		Select element 0 from row0 and row3, putting them in a temp register,
		which gives 0 - 8 -
		Then select element 0 from row1 and row2, store them in a second register
		giving - 4 - c
		(Note that the - are don't care)
	3) First row blend:
		The immediate value in the blend instruction is 4 bits,
		where bit N is used to decide from which variable
		the value of the N float is taken
		For reference:
		https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#text=_mm_blend_ps&ig_expand=6047,482
		By using the pattern 1010, we see that we are selecting
		T[0] from the first temp. value,
		T[1] from the second and so on
		The resulting operation is:
			0 - 8 -
			- 4 - c
		=   0 4 8 c
	4) Repeat same approach used for row 0 but instead of selecting
		element 0 from the rows in the shuffle, select
		element N
	5) Store each row one at a time
	*/

	//Store transposed rows
	_mm_storeu_ps(&dst[col * N + row], t1);
	_mm_storeu_ps(&dst[(col + 1) * N + row], t2);
	_mm_storeu_ps(&dst[(col + 2) * N + row], t3);
	_mm_storeu_ps(&dst[(col + 3) * N + row], t4);
}

void Transpose4x4_Aligned(MatType const* src, MatType* dst,
	uint32_t row, uint32_t col, uint32_t N) {
	__m128 row1{}, row2{}, row3{}, row4{};
	__m128 t1{}, t2{}, t3{}, t4{};

	row1 = _mm_load_ps(&src[row * N + col]);
	row2 = _mm_load_ps(&src[(row + 1) * N + col]);
	row3 = _mm_load_ps(&src[(row + 2) * N + col]);
	row4 = _mm_load_ps(&src[(row + 3) * N + col]);

	t1 = _mm_shuffle_ps(row1, row3, 0b00000000);
	t2 = _mm_shuffle_ps(row2, row4, 0b00000000);
	t1 = _mm_blend_ps(t1, t2, 0b1010);

	t2 = _mm_shuffle_ps(row1, row3, 0b00010001);
	t3 = _mm_shuffle_ps(row2, row4, 0b01000100);
	t2 = _mm_blend_ps(t2, t3, 0b1010);

	t3 = _mm_shuffle_ps(row1, row3, 0b00100010);
	t4 = _mm_shuffle_ps(row2, row4, 0b10001000);
	t3 = _mm_blend_ps(t3, t4, 0b1010);

	t4 = _mm_shuffle_ps(row1, row3, 0b00110011);
	row1 = _mm_shuffle_ps(row2, row4, 0b11001100);
	t4 = _mm_blend_ps(t4, row1, 0b1010);

	_mm_store_ps(&dst[col * N + row], t1);
	_mm_store_ps(&dst[(col + 1) * N + row], t2);
	_mm_store_ps(&dst[(col + 2) * N + row], t3);
	_mm_store_ps(&dst[(col + 3) * N + row], t4);
}

void BlockTranspose_NoSSE(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE) {
	for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
		for (uint32_t col_idx = 0; col_idx < N; col_idx += BLOCK_SIZE) {

			/*
			Testing prefetch:
			Locality 3  -> 63.13 ms
			Locality 2  -> 63.45 ms
			Locality 1  -> 63.16 ms
			No prefetch -> 62.1 61.96 62.59 64.83 62.25  64.72 64.12 64.85 61.85 62.55

			Conclusion: we cannot draw conclusions, either the prefetch is useless,
			or by manually re-running the program N times it is impossible to
			obtain a stable average for each case

			More testing necessary, by running the same matrix transpose hundreds of times
			in a strict loop. In any case, the performance gain is negligible
			*/

			//Compute row and column bounds (necessary if N % BLOCK_SIZE != 0)
			uint32_t row_bound = std::min(row_idx + BLOCK_SIZE, N);
			uint32_t col_bound = std::min(col_idx + BLOCK_SIZE, N);

			for (uint32_t row_block = row_idx; row_block < row_bound; row_block++) {
				for (uint32_t col_block = col_idx; col_block < col_bound; col_block++) {
					T[col_block * N + row_block] = M[row_block * N + col_block];
				}
			}

		}
	}
}

void BlockTranspose_NoSSE_OMP(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE) {
	//I noted that putting the parallel here and the for
	//inside the first loop drastically improves performance
#pragma omp parallel
	{
		for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
			//Use schedule(auto) so that we can change scheduling
			//by using env variable OMP_SCHEDULE
#pragma omp for collapse(2) schedule(auto)
			for (uint32_t col_idx = 0; col_idx < N; col_idx += BLOCK_SIZE) {
				//We can compute the bounds only in the loop conditions
				//Otherwise collapse fails
				for (uint32_t row_block = row_idx; row_block < std::min(row_idx + BLOCK_SIZE, N); row_block++) {
					for (uint32_t col_block = col_idx; col_block < std::min(col_idx + BLOCK_SIZE, N); col_block++) {
						T[col_block * N + row_block] = M[row_block * N + col_block];
					}
				}

			}
		}
	}
}

/*
Expect multiple versions of the same algorithm, since
we are trying to target C++11 (which is 13!! years old)
and we cannot use if constexpr for conditional template generation

if constexpr is available starting from C++17
*/

void matTransposeCacheObliviousImp(MatType const* M, MatType* T, uint32_t N,
	uint32_t N_rem, uint32_t col_offset, uint32_t row_offset) {
	if (N_rem <= 32) {
		//End condition, size is small enough

		if (N_rem % 4 == 0) { //Use sse
			//We trust the caller and expect the matrix to be
			//16-bytes aligned
			for (uint32_t row_idx = 0; row_idx < N_rem; row_idx += 4) {
				for (uint32_t col_idx = 0; col_idx < N_rem; col_idx += 4) {
					Transpose4x4_Aligned(M, T, row_offset + row_idx,
						col_offset + col_idx, N);
				}
			}
		}
		else {
			//Use normal transpose
			for (uint32_t row_idx = 0; row_idx < N_rem; row_idx++) {
				for (uint32_t col_idx = 0; col_idx < N_rem; col_idx++) {
					T[(col_idx + col_offset) * N + (row_offset + row_idx)] =
						M[(row_offset + row_idx) * N + (col_offset + col_idx)];
				}
			}
		}

	}
	else {
		//Divide and conquer in 4 submatrices
		uint32_t half_size = N_rem / 2;
		matTransposeCacheObliviousImp(M, T, N, half_size, col_offset, row_offset);
		matTransposeCacheObliviousImp(M, T, N, half_size, col_offset + half_size, row_offset);

		matTransposeCacheObliviousImp(M, T, N, half_size, col_offset, row_offset + half_size);
		matTransposeCacheObliviousImp(M, T, N, half_size, col_offset + half_size, row_offset + half_size);

		if (N_rem & 1) {
			//Size is not even, must transpose last row and column
			for (uint32_t row_idx = 0; row_idx < N_rem; row_idx++) {
				T[(col_offset + N_rem - 1) * N + (row_offset + row_idx)] =
					M[(row_offset + row_idx) * N + (col_offset + N_rem - 1)];

				T[(col_offset + row_idx) * N + (row_offset + N_rem - 1)] =
					M[(row_offset + N_rem - 1) * N + (col_offset + row_idx)];
			}
		}
	}
}

void matTransposeCacheObliviousImpOMP(MatType const* M, MatType* T, uint32_t N,
	uint32_t N_rem, uint32_t col_offset, uint32_t row_offset) {

	if (N_rem <= 64) {

		if (N_rem % 4 == 0) {
			for (uint32_t row_idx = 0; row_idx < N_rem; row_idx += 4) {
				for (uint32_t col_idx = 0; col_idx < N_rem; col_idx += 4) {
					Transpose4x4_Aligned(M, T, row_offset + row_idx,
						col_offset + col_idx, N);
				}
			}
		}
		else {
			for (uint32_t row_idx = 0; row_idx < N_rem; row_idx++) {
				for (uint32_t col_idx = 0; col_idx < N_rem; col_idx++) {
					T[(col_idx + col_offset) * N + (row_offset + row_idx)] =
						M[(row_offset + row_idx) * N + (col_offset + col_idx)];
				}
			}
		}

	}
	else {
		uint32_t half_size = N_rem / 2;

//Run 4 different tasks. Top level will spawn 
//4 different threads. Whether the other levels
//add new threads or not depends on external factors
		{
#pragma omp task 
			matTransposeCacheObliviousImp(M, T, N, half_size, col_offset, row_offset);
#pragma omp task 
			matTransposeCacheObliviousImp(M, T, N, half_size, col_offset + half_size, row_offset);
#pragma omp task 
			matTransposeCacheObliviousImp(M, T, N, half_size, col_offset, row_offset + half_size);
#pragma omp task 
			matTransposeCacheObliviousImp(M, T, N, half_size, col_offset + half_size, row_offset + half_size);
#pragma omp taskwait
		}

		if (N_rem & 1) {
			for (uint32_t row_idx = 0; row_idx < N_rem; row_idx++) {
				T[(col_offset + N_rem - 1) * N + (row_offset + row_idx)] =
					M[(row_offset + row_idx) * N + (col_offset + N_rem - 1)];

				T[(col_offset + row_idx) * N + (row_offset + N_rem - 1)] =
					M[(row_offset + N_rem - 1) * N + (col_offset + row_idx)];
			}
		}
	}
}

//These functions below follow the same principle as the blocked non-sse transform

template <>
void BlockTranspose_SSE<true>(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE) {
	for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
		for (uint32_t col_idx = 0; col_idx < N; col_idx += BLOCK_SIZE) {

			for (uint32_t row_block = row_idx; row_block < row_idx + BLOCK_SIZE; row_block += 4) {
				for (uint32_t col_block = col_idx; col_block < col_idx + BLOCK_SIZE; col_block += 4) {
					Transpose4x4_Aligned(M, T, row_block, col_block, N);
				}
			}

		}
	}
}

template <>
void BlockTranspose_SSE<false>(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE) {
	for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
		for (uint32_t col_idx = 0; col_idx < N; col_idx += BLOCK_SIZE) {

			for (uint32_t row_block = row_idx; row_block < row_idx + BLOCK_SIZE; row_block += 4) {
				for (uint32_t col_block = col_idx; col_block < col_idx + BLOCK_SIZE; col_block += 4) {
					Transpose4x4(M, T, row_block, col_block, N);
				}
			}

		}
	}
}

template <>
void BlockTranspose_SSE_OMP<true>(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE) {
#pragma omp parallel
	for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
#pragma omp for collapse(2) schedule(auto)
		for (uint32_t col_idx = 0; col_idx < N; col_idx += BLOCK_SIZE) {

			for (uint32_t row_block = row_idx; row_block < row_idx + BLOCK_SIZE; row_block += 4) {
				for (uint32_t col_block = col_idx; col_block < col_idx + BLOCK_SIZE; col_block += 4) {
					Transpose4x4_Aligned(M, T, row_block, col_block, N);
				}
			}

		}
	}
}

template <>
void BlockTranspose_SSE_OMP<false>(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE) {
#pragma omp parallel
	for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
#pragma omp for collapse(2) schedule(auto)
		for (uint32_t col_idx = 0; col_idx < N; col_idx += BLOCK_SIZE) {

			for (uint32_t row_block = row_idx; row_block < row_idx + BLOCK_SIZE; row_block += 4) {
				for (uint32_t col_block = col_idx; col_block < col_idx + BLOCK_SIZE; col_block += 4) {
					Transpose4x4(M, T, row_block, col_block, N);
				}
			}

		}
	}
}