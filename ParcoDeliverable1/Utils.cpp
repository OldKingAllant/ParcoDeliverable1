#include "Utils.h"

#include <ctime>
#include <cstring>
#include <iostream>

#include <omp.h>

//Init random number generation
void InitRand() {
	srand(time(0));
}

//Allocate N*N contiguous memory
//(do not use array of pointers for each row, bad for cache and paging)
Matrix CreateRandomMatrix(uint32_t N, uint32_t N_THREADS) {
	const uint32_t TOT_SIZE = N * N;

	auto unit_matrix = new MatType[TOT_SIZE];

	int omp_dynamic = omp_get_dynamic();
	omp_set_dynamic(0);

	omp_set_num_threads(N_THREADS);

	//Init matrix linearly, one element at a time
	//You may be asking, why are we using openmp here?
	//Well my friend, this is useless when running
	//on a desktop system with a normal desktop
	//cpu, but necessary when running on a NUMA
	//system.
	//In principle, the new only reserves virtual memory,
	//and does not, in fact, allocate physical memory.
	//This allows us to perform the real init on 
	//multiple threads that, hopefully, will be 
	//distributed on different sockets in a smart
	//way. By the time that we ready to run benchmarks
	//for the matrix transposition, the matrix will be 
	//distributed on the physical memory of multiple 
	//sockets, allowing fast access from all threads
#pragma omp parallel for
	for (uint32_t index = 0; index < TOT_SIZE; index++) {
		unit_matrix[index] = rand() % int(VALUE_MAX);
	}

	omp_set_dynamic(omp_dynamic);

	return unit_matrix;
}

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

int IsSameMatrix(MatType const* M, MatType const* T, uint32_t N) {
	//Compare two matrices for equality, by using brute-force memcmp
	return std::memcmp(M, T, (N * N) * sizeof(MatType));
}

uint32_t TryParseUint32(const char* str, const char* err_msg) {
	uint32_t parsed_value = 0;

	try {
		parsed_value = uint32_t(std::stoi(std::string{ str }));
	}
	catch (...) {
		std::cerr << err_msg << std::endl;
		std::exit(0);
	}

	return parsed_value;
}

////////////////////////////////////////////////
//Nothing to see here

MatType* AllocateAndInit(uint32_t N) {
	MatType* T = new MatType[N * N];

#pragma omp parallel for
	for (uint32_t i = 0; i < N * N; i++) {
		T[i] = 0;
	}

	return T;
}

bool VerifyNestedAvail() {
	int omp_nested = omp_get_nested();
	
	omp_set_nested(1);

	bool avail = omp_get_nested() == 1;

	omp_set_nested(omp_nested);

	return avail;
}