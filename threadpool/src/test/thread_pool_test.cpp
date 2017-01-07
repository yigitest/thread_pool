#include "test/catch.hpp"
#include "test/scoped_cout_timer.h"

#include "thread_pool.h"

#include <string>
#include <random>

TEST_CASE("single core vs threads", "[thread_pool_test]") {
	
	const int lower_bound = 0;
	const int upper_bound = 1000000;
	const int vector_size = 10000;
	const int vector_count = 1000;
	
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(lower_bound, upper_bound);

	
	std::vector<std::vector<int>> vec(vector_count);
	for (int v = 0; v < vector_count; ++v) {
		auto& row = vec[v];
		row.reserve(vector_size);
		for (int i = 0; i < vector_size; ++i) {
			row.emplace_back(dist(gen));
		}
	}
	

	{
		ScopedCoutTimer timer;
		for (int v = 0; v < vector_count; ++v) {
			auto row = vec[v]; //copy
			std::sort(row.begin(), row.end());
		}
	}

	{
		ScopedCoutTimer timer;
		tp::ThreadPool tp(2);
		std::vector< std::future<std::vector<int>> > results;	

		for (int v = 0; v < vector_count; ++v) {
			results.emplace_back(
				tp.enqueue([&vec, v] {
				auto row = vec[v]; //copy
				std::sort(row.begin(), row.end());
				return row;
			}));
		}

		for (auto && result : results) {
			result.get();
		}
	}


	{
		ScopedCoutTimer timer;
		tp::ThreadPool tp(4);
		std::vector< std::future<std::vector<int>> > results;
		for (int v = 0; v < vector_count; ++v) {
			results.emplace_back(
				tp.enqueue([&vec, v] {
				auto row = vec[v]; //copy
				std::sort(row.begin(), row.end());
				return row;
			}));
		}

		for (auto && result : results) {
			result.get();
		}
	}

	{
		ScopedCoutTimer timer;
		tp::ThreadPool tp(8);
		std::vector< std::future<std::vector<int>> > results;
		for (int v = 0; v < vector_count; ++v) {
			results.emplace_back(
				tp.enqueue([&vec, v] {
				auto row = vec[v]; //copy
				std::sort(row.begin(), row.end());
				return row;
			}));
		}

		for (auto && result : results) {
			result.get();
		}
	}


	{
		ScopedCoutTimer timer;
		tp::ThreadPool tp(tp::AUTODETECT);
		std::vector< std::future<std::vector<int>> > results;
		for (int v = 0; v < vector_count; ++v) {
			results.emplace_back(
				tp.enqueue([&vec, v] {
				auto row = vec[v]; //copy
				std::sort(row.begin(), row.end());
				return row;
			}));
		}

		for (auto && result : results) {
			result.get();
		}
	}

	
	REQUIRE(true);
}

TEST_CASE("wait for empty task queue", "[thread_pool_test]") {

	const int lower_bound = 0;
	const int upper_bound = 1000000;
	const int vector_size = 10000;
	const int vector_count = 1000;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(lower_bound, upper_bound);


	std::vector<std::vector<int>> vec(vector_count);
	for (int v = 0; v < vector_count; ++v) {
		auto& row = vec[v];
		row.reserve(vector_size);
		for (int i = 0; i < vector_size; ++i) {
			row.emplace_back(dist(gen));
		}
	}

	{
		ScopedCoutTimer timer_single;
		tp::ThreadPool tp(4);
		std::vector< std::future<std::vector<int>> > results;
		for (int v = 0; v < vector_count; ++v) {
			results.emplace_back(
				tp.enqueue([&vec, v] {
				auto row = vec[v]; //copy
				std::sort(row.begin(), row.end());
				return row;
			}));
		}

		tp.waitUntilEmpty();
		REQUIRE(tp.queueSize() == 0);

		// add another batch of tasks.
		for (int v = 0; v < vector_count; ++v) {
			results.emplace_back(
				tp.enqueue([&vec, v] {
				auto row = vec[v]; //copy
				std::sort(row.begin(), row.end());
				return row;
			}));
		}

		tp.waitUntilEmpty();
		REQUIRE(tp.queueSize() == 0);
	}

}