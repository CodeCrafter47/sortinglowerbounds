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


#include "stats.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>

thread_local std::array<uint64_t, NUM_STATS> 		Stats::loc = {0};
std::array<uint64_t, NUM_STATS> 		Stats::glob = {};
std::array<uint64_t, NUM_STATS> 		Stats::globRecent = {};
thread_local std::array<AvMaxItem, NUM_AVMSTATS> 	Stats::locAvMax = {};
std::array<AvMaxItem, NUM_AVMSTATS> 				Stats::globAvMax = {};
std::array<AvMaxItem, NUM_AVMSTATS> 				Stats::globAvMaxRecent = {};

std::vector<std::string> Stats::detailed() {
    accumulate();
    std::stringstream outLine;
    std::vector<std::string> result;
    {
        std::lock_guard<std::mutex> lck(lock);
        outLine << std::left << std::setw(20) << "Stat name" << "  " << std::setw(15) << "Total" << "Recent";
        result.push_back(outLine.str());
        for (int i = 0; i < NUM_STATS ; i++){
            std::stringstream outLine;
            outLine << std::left << std::setw(20) << statTags[i].name << ": " << std::setw(15) << glob[i] << globRecent[i];
            result.push_back(outLine.str());
        }

        for (int i =0 ; i < AVMSTAT::NUM_AVMSTATS ; i++){
            std::stringstream outLine;
            outLine << std::left 	<< std::setw(20) << statTagsAvMax[i].name;
            outLine << "  max: " 	<< std::setw(8)  << globAvMax[i].max;
            //			outLine <<  "  sum: "	<< std::setw(11) << globAvMax[i].sum;
            outLine << "  num: " 	<< std::setw(12) << globAvMax[i].num ;
            outLine <<  "  avg: " 	<< std::setw(11) << (double)globAvMax[i].sum/(double)globAvMax[i].num;
            outLine <<  "  num >=" 	<< std::setw(9)  << statTagsAvMax[i].largeThreshold << ": " << globAvMax[i].numLarge;
            result.push_back(outLine.str());
            outLine.clear();
        }

    }
    return result;
}

std::mutex 											Stats::lock;