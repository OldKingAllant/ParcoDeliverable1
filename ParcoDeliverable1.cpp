﻿// ParcoDeliverable1.cpp : Defines the entry point for the application.
//

#include "ParcoDeliverable1.h"

#include <memory>
#include <chrono>
#include <omp.h>

//Use old ctime header for time(), rand() and srand()
//Using the C++ distributions for random numbers is too much
//of a hassle
#include <ctime>

//memcmp()
#include <cstring>

#include <xmmintrin.h>
#include <immintrin.h>

//Type alias, use this to change
//type used in the program
using MatType = float;

using Matrix = std::unique_ptr<MatType[]>;

//Max value generated by the rand()
static constexpr MatType VALUE_MAX = 9.0f;

//Default value of rows and cols
static constexpr uint32_t CONST_N = 8000;

static constexpr uint32_t CACHE_LINE_SIZE = 64;
static constexpr uint32_t RECOMMENDED_BLOCK_SZ = CACHE_LINE_SIZE / sizeof(MatType);


//Init random number generation
void InitRand() {
	srand(time(0));
}

//Allocate N*N contiguous memory
//(do not use array of pointers for each row, bad for cache and paging)
Matrix CreateRandomMatrix(uint32_t N) {
	const uint32_t TOT_SIZE = N * N;

	auto unit_matrix = std::make_unique<MatType[]>(TOT_SIZE);

	//Init matrix linearly, one element at a time
	for (uint32_t index = 0; index < TOT_SIZE; index++) {
		unit_matrix[index] = rand() % int(VALUE_MAX);
	}

	return unit_matrix;
}

//Print matrix to console, row by row
void PrintMatrix(MatType* mat, uint32_t N) {
	if (mat == nullptr)
		return;

	for (uint32_t row_index = 0; row_index < N; row_index++) {
		for (uint32_t col_index = 0; col_index < N; col_index++) {
			std::cout << mat[row_index * N + col_index] << " ";
		}

		std::cout << "\n";
	}
}

//Compare two matrices for equality, by using brute-force memcmp
int IsSameMatrix(MatType const* M, MatType const* T, uint32_t N) {
	return std::memcmp(M, T, (N * N) * sizeof(MatType));
}

uint32_t ComputeBlockSize(uint32_t N, uint32_t CACHE_LINE) {
	uint32_t curr_block_sz = 1;
	uint32_t block_sz = 1;

	uint32_t upper_bound = uint32_t(float(CACHE_LINE / sizeof(MatType)) * 1.5f);

	while (curr_block_sz <= N && curr_block_sz <= upper_bound) {
		if (N % curr_block_sz == 0) block_sz = curr_block_sz;
		curr_block_sz++;
	}

	return block_sz;
}

/// <summary>
/// Transpose 4x4 block by using sse 4.1 
/// instructions (20 operations)
/// </summary>
/// <param name="src">Src pointer</param>
/// <param name="dst">Dst pointer</param>
/// <param name="row">Row offset inside src</param>
/// <param name="col">Col offset inside dst</param>
/// <param name="N">Matrix size</param>
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

//Same principle as the previous tranpose, but use
//aligned load/stores.
//WARNING: The function does not check if the alignment
//is correct. The program will crash with SIGSEGV
//if it is not
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
#pragma omp parallel num_threads(2)
	{
		for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
#pragma omp for collapse(2)
			for (uint32_t col_idx = 0; col_idx < N; col_idx += BLOCK_SIZE) {
				for (uint32_t row_block = row_idx; row_block < std::min(row_idx + BLOCK_SIZE, N); row_block++) {
					for (uint32_t col_block = col_idx; col_block < std::min(col_idx + BLOCK_SIZE, N); col_block++) {
						T[col_block * N + row_block] = M[row_block * N + col_block];
					}
				}

			}
		}
	}
}

template <bool Aligned>
void BlockTranspose_SSE(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE) {
	for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
		for (uint32_t col_idx = 0; col_idx < N; col_idx += BLOCK_SIZE) {

			for (uint32_t row_block = row_idx; row_block < row_idx + BLOCK_SIZE; row_block += 4) {
				for (uint32_t col_block = col_idx; col_block < col_idx + BLOCK_SIZE; col_block += 4) {
					if constexpr (Aligned) {
						Transpose4x4_Aligned(M, T, row_block, col_block, N);
					}
					else {
						Transpose4x4(M, T, row_block, col_block, N);
					}
				}
			}

		}
	}
}

template <bool Aligned>
void BlockTranspose_SSE_OMP(MatType const* M, MatType* T, uint32_t N, uint32_t BLOCK_SIZE) {
#pragma omp parallel num_threads(2)
	for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
#pragma omp for collapse(2)
		for (uint32_t col_idx = 0; col_idx < N; col_idx += BLOCK_SIZE) {

			//__builtin_prefetch(&M[row_idx * N + col_idx], 0, 0);
			//__builtin_prefetch(&T[col_idx * N + row_idx], 1, 0);

			for (uint32_t row_block = row_idx; row_block < row_idx + BLOCK_SIZE; row_block += 4) {
				for (uint32_t col_block = col_idx; col_block < col_idx + BLOCK_SIZE; col_block += 4) {
					if constexpr (Aligned) {
						Transpose4x4_Aligned(M, T, row_block, col_block, N);
					}
					else {
						Transpose4x4(M, T, row_block, col_block, N);
					}
				}
			}

		}
	}
}

///////////////////END OF UTILITY FUNCTIONS/CONSTANTS///////////////

////////////////////////////////////////////////////////////
// ///////////////////////MATRIX MANIP/CHECK FUNCTIONS//////

//Linear, "unoptimized" function
//for checking if the matrix is symmetrical
//Since this is not optimized, we will use
//for loops
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
* And even if an implementatio that
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

#pragma omp parallel num_threads(2) reduction(+:num_errors)
	{
		for (uint32_t row_idx = 0; row_idx < N; row_idx += BLOCK_SIZE) {
#pragma omp for collapse(2)
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

//Unoptimized matrix transpose
void matTranspose(MatType const* M, MatType* T, uint32_t N) {
	auto start = std::chrono::high_resolution_clock::now();

	for (uint32_t row_idx = 0; row_idx < N; row_idx++) {
		for (uint32_t col_idx = 0; col_idx < N; col_idx++) {
			T[col_idx * N + row_idx] = M[row_idx * N + col_idx];
		}
	}

	auto end = std::chrono::high_resolution_clock::now();

	auto diff = (end - start).count() / 1e6;

	std::cout << "Base transpose took " << diff << " ms" << std::endl;
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
/// could a big block size until the boundary of the matrix is
/// reached, and the remaining elements are copied one by one
void matTransposeImp(MatType const* M, MatType* T, uint32_t N) {
	uint32_t BLOCK_SIZE = ComputeBlockSize(N, CACHE_LINE_SIZE);

	auto start = std::chrono::high_resolution_clock::now();

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

	auto end = std::chrono::high_resolution_clock::now();

	auto diff = (end - start).count() / 1e6;

	std::cout << "Imp transpose took " << diff << " ms" << std::endl;
}

void matTransposeOMP(MatType const* M, MatType* T, uint32_t N) {
	uint32_t BLOCK_SIZE = ComputeBlockSize(N, CACHE_LINE_SIZE);

	auto start = std::chrono::high_resolution_clock::now();

	if (BLOCK_SIZE % 4 == 0) {
		if ((unsigned long long)(M) % 16 == 0 && (unsigned long long)(T) % 16 == 0) {
			BlockTranspose_SSE_OMP<true>(M, T, N, BLOCK_SIZE);
			//BlockTranspose_NoSSE_OMP(M, T, N, BLOCK_SIZE);
		}
		else {
			BlockTranspose_SSE_OMP<false>(M, T, N, BLOCK_SIZE);
			//BlockTranspose_NoSSE_OMP(M, T, N, BLOCK_SIZE);
		}
	}
	else {
		BlockTranspose_NoSSE_OMP(M, T, N, BLOCK_SIZE);
	}

	auto end = std::chrono::high_resolution_clock::now();

	auto diff = (end - start).count() / 1e6;

	std::cout << "OMP transpose took " << diff << " ms" << std::endl;
}

////////////////////////////////////////////////////////////


#define USE_CONSTANT

int main(int argc, char* argv[])
{
	uint32_t N = CONST_N;

#ifndef USE_CONSTANT
	if (argc < 2) {
		std::cerr << "Missing arguments\n";
		std::cerr << "Usage <program name> N" << std::endl;
		std::cin.get();
	}
#endif // !USE_CONSTANT

	InitRand();

	auto the_matrix = CreateRandomMatrix(N);

	//PrintMatrix(the_matrix.get(), N);
	//std::cout << std::endl;

	/////////////////////////////////
	auto start = std::chrono::high_resolution_clock::now();

	bool is_symm = checkSym(the_matrix.get(), N);

	auto end = std::chrono::high_resolution_clock::now();

	auto diff = (end - start).count() / 1e6;

	std::cout << "Base symm check took " << diff << " ms" << std::endl;
	/////////////////////////////////

	if (is_symm) {
		std::cout << "Matrix is symmetric" << std::endl;
	}
	else {
		std::cout << "Matrix is not symmetric" << std::endl;
	}

	/////////////////////////////////
	start = std::chrono::high_resolution_clock::now();

	bool is_symm_omp = checkSymOMP(the_matrix.get(), N);

	end = std::chrono::high_resolution_clock::now();

	diff = (end - start).count() / 1e6;

	std::cout << "OMP symm check took " << diff << " ms" << std::endl;

	if (is_symm != is_symm_omp) {
		std::cout << "checkSymOMP not working" << std::endl;
	}
	//////////////////////////////////

	auto T = std::make_unique<MatType[]>(N * N);

	matTranspose(the_matrix.get(), T.get(), N);

	//PrintMatrix(T.get(), N);
	//std::cout << std::endl;

	/////////////////////////////////

	auto T2 = std::make_unique<MatType[]>(N * N);
	matTransposeImp(the_matrix.get(), T2.get(), N);
	if (IsSameMatrix(T.get(), T2.get(), N))
		std::cout << "Improved transpose not working" << std::endl;

	//PrintMatrix(T2.get(), N);
	//std::cout << std::endl;

	////////////////////////////////

	auto T3 = std::make_unique<MatType[]>(N * N);
	matTransposeOMP(the_matrix.get(), T3.get(), N);
	if (IsSameMatrix(T.get(), T3.get(), N))
		std::cout << "OMP transpose not working" << std::endl;

	std::cin.get();
	return 0;
}
