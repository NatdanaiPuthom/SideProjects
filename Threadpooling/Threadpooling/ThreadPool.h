#pragma once

#include <queue>
#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <future>
#include <functional>
#include <condition_variable>

//Note!
//Please make sure the lifetime of the Object exceeds that lifetime of the task currently begin performed by ThreadPool
//Use shared pointers!

namespace Bass
{
	class ThreadPool
	{
	public:
		ThreadPool();
		~ThreadPool();

		template <typename Function, typename...Args>
		inline auto AddTask(Function&& aFunction, Args&&... someArguments) -> std::future<typename std::invoke_result<Function, Args...>::type>;

		size_t GetTasksInProgress() const;
		size_t GetQueueSize() const;
	private:
		std::vector<std::thread> myWorkers;
		std::queue<std::function<void()>> myTaskQueue;

		std::mutex myQueueMutex;
		std::condition_variable myCondition;
		std::atomic<size_t> myTasksInProgress;
		std::atomic<size_t> myQueueSize;
		std::atomic<bool> myShouldStop;
	};
}

template<typename Function, typename ...Args>
inline auto Bass::ThreadPool::AddTask(Function&& aFunction, Args && ...someArguments) -> std::future<typename std::invoke_result<Function, Args ...>::type>
{
	using return_type = typename std::invoke_result<Function, Args...>::type;

	auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<Function>(aFunction), std::forward<Args>(someArguments)...));

	std::future<return_type> result = task->get_future();
	{
		std::unique_lock<std::mutex> lock(myQueueMutex);

		if (myShouldStop)
			throw std::runtime_error("Cannot add task on stopped ThreadPool");

		myTaskQueue.emplace([task]() {(*task)(); });
	}

	myQueueSize++;
	myCondition.notify_one();

	return result;
}
