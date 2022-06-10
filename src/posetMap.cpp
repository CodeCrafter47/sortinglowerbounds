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


#include "posetMap.h"

#include "config.h"
#include "myHashmap.h"

PosetMap::PosetMap(size_t initialCapacity) : SposetMap() {

    numLocks = initialCapacity / 4096;
    numLocks = std::max(NCT::num_threads_glob, numLocks);
    numLocks = std::min(1u << 16, numLocks);
	
	//If items per hashmap is less than (1ULL << 12), initial load-factor is 0.52, otherwise 0.6
	double multiplier = static_cast<double>(initialCapacity) / numLocks < static_cast<double>((1ULL << 12)) ? 1.96 : 1.75;

    auto hmap_initial_capacity = static_cast<size_t>((static_cast<double>(initialCapacity) / numLocks) * multiplier);
    hmap_initial_capacity = std::max(993ul, hmap_initial_capacity);

    Scontainers.reserve(numLocks);
    SposetMap.reserve(numLocks);

    for (int lock = 0; lock < numLocks; lock++) {
        auto &container = Scontainers.emplace_back();
        SposetMap.emplace_back(std::ref(container), hmap_initial_capacity);
    }
}

uint64_t PosetMap::countPosets() {
    uint64_t result = 0;
    for (int lockId = 0; lockId < numLocks; lockId++) {
        result += Scontainers[lockId].size();
    }
    return result;
}

std::array<uint64_t, 8> PosetMap::countPosetsDetailed(bool unmarked) {
    std::array<uint64_t, 8> result = { 0, 0, 0, 0, 0, 0, 0,0 };
    for (int lockId = 0; lockId <  numLocks; lockId++) {
        std::array<uint64_t, 8> bla = Scontainers[lockId].countPosetsDetailed(unmarked);
        for (int i = 0; i < 8; i++)
            result[i] += bla[i];
    }
    return result;
}

PosetObj *PosetMap::find(AnnotatedPosetObj &candidate) {
    return SposetMap[candidate.GetLockHash() % numLocks].find(candidate);
}

PosetObj &PosetMap::findAndInsert(AnnotatedPosetObj &candidate) {
    uint64_t index = SposetMap[candidate.GetLockHash() % numLocks].findAndInsert(candidate);
    return Scontainers[candidate.GetLockHash() % numLocks].get(index);
}

void PosetMap::fill(std::vector<PosetObj>& vec) {
    vec.reserve(countPosets());
    for (int lockId = 0; lockId <  numLocks; lockId++) {
        uint64_t csize = Scontainers[lockId].size();
        for (unsigned int i = 0; i < csize; i++) {
            vec.push_back(Scontainers[lockId].get(i));
        }
    }
}

PosetMapExt::PosetMapExt(SemiOfflineVector<AnnotatedPosetObj> &container, size_t initialCapacity) : SposetMap() {

    numLocks = initialCapacity / 4096;
    numLocks = std::max(NCT::num_threads_glob, numLocks);
    numLocks = std::min(1u << 16, numLocks);
	
	//If items per hashmap is less than (1ULL << 12), initial load-factor is 0.52, otherwise 0.6
	double multiplier = static_cast<double>(initialCapacity) / numLocks < static_cast<double>((1ULL << 12)) ? 1.96 : 1.75;
	

    auto hmap_initial_capacity = static_cast<size_t>((static_cast<double>(initialCapacity) / numLocks) * multiplier);
    hmap_initial_capacity = std::max(993ul, hmap_initial_capacity);

    for (int lock = 0; lock < numLocks; lock++) {
        SposetMap.emplace_back(std::ref(container), hmap_initial_capacity);
    }
}

uint64_t PosetMapExt::findAndInsert(const AnnotatedPosetObj &candidate) {
    return SposetMap[candidate.GetLockHash() % numLocks].findAndInsert(candidate);
}

void PosetMapExt::clear() {
    for (auto &map: SposetMap) {
        map.clear();
    }
}