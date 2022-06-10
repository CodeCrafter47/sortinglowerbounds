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

#ifndef PosetContainerTemplate_H
#define PosetContainerTemplate_H

#include <vector>

#include "posetPointer.h"
#include "posetHandle.h"
#include "posetObj.h"

class PosetContainerTemplate {

    static constexpr size_t blockSize = 1 << 17;

	unsigned int num_elements;

    std::vector<PosetObj*> listHeads;

public:

    static bool useMmap;

    PosetContainerTemplate() {
        this->num_elements = 0;
    }

    PosetContainerTemplate(const PosetContainerTemplate&) = delete;
    PosetContainerTemplate(PosetContainerTemplate && other ) noexcept = default;

    PosetContainerTemplate& operator= (const PosetContainerTemplate&) = delete;
    PosetContainerTemplate& operator= (PosetContainerTemplate&&) noexcept = default;

    ~PosetContainerTemplate();

    [[nodiscard]] PosetObj& get(uint64_t index) const;

    uint64_t insert( const PosetObj & poset);

    [[nodiscard]] std::array<uint64_t, 8> countPosetsDetailed(bool unmarked) const;

    [[nodiscard]] uint64_t size() const {
        return num_elements;
    }
};

class PosetContainer {

};

#endif