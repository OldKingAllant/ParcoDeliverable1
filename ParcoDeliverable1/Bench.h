#ifndef PARCO_BENCH
#define PARCO_BENCH

#include <iostream>
#include <chrono>
#include <omp.h>

template <typename Func,
	typename std::enable_if< std::is_same<decltype((std::declval<Func>())()), void>::value, bool>::type = true
>
void Benchmark(Func&& function, const char* name, uint32_t repeat) {
	uint32_t rep_temp = repeat;
	auto start = std::chrono::high_resolution_clock::now();

	while (repeat--) {
		function();
	}

	auto end = std::chrono::high_resolution_clock::now();

	auto diff = (end - start).count() / 1e6;
	std::cout << name << " took " << diff / rep_temp << " ms" << std::endl;
}

template <typename Func,
	typename = typename std::enable_if< !std::is_same<decltype((std::declval<Func>())()), void>::value, bool>::type
>
decltype((std::declval<Func>())()) Benchmark(Func&& function, const char* name, uint32_t repeat) {
	uint32_t rep_temp = repeat;
	using RetType = decltype(function());
	RetType ret{};

	auto start = std::chrono::high_resolution_clock::now();

	while (repeat--) {
		ret = function();
	}

	auto end = std::chrono::high_resolution_clock::now();

	auto diff = (end - start).count() / 1e6;
	std::cout << name << " took " << diff / rep_temp << " ms" << std::endl;

	return ret;
}

////////////////////////////////////////////

template <typename Func, typename IncFunc,
	typename std::enable_if< std::is_same<decltype((std::declval<Func>())()), void>::value, bool>::type = true
>
void BenchmarkThreads(Func&& function, const char* name, uint32_t repeat, IncFunc&& inc_func, uint32_t init, uint32_t limit) {
	uint32_t num_cycles = 0;
	uint32_t curr_rep = repeat;
	uint32_t orig_rep = repeat;

	auto dynamic = omp_get_dynamic();
	omp_set_dynamic(0);

	while (init <= limit) {
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

		init = inc_func(init, num_cycles++);
	}

	omp_set_dynamic(dynamic);
}

template <typename Func, typename IncFunc,
	typename = typename std::enable_if< !std::is_same<decltype((std::declval<Func>())()), void>::value, bool>::type
>
decltype((std::declval<Func>())()) BenchmarkThreads(Func&& function, const char* name, uint32_t repeat
	, IncFunc&& inc_func, uint32_t init, uint32_t limit) {
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

		init = inc_func(init, num_cycles++);
	}

	omp_set_dynamic(dynamic);

	return ret;
}

#endif // !PARCO_BENCH
