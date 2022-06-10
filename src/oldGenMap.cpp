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


#include <cstdlib>

#include "oldGenMap.h"
#include "stats.h"
#include "isoTest.h"
#include <boost/interprocess/allocators/allocator.hpp>

OldGenMap::OldGenMap(boost::interprocess::managed_mapped_file::segment_manager *segment_manager, size_t size) :
size(size),
segment_manager(segment_manager),
empty(true) {
    posetArray = static_cast<PosetObj*>(segment_manager->allocate(sizeof(PosetObj) * size));
    hashArray = static_cast<uint16_t*>(malloc(sizeof(uint16_t) * size));
    for (size_t i = 0; i < size; i++) {
        hashArray[i] = ((1u << 16) - 1);
    }
    profile.fill(0);
    profileStorage.fill(0);
}

OldGenMap::~OldGenMap() {
    free(hashArray);
    segment_manager->deallocate(posetArray);
}

void OldGenMap::insert(const AnnotatedPosetObj& poset) {
    profile[poset.GetStatus()]++;

    uint64_t index = (poset.GetHash() * MULT1) % size;
    uint16_t hash = (poset.GetHash() * MULT2) % ((1u << 16) - 1);

    if (hashArray[index] == ((1u << 16) - 1) || poset.GetStatus() == SortableStatus::YES) {
        if (hashArray[index] != ((1u << 16) - 1)) {
            profileStorage[posetArray[index].GetStatus()]--;
        }
        hashArray[index] = hash;
        posetArray[index] = poset;
        profileStorage[poset.GetStatus()]++;
    }
    empty = false;
}

PosetObj* OldGenMap::find(const AnnotatedPosetObj& poset) {
    if (empty) {
        return nullptr;
    }

    uint64_t index = (poset.GetHash() * MULT1) % size;
    uint16_t hash = (poset.GetHash() * MULT2) % ((1u << 16) - 1);

    if (hashArray[index] != hash) {
        return nullptr;
    }

    auto &entry = posetArray[index];

    // check equality
    Stats::inc(STAT::NEqualTest);

    if (poset.isUniqueGraph() != entry.isUniqueGraph() || poset.GetSelfdualId() != entry.GetSelfdualId())
    {
        Stats::inc(STAT::NInPosetHashDiff);
        return nullptr;
    }

    Stats::inc(STAT::NIsoTest);
    if (poset.SameGraph(entry)){
        Stats::inc(NIsoPositive);
        assert(poset.isUniqueGraph() == entry.isUniqueGraph());
        return &entry;
    }

    //if the graph is unique and it does not agree bit-wise, we know that the graphs cannot be isomorphic
    if (poset.isUniqueGraph() && !poset.GetSelfdualId())
        return nullptr;

    unsigned int reduced_n = poset.GetReducedN();
    if(!entry.isSingletonsAbove(poset.GetFirstSingleton())){
        Stats::inc(STAT::NSingletonsDiff);
        return nullptr;
    }

    if(!entry.isPairs(reduced_n, poset.GetNumPairs())) {
        Stats::inc(STAT::NPairsDiff);
        return nullptr;
    }
    if (poset.GetSelfdualId()) {
        if (boost_is_isomorphic(poset, entry, reduced_n))
            return &entry;
        if (boost_is_rev_isomorphic(poset, entry, reduced_n))
            return &entry;
    } else {
        if (boost_is_isomorphic(poset, entry, reduced_n))
            return &entry;
    }
    return nullptr;
}
