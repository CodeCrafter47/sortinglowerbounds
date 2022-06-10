// MIT License
//
// Copyright (c) 2022 Florian Stober and Armin Weiß
// Institute for Formal Methods of Computer Science, University of Stuttgart
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef MYHASHMAP_H
#define MYHASHMAP_H

#include <vector>
#include <mutex>

#include "posetObj.h"
#include "stats.h"
#include "isoTest.h"
#include "eventLog.h"
#include "posetHandle.h"

namespace {
	float computeLoadFactor(uint64_t capacity) {
		if (capacity < (1ULL << 8))
		{
			return 0.45f;
		}
		else if (capacity < (1ULL << 12))
		{
			return 0.52f;
		}
		else if (capacity < (1ULL << 16))
		{
			return 0.6f;
		}
		else if (capacity < (3ULL << 17))
		{
			return 0.68f;
		}
		else
		{
			return 0.75f;
		}
	}
}

template<class Ptr, class Container>
class MyHashmap {
	 
	std::vector<Ptr> data;
	
	std::size_t capacity;
	std::size_t numElements;
	
	float loadFactor = 0.45f;

	uint64_t gen = 0;

public:
	Container& container;
	alignas(64) std::mutex mutex;

public:

	MyHashmap(MyHashmap && other) noexcept : numElements(other.numElements), capacity(other.capacity), data(std::move(other.data)), container(other.container) { }
	MyHashmap& operator= (MyHashmap&) = delete;
	MyHashmap& operator= (MyHashmap&& other) noexcept {
		numElements = other.numElements;
		capacity = other.capacity;
		data = std::move(other.data);
		container = other.container;
		return *this;
	};

    explicit MyHashmap(Container &container, size_t initialCapacity = 973) : container(container) {
        numElements = 0;
        capacity = initialCapacity;
        data.resize(capacity);
		loadFactor = computeLoadFactor(capacity);
    }

	void clear() {
		this->gen += 1;
		if (this->gen >= Ptr::posetGenMAX) {
			this->gen = 0;
			std::fill(data.begin(), data.end(), Ptr{0, Ptr::posetRefIndexInvalid, 0});
		}
        this->numElements = 0;
	}

	/**
	 * Find a poset in the hash map. Returns nullptr if not found.
	 */
	PosetObj* find(AnnotatedPosetObj& candidate) {
		std::lock_guard<std::mutex> lock{mutex};

		assert(capacity != 0);
		std::size_t index = (candidate.GetHash() % capacity);

		std::size_t startI = 0;
		std::size_t i = 0;

		assert(index < capacity);
		while(data[index].isValid(gen)) {

			Ptr& entryPtr = data[index];
			if (testEquality(candidate, entryPtr))
			{
				PosetObj& entry = container.get(entryPtr.GetPosetRefIndex());
				Stats::addVal<AVMSTAT::HFindGlobNStepsPos>(i + 1);
				return &entry;
			}
			i++;

			//if i >= capacity, then the element is not in the hashmap
			if (i>= capacity){
				assert(i == capacity);
				std::cout << "i>= capacity. i:" << i << " capacity: " << capacity << " num_elements: " << numElements << std::endl << std::endl;
				return nullptr;
			}

			assert(i < capacity);
			index +=i;
			if(index >= capacity)
				index-=capacity;
			DEBUG_ASSERT(index < capacity);
		}
		Stats::addVal<AVMSTAT::HFindGlobNStepsNeg>(i);
		return nullptr;
	}

	/**
	 * Find a poset in the hash map. Insert if not found. Returns index of the poset in the container.
	 */
	uint64_t findAndInsert(const AnnotatedPosetObj& candidate) {
		std::lock_guard<std::mutex> lock{mutex};

		beginning:
		assert(capacity != 0);

		if (static_cast<float>(numElements) >= loadFactor * static_cast<float>(capacity)) {
			rehash();
		}

		std::size_t index = candidate.GetHash() % capacity;
		std::size_t startI = 0;
		std::size_t i = 0;

		assert(index < capacity);
		while (data[index].isValid(gen)) {

			// check if the entry is equal to the candidate
			Ptr& entryPtr = data[index];
			if (testEquality(candidate, entryPtr)) {
				Stats::addVal<AVMSTAT::HFindGlobNStepsPos>(i + 1);
				return entryPtr.GetPosetRefIndex();
			}
			i++;

			// check for rehash
			if (i>= capacity or i >= (1ULL << 16)) {
				rehash();
				EventLog::write(true, "rehash required because no suitable position found. i:" + std::to_string(i) + " capacity: " + std::to_string(capacity));
				goto beginning;
			}
			assert(i < capacity);
			index +=i;
			if(index >= capacity)
				index-=capacity;
			DEBUG_ASSERT(index < capacity);
		}

		// insert
		auto pointer = container.insert(candidate);
		data[index] = Ptr(candidate.GetPointerHash(Ptr::moreHashWidth), pointer, gen);
		numElements++;
		return pointer;
	}

private:

	void rehash() {
		if (capacity < (1ULL << 5)) {
			capacity *= 5ULL;
		} else if (capacity < (3ULL << 9)) {
			capacity *= 2ULL;
		} else if (capacity < (3ULL << 12)) {
			capacity = static_cast<std::size_t>(static_cast<double>(capacity) * 1.7);
		} else if (capacity < (3ULL << 15)) {
			capacity = static_cast<std::size_t>(static_cast<double>(capacity) * 1.5);
		} else {
			capacity = static_cast<std::size_t>(static_cast<double>(capacity) * 1.3);
		}
		if (capacity % 2 == 0)
			capacity += 1;
		if (capacity % 3 == 0)
			capacity += 2;

		rehashInternal();
	}

	void rehashInternal() {
		loadFactor = computeLoadFactor(capacity);
		std::vector<Ptr> temp(capacity);

		unsigned int count = 0;
		for(auto pointer : data) {
			if (pointer.isValid(gen)) {
				count++;
				std::size_t i = 0;
				PosetObj &poset = container.get(pointer.GetPosetRefIndex());
				PosetHandleFull currentHandle = PosetHandleFull::fromPoset(poset);
				std::size_t index = currentHandle.GetHash() % capacity;
				while (temp[index].isValid(gen)) {
					i++;
					assert(i < capacity);
					index += i;
					if (index >= capacity)
						index -= capacity;

				}
				assert(i < capacity && index < capacity);
				temp[index] = pointer;
			}
		}
		std::swap(data,temp);
	}

	bool testEquality(const AnnotatedPosetObj& candidate, Ptr& entryPointer) const {
		// check pointer hash
		Stats::inc(STAT::NPtrHashEqualTest);
		if (candidate.GetPointerHash(Ptr::moreHashWidth) != entryPointer.GetPointerHash()) {
			Stats::inc(STAT::NPointerHashDiff);
			return false;
		}

		// check equality
		auto& entry = container.get(entryPointer.GetPosetRefIndex());
		Stats::inc(STAT::NEqualTest);

		if (candidate.isUniqueGraph() != entry.isUniqueGraph() || candidate.GetSelfdualId() != entry.GetSelfdualId())
		{
			Stats::inc(STAT::NInPosetHashDiff);
			return false;
		}

		Stats::inc(STAT::NIsoTest);
		if (candidate.SameGraph(entry)){
			Stats::inc(NIsoPositive);
			assert(candidate.isUniqueGraph() == entry.isUniqueGraph());
			return true;
		}

		//if the graph is unique and it does not agree bit-wise, we know that the graphs cannot be isomorphic
		if (candidate.isUniqueGraph() && !candidate.GetSelfdualId())
			return false;

		unsigned int reduced_n = candidate.GetReducedN();
		if(!entry.isSingletonsAbove(candidate.GetFirstSingleton())){
			Stats::inc(STAT::NSingletonsDiff);
			return false;
		}

		if(!entry.isPairs(reduced_n, candidate.GetNumPairs())){
			Stats::inc(STAT::NPairsDiff);
			return false;
		}
		if (candidate.GetSelfdualId())
		{
			if (boost_is_isomorphic(candidate, entry, reduced_n))
				return true;
			return boost_is_rev_isomorphic(candidate, entry, reduced_n);
		}
		else
			return boost_is_isomorphic(candidate, entry, reduced_n);
	}
	
};


#endif