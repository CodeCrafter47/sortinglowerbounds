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

#pragma once
#ifndef POSET_OBJ_CORE_H
#define POSET_OBJ_CORE_H

#include <cstdint>
#include <array>
#include <utility>

#include "config.h"
#include "sortableStatus.h"


class PosetObjCore {

public:

    static constexpr unsigned int numGraphBits = (MAXN* (MAXN - 1)) / 2;

#ifdef VISUAL_STUDIO
	static constexpr unsigned int statusWidth = 3; // one more bit because of strange Visual Studio ENUMs
#else
	static constexpr unsigned int statusWidth = 2;
#endif

private:
	
	static constexpr unsigned int wordLength = 8;
	

    static constexpr unsigned int numMainGraphChars = (numGraphBits - 1) / wordLength;
    static constexpr unsigned int numLastGraphBits = numGraphBits - numMainGraphChars * wordLength;
	

private:
    uint8_t graphMain[numMainGraphChars];
    uint8_t graphLastBits : numLastGraphBits;
	
	

    SortableStatus status : statusWidth;   
	uint8_t selfdualId : 1;	
	uint8_t uniqueGraph : 1;
    uint8_t mark : 1;
	



public:


    PosetObjCore(const PosetObjCore& other) = default;

    PosetObjCore();
	
    [[nodiscard]] inline bool isUniqueGraph() const {
        return uniqueGraph;
    }

    inline void setUniqueGraph(bool unique) {
        uniqueGraph = unique;
    }

    [[nodiscard]] inline SortableStatus GetStatus() const {
        return status;
    }

    inline void SetStatus(SortableStatus newStatus) {
        status = newStatus;
    }

    [[nodiscard]] inline bool GetSelfdualId() const {
        return (selfdualId == 1);
    }

    inline void SetSelfdualId(bool newId) {
        if (newId)
            selfdualId = 1;
        else
            selfdualId = 0;
    }

    [[nodiscard]] inline bool isMarked() const {
        return mark;
    }

    inline void setMark(bool pMark) {
        mark = pMark;
    }
	
	[[nodiscard]] uint64_t hashFromGraph() const;

    [[nodiscard]] bool SameGraph(const PosetObjCore& other) const;

    [[nodiscard]] inline bool graphGet(unsigned int i) const {
        DEBUG_ASSERT(0 <= i && i < numGraphBits);
        unsigned int outerIndex = i / wordLength;
        unsigned int innerIndex = i % wordLength;
        if (outerIndex < numMainGraphChars)
            return (graphMain[outerIndex] & (1ULL << innerIndex)) != 0;
        else
        {
            DEBUG_ASSERT(innerIndex < numLastGraphBits);
            return (graphLastBits & (1 << innerIndex)) != 0;
        }
    }

    void graphReset();

    inline void graphSet(unsigned int i)  {
        DEBUG_ASSERT(0 <= i && i < numGraphBits);
        unsigned int outerIndex = i / wordLength;
        unsigned int innerIndex = i % wordLength;
        if (outerIndex < numMainGraphChars)
            graphMain[outerIndex] |= (1ULL << innerIndex);
        else
        {
            DEBUG_ASSERT(innerIndex < numLastGraphBits);
            graphLastBits |= (1ULL << innerIndex);
        }
    }
	
	inline void graphSetToOr(unsigned int i, bool val)  {
        DEBUG_ASSERT(0 <= i && i < numGraphBits);
        unsigned int outerIndex = i / wordLength;
        unsigned int innerIndex = i % wordLength;
        if (outerIndex < numMainGraphChars)
            graphMain[outerIndex] |= ((uint8_t)val << innerIndex);
        else
        {
            DEBUG_ASSERT(innerIndex < numLastGraphBits);
            graphLastBits |= ((uint8_t)val << innerIndex);
        }
    }
	
	[[nodiscard]] inline std::pair<std::array<uint8_t,numGraphBits + 1>,int> runLengthProfile() const;
};

#endif //POSET_OBJ_CORE_H