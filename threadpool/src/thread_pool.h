#ifndef YY_THREAD_POOL_H
#define YY_THREAD_POOL_H

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <atomic>

namespace tp {

	enum {
		AUTODETECT = 0
	};

	class ThreadPool {

		public:

		// Constructor creates "threads_count" number of workers.
		// AUTODETECT, sets it to number of hardware thread contexts.
		explicit ThreadPool(size_t threads_count = AUTODETECT);

		// Not copyable
		ThreadPool(const ThreadPool &) = delete;
		ThreadPool& operator= (const ThreadPool &) = delete;

		// Not moveable
		ThreadPool(ThreadPool &&) = delete;
		ThreadPool& operator= (const ThreadPool &&) = delete;

		// the destructor.
		~ThreadPool();

		// Adds new task to the tasks queue.
		// It will be processed by one of the worker threads in the future.
		// Returned future object can be used access the results.
		template<typename Callable, typename... Args>
		auto enqueue(Callable&& callable, Args&&... args)
			->std::future<typename std::result_of<Callable(Args...)>::type>;

		// Enqueue task without requiring capture of std::future<>
		template<typename Callable, typename... Args>
		auto enqueueAndDetach(Callable&& callable, Args&&... args)
			-> void;

		// Number of tasks waiting in the queue.
		size_t queueSize();

		// Blocks execution until all tasks in the queue is finished.
		void waitUntilEmpty();

		private:

		// Worker threads.
		std::vector<std::thread> workers_;

		// The task queue.
		std::queue<std::function<void()>> tasks_;

		// Synchronization over condition for queue operations
		// and the stop flag.
		std::mutex queue_mutex_;
		std::condition_variable condition_producers_;
		std::condition_variable condition_consumers_;
		bool stop_;
		std::atomic<int> working_threads_counter_;

	};

	ThreadPool::ThreadPool(size_t threads_count)
		: stop_(false) {

		working_threads_counter_.store(0, std::memory_order_relaxed);

		auto workers_count = threads_count;
		if (threads_count == AUTODETECT) {
			workers_count = std::thread::hardware_concurrency();
		}

		if (workers_count == 0) {
			workers_count = 1;
		}

		workers_.reserve(workers_count);
		for (size_t i = 0; i < workers_count; ++i) {
			workers_.emplace_back(
				[this] {
				for (;;) {
					std::unique_lock<std::mutex> lock(this->queue_mutex_);
					this->condition_consumers_.wait(lock,
										  [this] { return this->stop_ || !this->tasks_.empty(); });
					if (this->stop_ && this->tasks_.empty()) {
						return;
					}
					auto task = std::move(this->tasks_.front());
					this->tasks_.pop();
					lock.unlock();

					working_threads_counter_++;
					task();
					working_threads_counter_--;

					lock.lock();					
					if (tasks_.empty() &&
						working_threads_counter_.load(std::memory_order_relaxed) == 0) {
						condition_producers_.notify_all();
					}
					lock.unlock();
				}
			}
			);
		}
	}

	template<typename Callable, typename... Args>
	auto ThreadPool::enqueue(Callable&& callable, Args&&... args)
		-> std::future<typename std::result_of<Callable(Args...)>::type> {
		using ReturnType = typename std::result_of<Callable(Args...)>::type;
		using Task = std::packaged_task<ReturnType()>;

		auto task = std::make_shared<Task>(
			std::bind(std::forward<Callable>(callable),
					  std::forward<Args>(args)...));

		std::future<ReturnType> res = task->get_future();
		{
			std::unique_lock<std::mutex> lock(queue_mutex_);

			if (stop_) {
				throw std::runtime_error("enqueue on stopped ThreadPool");
			}

			tasks_.emplace([task]() { (*task)(); });
		}
		condition_consumers_.notify_one();
		return res;
	}

	template<typename Callable, typename... Args>
	auto ThreadPool::enqueueAndDetach(Callable&& callable, Args&&... args)
		-> void {
			{
				std::unique_lock<std::mutex> lock(queue_mutex_);

				// Don't allow an enqueue after stopping
				if (stop)
					throw std::runtime_error("enqueue on stopped ThreadPool");

				// Push work back on the queue
				tasks.emplace(std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...));
			}
			condition.notify_one();
	}

	ThreadPool::~ThreadPool() {

		std::unique_lock<std::mutex> lock(queue_mutex_);
		stop_ = true;
		lock.unlock();

		condition_consumers_.notify_all();
		condition_producers_.notify_all();
		for (auto& worker : workers_) {
			if (worker.joinable()) {
				worker.join();
			}
		}
	}

	size_t ThreadPool::queueSize() {
		std::unique_lock<std::mutex> lock(queue_mutex_);
		size_t size = tasks_.size();
		return size;
	}

	void ThreadPool::waitUntilEmpty() {
		std::unique_lock<std::mutex> lock(queue_mutex_);
		condition_producers_.wait(lock, 
						[this] {
			return (tasks_.empty() &&
					working_threads_counter_.load(std::memory_order_relaxed) == 0);
		});
	}

}


#endif // YY_THREAD_POOL_H