#pragma once

#include "Job.h"
#include "STLAllocator.h"


class JobQueue
{
public:
	JobQueue() : mHead(&mStub), mTail(&mStub)
	{
		mOffset = offsetof(struct JobEntry, mNodeEntry);
		_ASSERT_CRASH(mHead.is_lock_free());
	}
	~JobQueue() {}

	/// mutiple produce
	void Push(JobEntry* newData)
	{
		NodeEntry* prevNode = (NodeEntry*)std::atomic_exchange_explicit(&mHead,
			&newData->mNodeEntry, std::memory_order_acq_rel);

		prevNode->mNext = &(newData->mNodeEntry);
	}

	/// single consume
	JobEntry* Pop()
	{
		NodeEntry* tail = mTail;
		NodeEntry* next = tail->mNext;

		if (tail == &mStub)
		{
			/// in case of empty
			if (nullptr == next)
				return nullptr;

			/// first pop
			mTail = next;
			tail = next;
			next = next->mNext;
		}

		/// in most cases...
		if (next)
		{
			mTail = next;

			return reinterpret_cast<JobEntry*>(reinterpret_cast<int64_t>(tail) - mOffset);
		}

		NodeEntry* head = mHead;
		if (tail != head)
			return nullptr;

		/// last pop
		mStub.mNext = nullptr;

		NodeEntry* prevNode = (NodeEntry*)std::atomic_exchange_explicit(&mHead,
			&mStub, std::memory_order_acq_rel);

		prevNode->mNext = &mStub;

		next = tail->mNext;
		if (next)
		{
			mTail = next;

			return reinterpret_cast<JobEntry*>(reinterpret_cast<int64_t>(tail) - mOffset);
		}

		return nullptr;
	}


private:

	std::atomic<NodeEntry*> mHead;

	NodeEntry* mTail;
	NodeEntry			mStub;

	int64_t				mOffset;

};
