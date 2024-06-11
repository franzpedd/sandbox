#pragma once

#include <algorithm>
#include <mutex>
#include <vector>

namespace Cosmos
{
	// thread safe implementation of a stack using an std::vector
	template<typename T>
	class Stack
	{
	public:

		// constructor
		Stack() = default;

		// destructor
		~Stack() = default;

		// returns a reference to the elements vector
		inline std::vector<T>& GetElementsRef() { return mElements; }

	public:

		// returns if queue is empty
		bool Empty()
		{
			std::unique_lock<std::mutex> lock(mMutex);
			return mElements.empty();
		}

		// returns the queue size
		int Size()
		{
			std::unique_lock<std::mutex> lock(mMutex);
			return mElements.size();
		}

		// pushes an element to the top half of the stack
		void PushOver(T ent)
		{
			std::unique_lock<std::mutex> lock(mMutex);
			mElements.emplace_back(ent);
		}

		// pops an element from the top half of the stack
		void PopOver(T ent)
		{
			std::unique_lock<std::mutex> lock(mMutex);

			auto it = std::find(mElements.begin() + mMiddlePos, mElements.end(), ent);
			if (it != mElements.end())
				mElements.erase(it);
		}

		// pushes an element to the bottom half of the stack
		void Push(T ent)
		{
			std::unique_lock<std::mutex> lock(mMutex);

			mElements.emplace(mElements.begin() + mMiddlePos, ent);
			mMiddlePos++;
		}

		// pops an element from the bottom half of the stack
		void Pop(T ent)
		{
			std::unique_lock<std::mutex> lock(mMutex);

			auto it = std::find(mElements.begin(), mElements.begin() + mMiddlePos, ent);
			if (it != mElements.begin() + mMiddlePos)
			{
				mElements.erase(it);
				mMiddlePos--;
			}
		}

	private:

		std::mutex mMutex;
		std::vector<T> mElements;
		uint32_t mMiddlePos = 0;
	};
}