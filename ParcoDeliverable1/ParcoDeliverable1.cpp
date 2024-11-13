// ParcoDeliverable1.cpp : Defines the entry point for the application.
//

#include "ParcoDeliverable1.h"

#include <memory>
#include <chrono>
#include <type_traits>

//Use old ctime header for time(), rand() and srand()
//Using the C++ distributions for random numbers is too much
//of a hassle
#include <ctime>

//memcmp()
#include <cstring>

//Project includes
#include "Defs.h"
#include "Bench.h"
#include "Utils.h"
#include "Matrix_utils.h"
#include "Matrix_manip.h"

//Default value of rows and cols
static constexpr uint32_t CONST_N = 4096;

////////////////////////////////////////////////////////////
// ///////////////////////MATRIX MANIP/CHECK FUNCTIONS//////

#ifdef CONSTEXPR_N
	//Here lies the attempt at using a constexpr matrix size to 
	//improve loop unrolling by the compiler.
	//Unfortunately, it seems that the amount of instructions
	//generated could not fit in the icache very well, leading
	//to worse than expected performance
	//R.I.P.
#endif // CONSTEXPR_N


////////////////////////////////////////////////////////////


#define USE_CONSTANT

int main(int argc, char* argv[])
{
	uint32_t N = CONST_N;

#ifndef USE_CONSTANT
	if (argc < 2) {
		std::cerr << "Missing arguments\n";
		std::cerr << "Usage: " << argv[0] << " N" << std::endl;
		std::cin.get();
		std::exit(0);
	}

	const char* m_size_str = argv[1];

	try {
		N = uint32_t(std::stoi(std::string{ m_size_str }));
	}
	catch (std::invalid_argument const&) {
		std::cerr << "Invalid N" << std::endl;
		std::exit(0);
	}
	
#endif // !USE_CONSTANT

	InitRand();

	auto the_matrix = CreateRandomMatrix(N);

	MatType* T = new MatType[N * N]{};
	MatType* T2 = new MatType[N * N]{};
	MatType* T3 = new MatType[N * N]{};
	MatType* T4 = new MatType[N * N]{};
	MatType* T5 = new MatType[N * N]{};

	std::memset(T, 0, N * N);
	std::memset(T2, 0, N * N);
	std::memset(T3, 0, N * N);

	const auto NUM_BYTES = uint64_t(N) * N * 4;

	std::cout << "Testing for " << N << " rows and columns\n";
	std::cout << "Which means " << (N * N) << " elements\n";
	std::cout << "For a total " << NUM_BYTES / 1.024e9 << " GB" << std::endl;

	/////////////////////////////////

	bool is_symm = Benchmark([=]() { return checkSym(the_matrix, N); }, 
		"Base symm check", 1);

	if (is_symm) {
		std::cout << "Matrix is symmetric" << std::endl;
	}
	else {
		std::cout << "Matrix is not symmetric" << std::endl;
	}

	/////////////////////////////////
	bool is_symm_imp = Benchmark([=]() { return checkSymImp(the_matrix, N); }, "checkSymImp", 100);

	if (is_symm != is_symm_imp) {
		std::cout << "checkSymImp not working" << std::endl;
	}

	/////////////////////////////////
	bool is_symm_omp = Benchmark([=]() { return checkSymOMP(the_matrix, N); }, "checkSymOMP", 100);

	if (is_symm != is_symm_omp) {
		std::cout << "checkSymOMP not working" << std::endl;
	}
	
	//////////////////////////////////

	Benchmark([=]() -> void {matTranspose(the_matrix, T, N); }, "Base transpose", 1);
	
	/////////////////////////////////
	Benchmark([=]() { matTransposeImp(the_matrix, T2, N); }, "Imp transpose", 100);
	if (IsSameMatrix(T, T2, N))
		std::cout << "Improved transpose not working" << std::endl;

	////////////////////////////////
	Benchmark([=]() { matTransposeOMP(the_matrix, T3, N); }, "OMP transpose", 100);
	if (IsSameMatrix(T, T3, N))
		std::cout << "OMP transpose not working" << std::endl;

	////////////////////////////////
	Benchmark([=]() { matTransposeCacheOblivious(the_matrix, T4, N); }, "Oblivious transpose", 100);
	if (IsSameMatrix(T, T4, N))
		std::cout << "Oblivious transpose not working" << std::endl;

	//PrintMatrix(T, N);
	//std::cout << std::endl;
	//PrintMatrix(T4, N);

	////////////////////////////////
	Benchmark([=]() { matTransposeCacheObliviousOMP(the_matrix, T5, N); }, "Oblivious OMP transpose", 100);
	if (IsSameMatrix(T, T5, N))
		std::cout << "Oblivious transpose not working" << std::endl;

	////////////////////////////////

	delete[] the_matrix;
	delete[] T;
	delete[] T2;
	delete[] T3;
	delete[] T4;
	delete[] T5;

	std::cin.get();
	return 0;
}
