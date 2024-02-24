#include <assert.h>
#include "ThreadPool.h"

Bass::ThreadPool::ThreadPool() : myShouldStop(false), myTasksInProgress(0)
{
	const size_t numberOfMainThreads = 1; //Currently we only have 1 main thread
	const size_t avaliableThreads = std::thread::hardware_concurrency();
	const size_t useableThreads = avaliableThreads - numberOfMainThreads; //Avoid unnessary context switching overhead due to more threads than there are hardware threads

	const int stopSingleCoreUsers = static_cast<int>(avaliableThreads - numberOfMainThreads);
	assert(stopSingleCoreUsers > 0 && "Bass does not allow single core non hyper-threading CPU users");

	myWorkers.reserve(useableThreads);

	for (size_t i = 0; i < useableThreads; ++i)
	{
		myWorkers.emplace_back([this]
			{
				while (true)
				{
					std::function<void()> task;

					{
						std::unique_lock<std::mutex> lock(myQueueMutex);
						myCondition.wait(lock, [this] { return myShouldStop || !myTaskQueue.empty(); });

						if (myShouldStop && myTaskQueue.empty())
							return;

						task = std::move(myTaskQueue.front());
						myTaskQueue.pop();
					}

					myQueueSize--;
					myTasksInProgress++;
					task();
					myTasksInProgress--;
				}
			});
	}
}

Bass::ThreadPool::~ThreadPool()
{
	myShouldStop = true;

	{
		std::unique_lock<std::mutex> lock(myQueueMutex);

		while (!myTaskQueue.empty())
		{
			myTaskQueue.pop();
			myQueueSize--;
		}
	}

	myCondition.notify_all();

	for (std::thread& worker : myWorkers)
	{
		if (worker.joinable())
			worker.join();
	}
}

size_t Bass::ThreadPool::GetTasksInProgress() const
{
	return myTasksInProgress.load();
}

size_t Bass::ThreadPool::GetQueueSize() const
{
	return myQueueSize.load();
}
