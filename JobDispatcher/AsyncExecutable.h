#pragma once

#include "JobQueue.h"
#include "Timer.h"


class AsyncExecutable
{
public:

	AsyncExecutable() : mRemainTaskCount(0), mRefCount(0) {}
	virtual ~AsyncExecutable()
	{
		_ASSERT_CRASH(mRefCount == 0);
	}


	template <class T, class... Args>
	void DoAsync(void (T::* memfunc)(Args...), Args&&... args)
	{
		auto* job = new Job<T, Args...>(static_cast<T*>(this), memfunc, std::forward<Args>(args)...);
		DoTask(job);
	}

	template <class T, class... Args>
	void DoAsyncAfter(uint32_t after, void (T::* memfunc)(Args...), Args&&... args)
	{
		auto* job = new Job<T, Args...>(static_cast<T*>(this), memfunc, std::forward<Args>(args)...);
		LTimer->PushTimerJob(this, after, job);
	}

	void AddRefForThis()
	{
		mRefCount.fetch_add(1);
	}

	void ReleaseRefForThis()
	{
		mRefCount.fetch_sub(1);
	}

	/// Push a task into Job Queue, and then Execute tasks if possible
	void DoTask(JobEntry* task)
	{
		if (mRemainTaskCount.fetch_add(1) != 0)
		{
			/// register the task in this dispatcher
			mJobQueue.Push(task);
		}
		else
		{
			/// register the task in this dispatcher
			mJobQueue.Push(task);

			AddRefForThis(); ///< refcount +1 for this object

			/// Does any dispathcer exist occupying this worker-thread at this moment?
			if (LCurrentExecuterOccupyingThisThread != nullptr)
			{
				/// just register this dispatcher in this worker-thread
				LExecuterList->push_back(this);
			}
			else
			{
				/// acquire
				LCurrentExecuterOccupyingThisThread = this;

				/// invokes all tasks of this dispatcher
				Flush();

				/// invokes all tasks of other dispatchers registered in this thread
				while (!LExecuterList->empty())
				{
					AsyncExecutable* dispacher = LExecuterList->front();
					LExecuterList->pop_front();
					dispacher->Flush();
					dispacher->ReleaseRefForThis();
				}

				/// release 
				LCurrentExecuterOccupyingThisThread = nullptr;
				ReleaseRefForThis(); ///< refcount -1 for this object
			}
		}
	}


private:	
	/// Execute all tasks registered in JobQueue of this dispatcher
	void Flush()
	{
		while (true)
		{
			if (JobEntry* job = mJobQueue.Pop())
			{
				job->OnExecute();
				delete job;

				if (mRemainTaskCount.fetch_sub(1) == 1)
					break;
			}
		}
	}

private:
	/// member variables
	JobQueue	mJobQueue;

	std::atomic<int64_t> mRemainTaskCount;

	/// should not release this object when it is in the dispatcher
	std::atomic<int32_t> mRefCount;

	//friend class Timer;
};