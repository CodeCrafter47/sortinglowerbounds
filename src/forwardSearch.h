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

#ifndef SORTINGLOWERBOUNDS_FORWARDSEARCH_H
#define SORTINGLOWERBOUNDS_FORWARDSEARCH_H

#include <atomic>
#include <functional>

#include "config.h"
#include "posetObj.h"
#include "posetInfo.h"
#include "state.h"
#include "oldGenMap.h"

class PosetStorage;
class TimeProfile;
class PosetHandle;
class PosetMap;
class PosetMapExt;
template<class T>
class SemiOfflineVector;
class PosetEntry;


void doForwardStep(SemiOfflineVector<AnnotatedPosetObj> &posetList,
                   SemiOfflineVector<uint64_t> &edgeList,
                   LayerState &parentState,
                   LayerState &childState,
                   unsigned int &parentC,
                   LinExtT childLayerCompleteAbove,
                   PosetMapExt &childMap,
                   PosetMap &childMapBW,
                   OldGenMap &childMapOld,
                   OldGenMap &parentMapOld,
                   LinExtT limit,
                   std::atomic<float> &progress,
                   TimeProfile &profile,
                   std::vector<uint64_t>& tempVec,
                   uint64_t childPosetLimit,
                   uint64_t childEdgeLimit);

void createInitialPosetFW(SemiOfflineVector<AnnotatedPosetObj>& posetList,
                          LayerState &parentState);

#endif //SORTINGLOWERBOUNDS_FORWARDSEARCH_H
