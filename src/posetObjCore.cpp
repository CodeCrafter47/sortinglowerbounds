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


#include "posetObjCore.h"

#include <cstring>
#include <cassert>

PosetObjCore::PosetObjCore() {
    graphReset();

    selfdualId = 1;
    uniqueGraph = 0;
    mark = 0;
    status = SortableStatus::UNFINISHED;
}

uint64_t PosetObjCore::hashFromGraph() const {
    constexpr int numUint64 = numMainGraphChars / 8;
    constexpr int numMoreChars = numMainGraphChars - 8*numUint64;
    uint64_t hash = 0;

    for(int i = 0; i < numUint64 ; i++){
        uint64_t dummy;
        std::memcpy(&dummy, ((char*)&graphMain) + 8*i, sizeof(uint64_t));

        hash ^= (hash << 17 ) ^ dummy ^ ((hash >> 5) & 0x4F0F0F0F0F0F0F0F );
    }

    for(int i = 0; i < numMoreChars ; i++){
        hash ^= (hash << 17 ) ^ (graphMain)[i + 8*numUint64] ^ ((hash >> 5) & 0x4F0F0F0F0F0F0F0F );
    }
    hash ^= ((uint64_t)graphLastBits <<47) + graphLastBits;
    hash *= 123456789;
    hash ^= (hash >> 50 ) ^ (hash >> 25 ) ^ ((hash >> 5) & 0x4F0F0F0F0F0F0F0F);

    return hash;
}

bool PosetObjCore::SameGraph(const PosetObjCore &other) const {
    for (int i = 0; i < numMainGraphChars; i++)
        if (graphMain[i] != other.graphMain[i])
            return false;
    return graphLastBits == other.graphLastBits;
}

void PosetObjCore::graphReset() {
    for (unsigned char & byte : graphMain)
        byte = 0;
    graphLastBits = 0;
}

std::pair<std::array<uint8_t, PosetObjCore::numGraphBits + 1>, int> PosetObjCore::runLengthProfile() const {

    std::array<uint8_t,numGraphBits + 1> result;
    for (int i = 0 ; i < numGraphBits + 1; i++)
        result[i] = 0;

    int curRunLength = 0;
    int weight = 0;
    for (int i = 0 ; i < numGraphBits ; i++){
        //		if(curRunLength > numGraphBits){
        //			std::cout << i << " curRunLength: " << curRunLength<< std::endl;
        //
        //		}
        if(!graphGet(i))
            curRunLength++;
        else{
            result[curRunLength]++;
            curRunLength = 0;
            weight++;
        }
        assert(curRunLength < numGraphBits + 1);
    }
    result[curRunLength]++;
    assert(weight <= MAXENDC);
    return std::pair(result,weight);
}
