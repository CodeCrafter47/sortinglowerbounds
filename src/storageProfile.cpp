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


#include "storageProfile.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>

#include "sortableStatus.h"

StorageProfileFull StorageProfile::profile;

std::vector<std::string> StorageProfileFull::summary() {
    std::stringstream outLine;
    std::vector<std::string> result;
    uint64_t totalNum = 0;
    uint64_t countC;
    for (int c = 0; c <= NCT::C; c++) {
        std::stringstream outLine;
        countC = profiles[c].sum();
        totalNum += countC;
        outLine  << "c = ";
        outLine.width(2);
        outLine << c ;  //<< std::fixed  << std::setw(2) << std::setfill(' ')
        outLine  << ":  " << std::left << std::setw(11) << countC; //
        StorageProfileC& profC = profiles[c];
        outLine << "\t Unf:  " << std::left << std::setw(11)  << profC.data[SortableStatus::UNFINISHED];
        outLine << "\t YES:  " << std::left << std::setw(11) << profC.data[SortableStatus::YES];
        outLine << "\t NO: "   << std::left << std::setw(11) << profC.data[SortableStatus::NO];
        result.push_back(outLine.str());
        outLine.clear();
    }

    outLine << "Total number: " << totalNum;
    result.push_back(outLine.str());

    return result;
}

void StorageProfileFull::updateDiff(unsigned int c, std::array<uint64_t, 8> before, std::array<uint64_t, 8> after) {
    profiles[c].updateCountsDiff(before, after);
}

void StorageProfileFull::update(unsigned int c, std::array<uint64_t, 8> counts) {
    profiles[c].updateCounts(counts);
}

StorageProfileFull::StorageProfileFull() noexcept {
    profiles.fill(StorageProfileC());
}

size_t StorageProfileFull::countInRange(unsigned int begin, unsigned int end) {
    if (end > NCT::C) {
        end = NCT::C + 1;
    }
    size_t result = 0;
    for (unsigned int c = begin; c < end; c++) {
        result += profiles[c].sum();
    }
    return result;
}

std::vector<std::string> StorageProfile::summary() {
    return profile.summary();
}

void StorageProfile::updateDiff(unsigned int c, std::array<uint64_t, 8> before, std::array<uint64_t, 8> after) {
    profile.updateDiff(c, before, after);
}

void StorageProfile::update(unsigned int c, std::array<uint64_t, 8> counts) {
    profile.update(c, counts);
}

size_t StorageProfile::countInRange(unsigned int begin, unsigned int end) {
    return profile.countInRange(begin, end);
}

uint64_t StorageProfileC::sum() {
    uint64_t sum = 0;
    for (uint64_t i : data) {
        sum += i;
    }
    return sum;
}

void StorageProfileC::updateCountsDiff(std::array<uint64_t, 8> before, std::array<uint64_t, 8> after) {
    for (int i = 0; i < 8; i++) {
        data[i] += after[i] - before[i];
    }
}

void StorageProfileC::updateCounts(std::array<uint64_t, 8> counts) {
    data = counts;
}

StorageProfileC::StorageProfileC() : data() {
    data.fill(0);
}
