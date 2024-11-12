#ifndef PARCO_BENCH
#define PARCO_BENCH

#include <iostream>
#include <chrono>

template <typename Func,
	std::enable_if_t< std::is_same_v<decltype((std::declval<Func>())()), void>, bool> = true
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
	typename = std::enable_if_t< !std::is_same_v<decltype((std::declval<Func>())()), void>, bool>
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

#endif // !PARCO_BENCH
