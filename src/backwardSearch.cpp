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

#include <tuple>
#include <cassert>
#include <thread>

#include "backwardSearch.h"
#include "posetHandle.h"
#include "posetObj.h"
#include "niceGraph.h"
#include "expandedPoset.h"
#include "posetMap.h"
#include "storeAndLoad.h"
#include "stats.h"
#include "storageProfile.h"
#include "TimeProfile.h"
#include "linExtCalculator.h"
#include "searchParams.h"

namespace {

    class BackwardSearch {

    private:
        typedef std::pair<uint8_t, uint8_t> Edge;

        PosetMap &parentMap;
        unsigned int parentC;
        PosetMap &childMap;
        LinExtT limitChildren;
        LinExtT limitParents;
        LinExtT linExtFirstChild;
        LinearExtensionCalculator linExtCalc;
        bool computeLinExt;

        uint64_t pred_count;
        uint64_t pot_pred_count;

    public:
        BackwardSearch(PosetMap &parentMap, unsigned int parentC, PosetMap &childMap, LinExtT limitChildren, LinExtT limitParent);

        void processPoset(PosetHandle &poset);

    private:
        void checkAndInsertParent(const AdjacencyMatrix &parentMat, const PosetInfo &childInfo, int k1, int k2, LinExtT linExtSecondChild, SortableStatus status);

        void exploreTransEdges(const AdjacencyMatrix &adjMat, const PosetInfo &info, int k1, int k2, Edge *transEdges, int teFirst, int teLast,
                               SortableStatus childStatus);

        void exploreComparison(PosetHandle &poset, const PosetInfo &info, int k1, int k2);

        SortableStatus checkReverseEdgeSortable(const AdjacencyMatrix &adjMat, const PosetInfo &info, int k1, int k2, LinExtT &linExtOut);
    };

    /**
     * Check whether v is a singleton in the poset represented by adjMat.
     */
    bool isSingleton(const AdjacencyMatrix &adjMat, int v) {
        for (int i = 0; i < adjMat.size(); i++) {
            if (i != v && (adjMat.get(i, v) || adjMat.get(v, i))) {
                return false;
            }
        }
        return true;
    }

    /**
     * Check whether v is part of a pair in the poset represented by adjMat.
     *
     * If it is part of a pair, then first and second will be set, such that there is an edge first -> second and one of them is v.
     */
    bool checkPair(const AdjacencyMatrix &adjMat, int v, uint8_t &first, uint8_t &second) {
        int other = -1;
        for (int i = 0; i < adjMat.size(); i++) {
            if (i != v && (adjMat.get(i, v) || adjMat.get(v, i))) {
                assert(!(adjMat.get(i, v) && adjMat.get(v, i)));
                other = i;
                break;
            }
        }
        if (other == -1) {
            return false;
        }
        for (int i = other + 1; i < adjMat.size(); i++) {
            if (i != v && (adjMat.get(i, v) || adjMat.get(v, i))) {
                return false;
            }
        }
        for (int i = 0; i < adjMat.size(); i++) {
            if (i != v && i != other && (adjMat.get(i, other) || adjMat.get(other, i))) {
                return false;
            }
        }
        if (adjMat.get(v, other)) {
            first = v;
            second = other;
        } else {
            assert(adjMat.get(other, v));
            first = other;
            second = v;
        }
        return true;
    }

    /**
     * Check whether the reverse edge poset obtained from the potential predecessor adjMat by adding the edge k2 -> k1 is sortable using at most parentC - 1
     * comparisons.
     *
     * @return SortableStatus::NO if not sortable, SortableStatus::YES if sortable, and SortableStatus::UNFINISHED if it cannot be determined whether the
     * reverse edge poset is sortable, because there is only partial information available on the child layer.
     */
    SortableStatus BackwardSearch::checkReverseEdgeSortable(const AdjacencyMatrix &adjMat, const PosetInfo &info, int k1, int k2, LinExtT &linExtOut) {
        pot_pred_count += 1;

        AdjacencyMatrix mat = adjMat;
        mat.set(k2, k1);
		
		
		 if (!computeLinExt && adjMat.edgeCount() > parentC) {
            return SortableStatus::UNFINISHED;
        }

        ExpandedPosetChild revEdgePoset{mat, info, 0, k2, k1};
        auto handle = revEdgePoset.getHandle();
        PosetObj *result = childMap.find(handle);
        if (computeLinExt) {
            PosetHandle handle2{handle, PosetInfo(handle)};
            linExtOut = linExtCalc.calculateLinExtensionsSingleton(handle2, parentC + 1, false, true);
            if (linExtOut > (LinExtT(1) << (NCT::C - parentC - 1))) {
                assert(result == nullptr);
                return SortableStatus::NO;
            }
        }

        if (adjMat.edgeCount() > parentC) {
            return SortableStatus::UNFINISHED;
        }

        if (result != nullptr) {
            return result->GetStatus();
        } else {
            if (computeLinExt && linExtOut < limitChildren) {
                return SortableStatus::UNFINISHED;
            } else {
                return SortableStatus::NO;
            }
        }
    }

    void BackwardSearch::checkAndInsertParent(const AdjacencyMatrix &parentMat, const PosetInfo &childInfo, int k1, int k2, LinExtT linExtSecondChild, SortableStatus status) {

        if (parentMat.edgeCount() > parentC) {
            return;
        }

        if (computeLinExt && (linExtFirstChild + linExtSecondChild) < limitParents) {
            return;
        }

        unsigned int singletons = childInfo.GetnumSingletons();
        unsigned int pairs = childInfo.GetNumPairs();
        AdjacencyMatrix reorderedMat = parentMat;
        if (k1 >= childInfo.GetFirstInPair()) {
            assert(k1 == childInfo.GetFirstSingleton() - 2);
            assert(k2 == childInfo.GetFirstSingleton() - 1);
            pairs -= 1;
            singletons += 2;
        } else {
            assert(pairs == 0);
            // look for new singletons and pairs and move them to the end
            uint8_t verticesToMove[6];
            int idx = 0;
            if (checkPair(parentMat, k1, verticesToMove[idx], verticesToMove[idx + 1])) {
                idx += 2;
                pairs += 1;
            }
            if (checkPair(parentMat, k2, verticesToMove[idx], verticesToMove[idx + 1])) {
                idx += 2;
                pairs += 1;
            }
            if (isSingleton(parentMat, k1)) {
                verticesToMove[idx++] = k1;
                singletons += 1;
            }
            if (isSingleton(parentMat, k2)) {
                verticesToMove[idx++] = k2;
                singletons += 1;
            }
            // create reordered matrix
            if (idx != 0) {
                VertexList permutation{};
                int max_n = childInfo.GetReducedN();
                for (int i = 0; i < max_n; i++) {
                    if (std::find(&verticesToMove[0], &verticesToMove[idx], i) == &verticesToMove[idx]) {
                        permutation.add(i);
                    }
                }
                for (int i = 0; i < idx; i++) {
                    permutation.add(verticesToMove[i]);
                }
                reorderedMat.reorder(permutation);
            }
        }
        PosetInfo parentInfo{singletons, pairs};

        ExpandedPosetChild expandedPoset{reorderedMat, parentInfo, linExtFirstChild + linExtSecondChild};
        auto handle = expandedPoset.getHandle();

        if (status == SortableStatus::YES) {
            handle.SetSortable();
        }
        if (!childMap.find(handle)) {
            pred_count += 1;
            parentMap.findAndInsert(handle);
        }
    }

    typedef std::pair<uint8_t, uint8_t> Edge;

    void BackwardSearch::exploreTransEdges(const AdjacencyMatrix &adjMat, const PosetInfo &info, int k1, int k2, Edge *transEdges, int teFirst, int teLast,
                                           SortableStatus childStatus) {
        if (teFirst == teLast) {
            return;
        }

        // check if we can remove enough edges such that the obtained predecessor would be stored
        if (adjMat.edgeCount() - (teLast - teFirst) > parentC) {
            Stats::inc(STAT::NPredLimitEdgeCount);
            return;
        }

        // try with first trans edge
        std::pair<uint8_t, uint8_t> edge = transEdges[teFirst++];
        exploreTransEdges(adjMat, info, k1, k2, transEdges, teFirst, teLast, childStatus);

        // try without first trans edge
        int j1 = edge.first;
        int j2 = edge.second;
        AdjacencyMatrix reducedParent = adjMat;
        reducedParent.deleteEdge(j1, j2);
        AdjacencyMatrix transClosure = reducedParent;
        transClosure.TransitiveClosure();
        // gather transitive edges to keep
        for (int i = 0; i < j1; i++) {
            if (reducedParent.get(i, j1) && !transClosure.get(i, j2)) {
                reducedParent.set(i, j2);
                transEdges[teLast++] = std::make_pair(i, j2);
            }
        }
        for (int i = j2 + 1; i < NCT::N; i++) {
            if (reducedParent.get(j2, i) && !transClosure.get(j1, i)) {
                reducedParent.set(j1, i);
                transEdges[teLast++] = std::make_pair(j1, i);
            }
        }
        LinExtT linExtRevEdge;
        SortableStatus status;
        if ((teLast - teFirst) == 1 && reducedParent.edgeCount() > parentC) {
            status = SortableStatus::UNFINISHED;
        } else {
            status = checkReverseEdgeSortable(reducedParent, info, k1, k2, linExtRevEdge);
        }
        if (SortableStatus::NO != status) {
            checkAndInsertParent(reducedParent, info, k1, k2, linExtRevEdge, status == SortableStatus::UNFINISHED ? status : childStatus);
            exploreTransEdges(reducedParent, info, k1, k2, transEdges, teFirst, teLast, childStatus);
        }
    }

    void BackwardSearch::exploreComparison(PosetHandle &poset, const PosetInfo &info, int k1, int k2) {
        AdjacencyMatrix adjMat(NCT::N);
        poset->GetAdMatrix(adjMat);

        // remove edge
        adjMat.deleteEdge(k1, k2);

        AdjacencyMatrix transClosure = adjMat;
        transClosure.TransitiveClosure();

        // gather transitive edges to keep
        AdjacencyMatrix parent = adjMat;
        Edge transEdges[NCT::N * NCT::N];
        int teLast = 0;
        for (int i = 0; i < k1; i++) {
            if (adjMat.get(i, k1) && !transClosure.get(i, k2)) {
                parent.set(i, k2);
                transEdges[teLast++] = std::make_pair(i, k2);
            }
        }
        for (int i = k2 + 1; i < NCT::N; i++) {
            if (adjMat.get(k2, i) && !transClosure.get(k1, i)) {
                parent.set(k1, i);
                transEdges[teLast++] = std::make_pair(k1, i);
            }
        }

        // check
        LinExtT linExtRevEdge;
        SortableStatus status = checkReverseEdgeSortable(parent, info, k1, k2, linExtRevEdge);
        if (SortableStatus::NO != status) {
            checkAndInsertParent(parent, info, k1, k2, linExtRevEdge, status == SortableStatus::UNFINISHED ? status : poset->GetStatus());
            exploreTransEdges(parent, info, k1, k2, transEdges, 0, teLast, poset->GetStatus());
        }
    }

/**
 * Explores predecessors of one poset.
 */
    void BackwardSearch::processPoset(PosetHandle &poset) {

        if (computeLinExt) {
            linExtFirstChild = linExtCalc.calculateLinExtensionsSingleton(poset, parentC + 1, false, true);

            if (linExtFirstChild < limitParents / 2) {
                assert(limitChildren == 1);
                return;
            }
            if (linExtFirstChild > (LinExtT(1) << (NCT::C - parentC - 1))) {
                assert(false);
            }
        }

        pred_count = 0;
        pot_pred_count = 0;
        if (poset.GetNumPairs() > 0) {
            // if there is a pair, remove it
            AdjacencyMatrix parentMat(NCT::N);
            poset->GetAdMatrix(parentMat);
            int k1 = poset.GetFirstSingleton() - 2;
            int k2 = poset.GetFirstSingleton() - 1;
            parentMat.deleteEdge(k1, k2);
            checkAndInsertParent(parentMat, poset, k1, k2, linExtFirstChild, poset->GetStatus());
        } else {
            // otherwise, iterate over all edges in the poset
            int n = poset.GetReducedN();
            for (int i = 0; i < n - 1; i++) {
                for (int j = i + 1; j < n; j++) {
                    if (poset->isEdge(i, j)) {
                        exploreComparison(poset, poset, i, j);
                    }
                }
            }
        }
        Stats::addVal<AVMSTAT::PredCount>(pred_count);
        Stats::addVal<AVMSTAT::PotPredCount>(pot_pred_count);
    }

    BackwardSearch::BackwardSearch(PosetMap &parentMap, unsigned int parentC, PosetMap &childMap, LinExtT limitChildren, LinExtT limitParents) :
            parentMap(parentMap), parentC(parentC), childMap(childMap), limitChildren(limitChildren), limitParents(limitParents), linExtCalc(NCT::N, NCT::C) {
        computeLinExt = limitParents > 1;
    }

    void processLayerBW(std::vector<PosetObj>& children, std::atomic<size_t>& childIndex, PosetMap& childMap, PosetMap& parentMap, unsigned int parentC,
                        std::atomic<float>& progress, LinExtT limitParents, LinExtT limitChildren) {

        NCT::initThread();

        BackwardSearch backwardSearch{parentMap, parentC, childMap, limitChildren, limitParents};

        int cnt = 0;
        while (true) {

            // grab a batch of posets to process
            size_t beginIndex = childIndex.fetch_add(SearchParams::batchSize);
            size_t endIndex = std::min((size_t) children.size(), beginIndex + SearchParams::batchSize);

            // terminate if batch empty
            if (endIndex <= beginIndex) {
                break;
            }

            // update progress
            progress = static_cast<float>(childIndex) / static_cast<float>(children.size());

            // process posets in batch
            for (size_t index = beginIndex; index < endIndex; index++) {
                // get poset
                PosetObj& poset = children[index];
                auto handle = PosetHandle::fromPoset(poset);
                // search
                backwardSearch.processPoset(handle);
            }

            if (cnt++ % 100 == 99) {
                Stats::accumulate();
            }
        }

        Stats::accumulate();
    }
}

void createInitialPosetBW(PosetStorage& storage) {
    PosetMap map { 1 };
    AdjacencyMatrix initialAdjMat{NCT::N};
    VertexList permutation{};
    for (int i = 0; i < NCT::N - 1; i++) {
        initialAdjMat.set(i, i+1);
        permutation.add(i);
    }
    permutation.add(NCT::N - 1);
    PosetObj initialPo{};
    PosetInfo info{0, 0};
    initialPo.SetgraphPermutation(initialAdjMat, permutation, info);
    initialPo.setUniqueGraph(true);
    initialPo.SetSelfdualId(false);
    initialPo.SetSortable();
    AnnotatedPosetObj ihandle{initialPo, PosetInfoFull(info, initialPo.computeHash()), 1};
    map.findAndInsert(ihandle);
    auto initialStats = map.countPosetsDetailed();
    StorageProfile::update(NCT::C, initialStats);
    std::array<LinExtT, MAXENDC> maxLinExt{};
    for (int i = 0; i <= NCT::C; i++) {
        maxLinExt[i] = i < NCT::N - 1 ? 0 : 1;
    }
    const Meta meta {
            .n = NCT::N,
            .c = NCT::C,
            .C = NCT::C,
            .completeAbove = 1,
            .maxLinExt = maxLinExt,
            .numYes = 1,
            .numUnf = 0,
    };
    storage.storePosets(map, meta);
}

std::array<uint64_t, 8> countMarkedPosetsDetailed(std::vector<PosetObj> &vec) {
    std::array<uint64_t, 8> result = {0, 0, 0, 0, 0, 0, 0, 0};
    for (auto &poset: vec) {
        if (poset.isMarked()) {
            result[poset.GetStatus()]++;
        }
    }
    return result;
}

void doBackwardStep(TimeProfile &profile, std::atomic<float> &progress, PosetStorage& storage, unsigned int parentC,
                    LinExtT limitParents, LinExtT limitChildren, std::vector<PosetObj> &childList, PosetMap &childMap) {

    profile.section(Section::BW_WORK);
    PosetMap parentMap{childList.size()};
    std::atomic<size_t> childIdx = 0;
    if (childMap.countPosets() > SearchParams::batchSize * 4) {
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < NCT::num_threads; i++) {
            threads.emplace_back(processLayerBW, std::ref(childList), std::ref(childIdx), std::ref(childMap), std::ref(parentMap), parentC,
                                 std::ref(progress), limitParents, limitChildren);
        }
        for (auto &thread: threads) {
            thread.join();
        }
    } else {
        processLayerBW(childList, childIdx, childMap, parentMap, parentC, progress, limitParents, limitChildren);
    }

    // stats
    profile.section(Section::OTHER);
    auto statsAfterP = parentMap.countPosetsDetailed();
    StorageProfile::update(parentC, statsAfterP);

    // store new parent posets
    profile.section(Section::BW_IO);
    Meta meta {
            .n = NCT::N,
            .c = parentC,
            .C = NCT::C,
            .completeAbove = limitParents,
            .maxLinExt = {},
            .numYes = statsAfterP[SortableStatus::YES],
            .numUnf = statsAfterP[SortableStatus::UNFINISHED],
    };
    meta.maxLinExt.fill(LinExtT(1) << (NCT::C - parentC));
    storage.storePosets(parentMap, meta);
}