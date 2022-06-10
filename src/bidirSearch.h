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

#ifndef SORTINGLOWERBOUNDS_BIDIRSEARCH_H
#define SORTINGLOWERBOUNDS_BIDIRSEARCH_H

#include <atomic>
#include <string>

#include "posetObj.h"
#include "TimeProfile.h"
#include "state.h"
#include "oldGenMap.h"

class Search {

public:

    //static constexpr std::string_view  scratchFast = "/scratch/usfs/test.mmap";
    //static constexpr std::string_view  scratchMedium = "/scratch_medium/usfs/test2.mmap";
    std::string scratchFast;
    std::string scratchMedium;
    std::string bw_storage_path;
    // static constexpr uint64_t activePosetMemory = 150'000'000'000;
    // static constexpr uint64_t oldGenMemory = 100'000'000'000;
    uint64_t activePosetMemory = 100'000'000;
    uint64_t oldGenMemory = 100'000'000;

    TimeProfile profile;
    std::atomic<float> progress;
    std::vector<LayerState> layerState;
    std::vector<OldGenMap> oldGenMap;
    unsigned int forwardC;
    unsigned int backwardC;
    bool collided;
    bool do_fw_search;
    bool do_bw_search;
    bool reuse_bw;

    double effBandwidth;
    unsigned int fullLayers;

    double effBandwidth2;
    unsigned int effBand2Thr = MAXC;

    Search();

    void run();

    std::vector<std::string> getFwProfile();
    std::vector<std::string> getMapProfile();
};

#endif //SORTINGLOWERBOUNDS_BIDIRSEARCH_H
