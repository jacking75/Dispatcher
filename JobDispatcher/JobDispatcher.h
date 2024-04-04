#pragma once

#include "STLAllocator.h"
#include "Job.h"
#include "JobQueue.h"
#include "AsyncExecutable.h"
#include "Timer.h"
#include "ThreadLocal.h"
#include "Runnable.h"

#include <atomic>
#include <thread>



template <class T>
class JobDispatcher
{
public:
	JobDispatcher(int workerCount) : mWorkerThreadCount(workerCount)
	{
		static_assert(true == std::is_convertible<T*, Runnable*>::value, "only allowed when Runnable");
	}

	void RunWorkerThreads()
	{
		for (int i = 0; i < mWorkerThreadCount; ++i)
		{
			mWorkerThreadList.emplace_back(std::thread(&T::ThreadRun, std::make_unique<T>()));
		}

		for (auto& thread : mWorkerThreadList)
		{
			if (thread.joinable())
				thread.join();
		}
	}

private:

	int mWorkerThreadCount;
	std::vector<std::thread> mWorkerThreadList;

};