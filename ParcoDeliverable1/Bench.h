#ifndef PARCO_BENCH
#define PARCO_BENCH

#include <iostream>
#include <fstream>
#include <chrono>
#include <omp.h>

/// <summary>
/// Runs the given function a certain amount
/// of times while registering the time
/// it takes to execute it, then performs
/// the average of the wall clock time
/// and prints it to the console
/// </summary>
/// <typeparam name="Func">Function type</typeparam>
/// <typeparam name="type">Hidden</typeparam>
/// <param name="function">The function to benchmark</param>
/// <param name="name">Benchmark name</param>
/// <param name="repeat">Amount of times that the function needs to be run</param>
template <typename Func,
	typename std::enable_if< std::is_same<decltype((std::declval<Func>())()), void>::value, bool>::type = true
>
void Benchmark(Func&& function, const char* name, uint32_t repeat, std::ofstream& out) {
	uint32_t rep_temp = repeat;
	auto start = std::chrono::high_resolution_clock::now();

	while (repeat--) {
		function();
	}

	auto end = std::chrono::high_resolution_clock::now();

	auto diff = (end - start).count() / 1e6;
	std::cout << name << " took " << diff / rep_temp << " ms" << std::endl;

	out << diff / rep_temp << std::endl;
}

/// <summary>
/// Runs the given function a certain amount
/// of times while registering the time
/// it takes to execute it, then performs
/// the average of the wall clock time
/// and prints it to the console.
/// Also returns the value from the function
/// (which means that it would be a good idea
/// to use a routine that always returns the same
/// thing)
/// </summary>
/// <typeparam name="Func">Function type</typeparam>
/// <typeparam name="type">Hidden</typeparam>
/// <param name="function">The function to benchmark</param>
/// <param name="name">Benchmark name</param>
/// <param name="repeat">Amount of times that the function needs to be run</param>
/// <returns>The return value of the function</returns>
template <typename Func,
	typename = typename std::enable_if< !std::is_same<decltype((std::declval<Func>())()), void>::value, bool>::type
>
decltype((std::declval<Func>())()) Benchmark(Func&& function, const char* name, uint32_t repeat, 
	std::ofstream& out) {
	uint32_t rep_temp = repeat;
	using RetType = decltype(function()); //Deduce return type
	RetType ret{};

	auto start = std::chrono::high_resolution_clock::now();

	while (repeat--) {
		ret = function();
	}

	auto end = std::chrono::high_resolution_clock::now();

	auto diff = (end - start).count() / 1e6;
	std::cout << name << " took " << diff / rep_temp << " ms" << std::endl;

	out << diff / rep_temp << std::endl;

	return ret;
}

////////////////////////////////////////////

/// <summary>
/// The principle is the same for the normal
/// benchmark, but performed with a different
/// number of threads each time
/// </summary>
/// <typeparam name="Func">Type of the function</typeparam>
/// <typeparam name="type">Hidden</typeparam>
/// <typeparam name="IncFunc">Type of the increment function</typeparam>
/// <param name="function">Function to benchmark</param>
/// <param name="name">Benchmark name</param>
/// <param name="repeat">Number of times that a test with N threads is performed</param>
/// <param name="inc_func">Function that returns the next number of threads</param>
/// <param name="init">Init number of threads</param>
/// <param name="limit">Max threads</param>
template <typename Func, typename IncFunc,
	typename std::enable_if< std::is_same<decltype((std::declval<Func>())()), void>::value, bool>::type = true
>
void BenchmarkThreads(Func&& function, const char* name, uint32_t repeat, IncFunc&& inc_func, uint32_t init, uint32_t limit,
	std::ofstream& out) {
	uint32_t num_cycles = 0;
	uint32_t curr_rep = repeat;
	uint32_t orig_rep = repeat;

	//Force the OMP runtime to use the exact number
	//of threads that we want
	auto dynamic = omp_get_dynamic();
	omp_set_dynamic(0);

	//Repeat until max threads is reached
	while (init <= limit) {
		//Set N threads
		omp_set_num_threads(init);
		uint32_t rep_temp = orig_rep;
		curr_rep = orig_rep;
		auto start = std::chrono::high_resolution_clock::now();

		while (curr_rep--) {
			function();
		}

		auto end = std::chrono::high_resolution_clock::now();

		auto diff = (end - start).count() / 1e6;
		std::cout << name << " with " << init << " threads took " << diff / rep_temp << " ms" << std::endl;

		out << init << " " << diff / rep_temp << std::endl;

		//Get next thread count
		init = inc_func(init, num_cycles++);
	}

	omp_set_dynamic(dynamic);
}

/// <summary>
/// The principle is the same for the normal
/// benchmark, but performed with a different
/// number of threads each time
/// </summary>
/// <typeparam name="Func">Type of the function</typeparam>
/// <typeparam name="type">Hidden</typeparam>
/// <typeparam name="IncFunc">Type of the increment function</typeparam>
/// <param name="function">Function to benchmark</param>
/// <param name="name">Benchmark name</param>
/// <param name="repeat">Number of times that a test with N threads is performed</param>
/// <param name="inc_func">Function that returns the next number of threads</param>
/// <param name="init">Init number of threads</param>
/// <param name="limit">Max threads</param>
/// <returns>Function's return value</returns>
template <typename Func, typename IncFunc,
	typename = typename std::enable_if< !std::is_same<decltype((std::declval<Func>())()), void>::value, bool>::type
>
decltype((std::declval<Func>())()) BenchmarkThreads(Func&& function, const char* name, uint32_t repeat
	, IncFunc&& inc_func, uint32_t init, uint32_t limit, std::ofstream& out) {
	using RetType = decltype(function());
	RetType ret{};

	uint32_t curr_rep = repeat;
	uint32_t orig_rep = repeat;
	uint32_t num_cycles = 0;

	auto dynamic = omp_get_dynamic();
	omp_set_dynamic(0);

	while (init <= limit) {
		omp_set_num_threads(init);
		uint32_t rep_temp = orig_rep;
		curr_rep = orig_rep;
		auto start = std::chrono::high_resolution_clock::now();

		while (curr_rep--) {
			ret = function();
		}

		auto end = std::chrono::high_resolution_clock::now();

		auto diff = (end - start).count() / 1e6;
		std::cout << name << " with " << init << " threads took " << diff / rep_temp << " ms" << std::endl;

		out << init << " " << diff / rep_temp << std::endl;

		init = inc_func(init, num_cycles++);
	}

	omp_set_dynamic(dynamic);

	return ret;
}

#endif // !PARCO_BENCH
