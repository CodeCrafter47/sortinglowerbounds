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


#include <thread>
#include <parallel/algorithm>
#include <fstream>

#include "forwardSearch.h"
#include "expandedPoset.h"
#include "posetMap.h"
#include "storageProfile.h"
#include "linExtCalculator.h"
#include "searchParams.h"
#include "stats.h"
#include "utils.h"
#include "eventLog.h"
#include "TimeProfile.h"
#include "state.h"
#include "semiOfflineVector.h"

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
                   std::vector<uint64_t> &tempVec,
                   const uint64_t childPosetLimit,
                   const uint64_t childEdgeLimit) {

    if (parentState.phase >= 2) {

        if (parentState.phase == 2) {
            Stats::inc(STAT::NPhase2);
        } else {
            assert(parentState.phase == 3);
            Stats::inc(STAT::NPhase3);
        }

        std::atomic<size_t> parentIndex;
        std::atomic<bool> hasUnfinished = false;
        auto processThread = [&]() {

            std::vector<uint64_t> localEdgeList;

            auto processPoset = [&](AnnotatedPosetObj &entry) {

                assert(entry.GetStatus() == SortableStatus::UNFINISHED);

                localEdgeList.clear();
                auto elIndex = entry.elIndex;
                assert(elIndex >= parentState.elBegin);
                auto elSize = edgeList[elIndex++];
                assert(elSize % 2 == 0);

                bool unsortable = true;
                for (int i = 0; i < elSize; i += 2) {
                    auto idxFirst = edgeList[elIndex + i];
                    auto idxSecond = edgeList[elIndex + i + 1];
                    assert(parentState.phase == 2 || idxFirst == idxSecond);
                    auto &first = posetList[idxFirst];
                    auto &second = posetList[idxSecond];
                    auto firstSortable = first.GetStatus() == SortableStatus::YES;
                    auto secondSortable = second.GetStatus() == SortableStatus::YES;
                    if (firstSortable && secondSortable) {
                        entry.SetSortable();
                        return;
                    } else if (first.GetStatus() == SortableStatus::NO
                               || second.GetStatus() == SortableStatus::NO) {
                        // do nothing
                    } else if (firstSortable) {
                        localEdgeList.push_back(idxSecond);
                        localEdgeList.push_back(idxSecond);
                        unsortable = false;
                    } else if (secondSortable) {
                        localEdgeList.push_back(idxFirst);
                        localEdgeList.push_back(idxFirst);
                        unsortable = false;
                    } else {
                        assert(false);
                    }
                }

                if (unsortable) {
                    entry.SetUnsortable();
                    return;
                }

                // update edge list
                auto newElSize = localEdgeList.size();
                assert(newElSize <= elSize);
                edgeList[elIndex - 1] = newElSize;
                for (size_t i = 0; i < newElSize; i++) {
                    edgeList[elIndex + i] = localEdgeList[i];
                    Stats::inc(STAT::NMarkSecond);
                    posetList[localEdgeList[i]].setMark(true);
                }
                hasUnfinished = true;
                Stats::addVal<AVMSTAT::ELSizePhase2>(newElSize / 2);
            };

            NCT::initThread();

            while (true) {

                // grab a batch of posets to process
                size_t beginIndex = parentIndex.fetch_add(SearchParams::batchSize);
                size_t endIndex = std::min(parentState.parentsSliceEnd, beginIndex + SearchParams::batchSize);

                // terminate if batch empty
                if (endIndex <= beginIndex) {
                    break;
                }

                // process posets in batch
                for (size_t index = beginIndex; index < endIndex; index++) {
                    // get poset
                    auto &parent = posetList[edgeList[index]];
                    if (!parent.isMarked() || parent.GetStatus() != SortableStatus::UNFINISHED) {
                        continue;
                    }
                    // search
                    processPoset(parent);
                    assert(parent.GetStatus() != SortableStatus::UNFINISHED || hasUnfinished);
                }
            }

            Stats::accumulate();
        };

        // io
        profile.section(Section::FW_IO);
        posetList.ensureOnlineFrom(parentState.posetListBegin);
        edgeList.ensureOnlineFrom(parentState.parentsBegin);

        profile.section(Section::FW_PHASE2);
        EventLog::write(true, "Processing layer c=" + std::to_string(parentC) +
                              " phase " + std::to_string(parentState.phase));
        parentIndex = parentState.parentsSliceBegin;
        if ((parentState.parentsSliceEnd - parentState.parentsSliceBegin) > SearchParams::batchSize * 4) {
            std::vector<std::thread> threads;
            for (unsigned int i = 0; i < NCT::num_threads; i++) {
                threads.emplace_back(processThread);
            }
            for (auto &thread: threads) {
                thread.join();
            }
        } else {
            processThread();
        }

        if (parentState.phase == 3) {
            assert(hasUnfinished == false);
        }

        if (hasUnfinished) {
            parentState.phase = 3;
            childState.posetListBegin = parentState.posetListEnd;
            assert(childState.posetListEnd == posetList.size());
            childState.phase = 0;
            parentC += 1;
            return;
        } else {
            // remove unneeded data
            profile.section(Section::FW_IO);
            posetList.resize(parentState.posetListEnd);
            edgeList.resize(parentState.elBegin);

            if (parentState.parentsSliceEnd == parentState.parentsEnd) {

                // store parents in offline hash map!!!
                profile.section(Section::FW_OLDGEN);
                tempVec.clear();
                for (auto i = parentState.parentsBegin; i < parentState.parentsEnd; i++) {
                    auto &poset = posetList[edgeList[i]];
                    if (poset.isMarked() && poset.GetStatus() != SortableStatus::UNFINISHED) {
                        tempVec.push_back(edgeList[i]);
                        // unmark here
                        poset.setMark(false);
                    }
                }
                auto size = parentMapOld.size;
                __gnu_parallel::sort(tempVec.begin(), tempVec.end(), [&](uint64_t a, uint64_t b) {
                    return ((posetList[a].GetHash() * MULT1) % size) < ((posetList[b].GetHash() * MULT1) % size);
                });
                if (false && tempVec.size() > 1000) {
                    std::vector<std::thread> threads{};
                    NCT::initThread();
                    for (unsigned int i = 0; i < NCT::num_threads; i++) {
                        threads.emplace_back([&,i] () {
                            NCT::initThread();
                            int begin = tempVec.size() * i / NCT::num_threads;
                            int end = tempVec.size() * (i + 1) / NCT::num_threads;
                            for (int id = begin; id < end; id++) {
                                if (id == 0 || ((posetList[tempVec[id - 1]].GetHash() * MULT1) % size) != ((posetList[tempVec[id]].GetHash() * MULT1) % size)) {
                                    parentMapOld.insert(posetList[tempVec[id]]);
                                }
                            }
                        });
                    }
                    for (auto &thread: threads) {
                        thread.join();
                    }
                } else {
                    for (auto id: tempVec) {
                        parentMapOld.insert(posetList[id]);
                    }
                }
                edgeList.resize(parentState.parentsBegin);

                if (parentC > 0) {
                    parentC -= 1;
                }
                return;
            } else {
                parentState.phase = 1;
            }
        }
    }

    if (parentState.phase == 0) {
        profile.section(Section::FW_PHASE1);
        tempVec.clear();
        for (auto i = parentState.posetListBegin; i < parentState.posetListEnd; i++) {
            if (posetList[i].isMarked()) {
                tempVec.push_back(i);
            }
        }

        // sort
        __gnu_parallel::sort(tempVec.begin(), tempVec.end(), [&](uint64_t a, uint64_t b) {
            return posetList[a].linExt < posetList[b].linExt;
        });

        profile.section(Section::FW_IO);
        edgeList.ensureOnlineAvailable(tempVec.size());

        profile.section(Section::FW_PHASE1);
        parentState.parentsBegin = edgeList.size();
        edgeList.insert(tempVec.cbegin(), tempVec.cend());
        parentState.parentsEnd = edgeList.size();
        parentState.parentsSliceBegin = parentState.parentsBegin;
        parentState.parentsSliceEnd = parentState.parentsBegin;
        parentState.phase = 1;
    }

    if (parentState.phase == 1) {

        Stats::inc(STAT::NPhase1);

        std::atomic<size_t> parentIndex;
        unsigned int pOffset;
        unsigned int pMax;

        auto processFWThread = [&, childLayerCompleteAbove, parentC]() {
            NCT::initThread();

            enum ComparisonStatus {
                SORTABLE,
                UNSORTABLE,
                INDETERMINATE
            };

            struct ComparisonTuple {
                const int k1, k2;
                const LinExtT lin1, lin2;
                const bool singletonComp;

                ComparisonTuple(int kk1, int kk2, LinExtT l1, LinExtT l2, bool singleton) : k1(kk1), k2(kk2), lin1(l1), lin2(l2), singletonComp(singleton) {}
            };

            LinearExtensionCalculator linExtCalculator{NCT::N, NCT::C};
            std::vector<ComparisonTuple> comparisonVector;
            std::vector<uint64_t> localEdgeList;

            auto checkChild = [&, childLayerCompleteAbove, parentC](AnnotatedPosetObj &child, LinExtT linExt) {

                if (linExt >= childLayerCompleteAbove) {
                    Stats::inc(STAT::NChildMapBWFind);
                    auto result = childMapBW.find(child);
                    if (result == nullptr) {
                        return ComparisonStatus::UNSORTABLE;
                    } else if (result->GetStatus() == SortableStatus::NO) {
                        Stats::inc(STAT::NChildMapBWFindNo);
                        return ComparisonStatus::UNSORTABLE;
                    } else if (result->GetStatus() == SortableStatus::YES) {
                        Stats::inc(STAT::NChildMapBWFindYes);
                        return ComparisonStatus::SORTABLE;
                    } else {
                        assert(result->GetStatus() == SortableStatus::UNFINISHED);
                        Stats::inc(STAT::NChildMapBWFindUnf);
                    }
                }
                Stats::inc(STAT::NChildMapOldFind);
                auto result = childMapOld.find(child);
                if (result != nullptr) {
                    if (result->GetStatus() == SortableStatus::NO) {
                        Stats::inc(STAT::NChildMapOldFindNo);
                        return ComparisonStatus::UNSORTABLE;
                    } else if (result->GetStatus() == SortableStatus::YES) {
                        Stats::inc(STAT::NChildMapOldFindYes);
                        return ComparisonStatus::SORTABLE;
                    }
                    assert(false);
                }
                return ComparisonStatus::INDETERMINATE;
            };

            auto addComparisonIfFeasible = [&](const AnnotatedPosetObj &parentHandle, unsigned int j, unsigned int k, LinExtT limit,
                                               bool singletonComp = false) {
                LinExtT p_1 = linExtCalculator.linExtTable[j][k];
                LinExtT p_2 = linExtCalculator.linExtTable[k][j];
                assert(parentC == 0 || p_1 <= 2 * limit);
                assert(parentC == 0 || p_2 <= 2 * limit);

                if (p_1 == 0 || p_2 == 0) {
                    //comparing already related pair
                    return;
                }
                if (p_1 > limit || p_2 > limit) {
                    //not sortable in remaining comparisons
                    return;
                }

                // order children
                unsigned int k1, k2;
                LinExtT lin1, lin2;
                if (p_1 >= p_2) {
                    k1 = j;
                    k2 = k;
                    lin1 = p_1;
                    lin2 = p_2;
                } else {
                    k1 = k;
                    k2 = j;
                    lin1 = p_2;
                    lin2 = p_1;
                }

                if (singletonComp) {
                    assert(lin1 == lin2 && k2 == k1 + 1);
                }

                comparisonVector.emplace_back(k1, k2, lin1, lin2, singletonComp);
            };

            auto enumerateComparisons = [&](AnnotatedPosetObj &poset, LinExtT limit) {
                unsigned int n = NCT::N;

                auto currentNumSingletons = poset.GetnumSingletons();
                auto currentNumPairs = poset.GetNumPairs();

                if (currentNumPairs == 2) {
                    auto startPairs = poset.GetFirstInPair();
                    assert(poset.GetFirstSingleton() - startPairs == 4);

                    //compare all binom(4)(2) = 6 possibilities -- this can be reduced to 3 possibilities if there are guarantees that pairs are successive
                    addComparisonIfFeasible(poset, startPairs, startPairs + 1, limit);
                    addComparisonIfFeasible(poset, startPairs, startPairs + 2, limit);
                    addComparisonIfFeasible(poset, startPairs, startPairs + 3, limit);
                    addComparisonIfFeasible(poset, startPairs + 1, startPairs + 2, limit);
                    addComparisonIfFeasible(poset, startPairs + 1, startPairs + 3, limit);
                    addComparisonIfFeasible(poset, startPairs + 2, startPairs + 3, limit);
                } else {
                    assert(currentNumPairs <= 1);

                    //compare two singletons if there are
                    if (currentNumSingletons >= 2) {
                        addComparisonIfFeasible(poset, poset.GetFirstSingleton(), poset.GetFirstSingleton() + 1, limit, true);
                    }
                    if (currentNumPairs == 1) {
                        //compare Pair with singleton
                        if (currentNumSingletons >= 1) {
                            addComparisonIfFeasible(poset, poset.GetFirstInPair(), poset.GetFirstSingleton(), limit);
                            addComparisonIfFeasible(poset, poset.GetFirstInPair() + 1, poset.GetFirstSingleton(), limit);

                        }
                        //compare pair with elements before it
                        for (int j = 0; j < poset.GetFirstInPair(); j++) {
                            addComparisonIfFeasible(poset, j, poset.GetFirstInPair(), limit);
                            addComparisonIfFeasible(poset, j, poset.GetFirstInPair() + 1, limit);
                        }
                    } else {
                        assert(currentNumPairs == 0);
                        unsigned int endNode = std::min(n - currentNumSingletons + 1, n);
                        // compare all possibilities of other elements (involving at most one singleton)
                        for (int j = 0; j < endNode - 1; j++) {
                            for (int k = j + 1; k < endNode; k++) {
                                addComparisonIfFeasible(poset, j, k, limit);
                            }
                        }
                    }
                }
            };

            auto createChildEntrySingleton = [&](const AnnotatedPosetObj &child) {
                Stats::inc(STAT::NCompOneChild);
                // find / insert
                auto id = childMap.findAndInsert(child);
                // edge list
                localEdgeList.push_back(id);
                localEdgeList.push_back(id);
            };

            auto createChildEntry = [&](AnnotatedPosetObj &first, AnnotatedPosetObj second) {
                Stats::inc(STAT::NCompTwoChildren);
                // find / insert
                auto idFirst = childMap.findAndInsert(first);
                auto idSecond = childMap.findAndInsert(second);
                // edge list
                localEdgeList.push_back(idFirst);
                localEdgeList.push_back(idSecond);
            };

            auto exploreComparison = [&, parentC](AnnotatedPosetObj &parentPoset, const ComparisonTuple &comparison) {
                int k1 = comparison.k1;
                int k2 = comparison.k2;
                LinExtT lin1 = comparison.lin1;
                LinExtT lin2 = comparison.lin2;

                bool firstSortable = isEasilySortableLinExt(remainingComparisonsChild(parentC), lin1);
                bool secondSortable = isEasilySortableLinExt(remainingComparisonsChild(parentC), lin2);
                if (firstSortable && secondSortable) {
                    return ComparisonStatus::SORTABLE;
                }

                AnnotatedPosetObj firstChild;
                PosetHandle handleParent{parentPoset, PosetInfo(parentPoset)};
                if (!firstSortable) {
                    ExpandedPosetChild new_poset_p1{handleParent, lin1, k1, k2};
                    firstSortable = new_poset_p1.isEasilySortableUnrelatedPairs(remainingComparisonsChild(parentC));

                    if (comparison.singletonComp || isEasilySortableLinExt(remainingComparisonsChild(parentC), lin2)) {
                        // comparing two singletons, the two child posets are isomorphic, only need to check one; or
                        // second child is obviously sortable, only check first
                        if (firstSortable) {
                            return ComparisonStatus::SORTABLE;
                        }
                        firstChild = new_poset_p1.getHandle();
                        auto status = checkChild(firstChild, lin1);
                        if (status != ComparisonStatus::INDETERMINATE) {
                            return status;
                        }
                        createChildEntrySingleton(new_poset_p1.getHandle());
                        return ComparisonStatus::INDETERMINATE;
                    }

                    if (firstSortable && secondSortable) {
                        return ComparisonStatus::SORTABLE;
                    }

                    if (!firstSortable) {
                        firstChild = new_poset_p1.getHandle();
                        auto status = checkChild(firstChild, lin1);
                        if (status == ComparisonStatus::UNSORTABLE) {
                            return status;
                        } else if (status == ComparisonStatus::SORTABLE) {
                            firstSortable = true;
                        }
                    }

                    if (firstSortable && secondSortable) {
                        return ComparisonStatus::SORTABLE;
                    }
                }

                AnnotatedPosetObj secondChild;
                if (!secondSortable) {
                    ExpandedPosetChild new_poset_p2{handleParent, lin2, k2, k1};
                    secondSortable = new_poset_p2.isEasilySortableUnrelatedPairs(remainingComparisonsChild(parentC));

                    if (firstSortable && secondSortable) {
                        return ComparisonStatus::SORTABLE;
                    }

                    if (!secondSortable) {
                        secondChild = new_poset_p2.getHandle();
                        auto status = checkChild(secondChild, lin2);
                        if (status == ComparisonStatus::UNSORTABLE) {
                            return status;
                        } else if (status == ComparisonStatus::SORTABLE) {
                            secondSortable = true;
                        }
                    }
                }

                if (!firstSortable && !secondSortable) {
                    createChildEntry(firstChild, secondChild);
                } else if (!firstSortable) {
                    createChildEntrySingleton(firstChild);
                } else if (!secondSortable) {
                    createChildEntrySingleton(secondChild);
                } else {
                    return ComparisonStatus::SORTABLE;
                }
                return ComparisonStatus::INDETERMINATE;
            };

            auto processPoset = [&, parentC, limit](AnnotatedPosetObj &poset) {

                assert(poset.GetStatus() == SortableStatus::UNFINISHED);

                comparisonVector.clear();
                localEdgeList.clear();
                localEdgeList.push_back(0);

                PosetHandle handle{poset, PosetInfo(poset)};
                LinExtT linExt = linExtCalculator.calculateLinExtensionsSingleton(handle, parentC, true, false);

                if (linExt > limit * 2) {
                    Stats::inc(STAT::NParentUnsortableBWLimit);
                    poset.SetUnsortable();
                    return;
                }

                enumerateComparisons(poset, limit);

                bool unsortable = true;
                for (const ComparisonTuple &item: comparisonVector) {
                    ComparisonStatus status = exploreComparison(poset, item);
                    if (status == ComparisonStatus::SORTABLE) {
                        poset.SetSortable();
                        poset.elIndex = -1;
                        return;
                    } else if (status == ComparisonStatus::INDETERMINATE) {
                        unsortable = false;
                    }
                }

                if (unsortable) {
                    poset.SetUnsortable();
                    return;
                }

                // append to edge list
                auto elSize = localEdgeList.size() - 1;
                localEdgeList[0] = elSize;
                auto index = edgeList.insert(localEdgeList.cbegin(), localEdgeList.cend());
                assert(index >= parentState.elBegin);
                poset.elIndex = index;
                Stats::addVal<AVMSTAT::ELSizePhase1>(elSize / 2);
            };

            while ((edgeList.size() - parentState.elBegin) < childEdgeLimit
                   && (posetList.size() - parentState.posetListBegin) < childPosetLimit) {

                // grab a batch of posets to process
                size_t beginIndex = parentIndex.fetch_add(SearchParams::batchSize);
                size_t endIndex = std::min((size_t) parentState.parentsEnd, beginIndex + SearchParams::batchSize);

                // terminate if batch empty
                if (endIndex <= beginIndex) {
                    break;
                }

                // update progress
                progress = static_cast<float>((parentIndex - parentState.parentsBegin)) / static_cast<float>(pMax);

                // process posets in batch
                for (size_t index = beginIndex; index < endIndex; index++) {
                    // get poset
                    auto entryIdx = edgeList[index];
                    auto &parent = posetList[entryIdx];
                    if (!parent.isMarked() || parent.GetStatus() != SortableStatus::UNFINISHED) {
                        continue;
                    }
                    // search
                    processPoset(parent);
                    assert(parent.GetStatus() != SortableStatus::UNFINISHED || parent.elIndex != 0 || parentC == 0);
                }
            }

            Stats::accumulate();
        };

        auto childListBegin = posetList.size();
        parentState.elBegin = edgeList.size();

        parentState.parentsSliceBegin = parentState.parentsSliceEnd;

        // io
        profile.section(Section::FW_IO);
        posetList.ensureOnlineFrom(parentState.posetListBegin);
        posetList.ensureOnlineAvailable(childPosetLimit + 50000); // magic number
        edgeList.ensureOnlineFrom(parentState.parentsSliceBegin);
        edgeList.ensureOnlineAvailable(childEdgeLimit + 100000); // magic number

        // process
        profile.section(Section::FW_PHASE1);
        pOffset = parentState.parentsSliceBegin - parentState.parentsBegin;
        pMax = parentState.parentsEnd - parentState.parentsBegin;
        EventLog::write(true, "Processing layer c=" + std::to_string(parentC) +
                              " phase 1");
        parentIndex = parentState.parentsSliceBegin;
        if ((parentState.parentsEnd - parentState.parentsSliceBegin) > SearchParams::batchSize * 4) {
            std::vector<std::thread> threads;
            for (unsigned int i = 0; i < NCT::num_threads; i++) {
                threads.emplace_back(processFWThread);
            }
            for (auto &thread: threads) {
                thread.join();
            }
        } else {
            processFWThread();
        }
        parentState.parentsSliceEnd = std::min(static_cast<uint64_t>(parentIndex), parentState.parentsEnd);

        childMap.clear();

        if (parentState.elBegin == edgeList.size()) {
            posetList.resize(parentState.posetListEnd);
            edgeList.resize(parentState.parentsBegin);
            // move up
            if (parentC > 0) {
                parentC -= 1;
            }
            return;
        }

        // mark posets
        for (auto i = parentState.parentsSliceBegin; i < parentState.parentsSliceEnd; i++) {
            auto &poset = posetList[edgeList[i]];
            if (poset.isMarked() && poset.GetStatus() == SortableStatus::UNFINISHED) {
                auto elIndex = poset.elIndex;
                auto elSize = edgeList[elIndex];
                for (int index = 1; index <= elSize; index += 2) {
                    auto idFirst = edgeList[elIndex + index];
                    auto idSecond = edgeList[elIndex + index + 1];
                    // mark first, unless second is marked
                    if (!posetList[idSecond].isMarked()) {
                        if (!posetList[idFirst].isMarked()) {
                            Stats::inc(STAT::NMarkFirst);
                            posetList[idFirst].setMark(true);
                        }
                    }
                }
            }
        }

        parentState.phase = 2;
        childState.posetListBegin = childListBegin;
        childState.posetListEnd = posetList.size();
        childState.phase = 0;
        parentC += 1;
        return;
    }
}

void createInitialPosetFW(SemiOfflineVector<AnnotatedPosetObj> &posetList,
                          LayerState &parentState) {
    PosetObj posetObj;
    posetObj.setMark(true);
    PosetInfo info = PosetInfo::fromPoset(posetObj);
    posetList.insert(AnnotatedPosetObj(posetObj, PosetInfoFull(info, posetObj.computeHash()), factorial(NCT::N)));
    parentState.posetListBegin = 0;
    parentState.posetListEnd = 1;
    parentState.phase = 0;
}