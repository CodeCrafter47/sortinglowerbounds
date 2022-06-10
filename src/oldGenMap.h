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


#ifndef SORTINGLOWERBOUNDS_OLDGENMAP_H
#define SORTINGLOWERBOUNDS_OLDGENMAP_H

#include <cstdint>
#include <boost/interprocess/managed_mapped_file.hpp>

#include "posetObj.h"

class OldGenMap {

    boost::interprocess::managed_mapped_file::segment_manager *const segment_manager;
    uint16_t *hashArray;
    PosetObj *posetArray;
    bool empty;

public:
    const size_t size;
    std::array<uint64_t, 8> profile;
    std::array<uint64_t, 8> profileStorage;

public:
    OldGenMap(boost::interprocess::managed_mapped_file::segment_manager *segment_manager, size_t size);
    ~OldGenMap();

    void insert(const AnnotatedPosetObj& poset);

    PosetObj* find(const AnnotatedPosetObj& poset);
};

#endif //SORTINGLOWERBOUNDS_OLDGENMAP_H
