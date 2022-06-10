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


#ifndef SORTINGLOWERBOUNDS_POSETMAP_H
#define SORTINGLOWERBOUNDS_POSETMAP_H

#include <vector>
#include <array>
#include <cstdint>

#include "myHashmap.h"
#include "posetPointer.h"
#include "posetContainer.h"
#include "semiOfflineVector.h"

class PosetObj;
class PosetHandleFull;

class PosetMap {

    uint32_t numLocks;

public:

    alignas(64) std::vector<MyHashmap<PosetPointer<24, 7, 1>, PosetContainerTemplate>>  SposetMap;
    alignas(64) std::vector<PosetContainerTemplate>  Scontainers;

    PosetMap(const PosetMap&) = delete;
    PosetMap(PosetMap&&) noexcept = default;
    PosetMap& operator= (PosetMap&) = delete;
    PosetMap& operator= (PosetMap&&) noexcept = default;

    explicit PosetMap(size_t initialCapacity);

    /**
     * Find a poset in the hash map. Returns nullptr if not found.
     */
    PosetObj* find(AnnotatedPosetObj& candidate);

    /**
     * Find a poset in the hash map. Insert if not found. Returns a reference to the existing or inserted poset.
     */
    PosetObj& findAndInsert(AnnotatedPosetObj& candidate);

    uint64_t countPosets();

    std::array<uint64_t, 8> countPosetsDetailed(bool unmarked = true);

    void fill(std::vector<PosetObj>& vec);

};



class PosetMapExt {

    uint32_t numLocks;

public:

    alignas(64) std::vector<MyHashmap<PosetPointer<64, 32, 32>, SemiOfflineVector<AnnotatedPosetObj>>> SposetMap;

    explicit PosetMapExt(SemiOfflineVector<AnnotatedPosetObj> &container, size_t initialCapacity);

    /**
     * Find a poset in the hash map. Insert if not found. Returns a reference to the existing or inserted poset.
     */
    uint64_t findAndInsert(const AnnotatedPosetObj& candidate);

    void clear();
};

#endif //SORTINGLOWERBOUNDS_POSETMAP_H
