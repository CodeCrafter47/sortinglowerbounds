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

#ifndef PosetInfo_H
#define PosetInfo_H

#include "config.h"

class PosetObj;

class PosetInfo {
		
	uint8_t numSingletons;
    uint8_t numPairs;

public:

    PosetInfo(unsigned int numSingletons, unsigned int numPairs) :
            numSingletons(numSingletons), numPairs(numPairs) { }
	
    static PosetInfo fromPoset(PosetObj& poset);

    [[nodiscard]] inline unsigned int GetnumSingletons() const {
        return numSingletons;
    }

    [[nodiscard]] inline unsigned int GetNumPairs() const {
        return numPairs;
    }

    [[nodiscard]] inline unsigned int GetReducedN() const {
        return (NCT::N - (2U * GetNumPairs())) - GetnumSingletons();
    }

    [[nodiscard]] inline unsigned int GetFirstSingleton() const {
        return NCT::N - GetnumSingletons();
    }

    [[nodiscard]] inline unsigned int GetFirstInPair() const {
        return (NCT::N - (2U * GetNumPairs())) - GetnumSingletons();
    }

    [[nodiscard]] inline bool isSingleton(unsigned int i) const {
        DEBUG_ASSERT(i < NCT::N);
        return i >= GetFirstSingleton();
    }

    [[nodiscard]] inline bool isInBigPart(unsigned int i) const {
        DEBUG_ASSERT(i < NCT::N);
        return i < GetFirstInPair();
    }

    [[nodiscard]] inline bool isInPair(unsigned int i) const {
        DEBUG_ASSERT(i < NCT::N);
        bool result = (i < GetFirstSingleton()) && (i >= GetFirstInPair());
        return result;
    }

    [[nodiscard]] inline bool isPairComp(unsigned int i, unsigned int j) const {
        DEBUG_ASSERT(i < NCT::N && j < NCT::N);
        return isInPair(i) && isInPair(j);
    }
};


class PosetInfoFull : public PosetInfo {
	uint64_t hash;

public:

	PosetInfoFull(const PosetInfo& info, uint64_t hashValue):
		PosetInfo(info),
		hash(hashValue) { }
	
	PosetInfoFull(const PosetInfoFull & other ) = default;

	[[nodiscard]] uint32_t GetLockHash() const {
		return hash % PRIME2;
	}

	[[nodiscard]] uint64_t GetHash() const {
		return hash;
	}

	[[nodiscard]] uint64_t GetPointerHash(unsigned int width) const {
		return (hash % PRIME3) & ((1UL << width) - 1);
	}
};

#endif