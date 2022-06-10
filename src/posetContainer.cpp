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


#include "posetContainer.h"
#include "semiOfflineVector.h"

#include <cassert>
#include <mutex>

#include "posetObj.h"
#include "mmapAllocator.h"


static MmapAllocator alloc;

bool PosetContainerTemplate::useMmap = false;

PosetObj &PosetContainerTemplate::get(uint64_t index) const {

    assert(index <= this->num_elements);

    return listHeads[index / blockSize][index % blockSize];
}

uint64_t PosetContainerTemplate::insert(const PosetObj &poset) {

    if (listHeads.size() * blockSize == this->num_elements) {
        if (useMmap) {
            this->listHeads.push_back(alloc.requestMemory(blockSize));
        } else {
            this->listHeads.push_back(static_cast<PosetObj*>(malloc(sizeof(PosetObj) * blockSize)));
        }
    }

    size_t index = num_elements++;

    PosetObj* pointer = &listHeads[index / blockSize][index % blockSize];

    assert(pointer != nullptr);

    new(pointer) PosetObj(poset);

    return index;
}

std::array<uint64_t, 8> PosetContainerTemplate::countPosetsDetailed(bool unmarked) const {

    std::array<uint64_t, 8> result = { 0, 0, 0, 0, 0, 0, 0 ,0};
    for (int index = 0; index < this->num_elements; index++) {
        PosetObj &poset = get(index);
        if (unmarked || poset.isMarked()) {
            result[poset.GetStatus()]++;
        }
    }
    return result;
}

PosetContainerTemplate::~PosetContainerTemplate() {
    for (auto ptr: listHeads) {
        if (useMmap) {
            alloc.returnMemory(ptr, blockSize);
        } else {
            free(ptr);
        }
    }
    listHeads.clear();
    num_elements = 0;
}
