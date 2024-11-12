#include "Utils.h"

#include <ctime>
#include <cstring>
#include <iostream>

//Init random number generation
void InitRand() {
	srand(time(0));
}

//Allocate N*N contiguous memory
//(do not use array of pointers for each row, bad for cache and paging)
Matrix CreateRandomMatrix(uint32_t N) {
	const uint32_t TOT_SIZE = N * N;

	auto unit_matrix = new MatType[TOT_SIZE]{};

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