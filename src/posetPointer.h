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

#ifndef PosetPointer_H
#define PosetPointer_H

#include "config.h"
#include <cassert>

/**
 * Addresses a poset inside a PosetContainer. Also holds a few bits of the posets hash value to speed up poset comparisons.
 */
template<unsigned posetRefIndexWidth, unsigned pointerHashWidth, unsigned pointerGenWidth>
class PosetPointer {

public:

    static constexpr uint64_t moreHashWidth = pointerHashWidth;
    static constexpr uint64_t moreHashMAX = ((1ULL << (pointerHashWidth - 1)) - 1) | (1ULL << (pointerHashWidth - 1));
    static constexpr uint64_t posetRefIndexInvalid = (((1ULL << (posetRefIndexWidth - 1)) - 1) | (1ULL << (posetRefIndexWidth - 1)));
    static constexpr uint64_t posetRefIndexMAX = posetRefIndexInvalid - 1;
    static constexpr uint64_t posetGenMAX = ((1ULL << (pointerGenWidth - 1)) - 1) | (1ULL << (pointerGenWidth - 1));

private:

    uint64_t posetRefIndex: posetRefIndexWidth __attribute__ ((packed));
    uint64_t pointerHash: pointerHashWidth __attribute__ ((packed));
    uint64_t pointerGen: pointerGenWidth __attribute__ ((packed));


public:

    PosetPointer() : posetRefIndex(posetRefIndexInvalid), pointerHash(0) {}

    PosetPointer(const PosetPointer &) = default;

    PosetPointer(uint64_t hash, uint64_t posRefIndex, uint64_t gen) : posetRefIndex(posRefIndex), pointerHash(hash), pointerGen(gen) {
        assert(posRefIndex <= posetRefIndexMAX);
        assert(hash <= moreHashMAX);
        assert(gen <= posetGenMAX);
    }

    bool operator==(const PosetPointer &other) const {
        return this->pointerHash == other.pointerHash && this->posetRefIndex == other.posetRefIndex;
    }

    [[nodiscard]] uint64_t GetPointerHash() const {
        return this->pointerHash;
    }

    [[nodiscard]] uint64_t GetPosetRefIndex() const {
        return this->posetRefIndex;
    }

    [[nodiscard]] bool isValid(uint64_t gen) const {
        return this->posetRefIndex != posetRefIndexInvalid && this->pointerGen == gen;
    }
};

#endif


