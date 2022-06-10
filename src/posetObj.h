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

#ifndef POSETOBJ_H
#define POSETOBJ_H

#include <boost/graph/adjacency_list.hpp>
#include "posetObjCore.h"

#include "posetInfo.h"
#include "config.h"

class AdjacencyMatrix;
class VertexList;



constexpr std::array<unsigned int, MAXN> fill_array() {
    std::array<unsigned int, MAXN> jOffset{0};
    int add = MAXN - 1;
    for (int f = 1; f < MAXN; f++){
        jOffset[f] = jOffset[f-1] + add;
        add--;
    }
    return jOffset;
}


class PosetObj {
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS> Boostgraph;

	static constexpr std::array<unsigned int, MAXN> jOffset = fill_array();


private:
    PosetObjCore posetCore;

public:

    PosetObj(const PosetObj &other) = default;

    PosetObj() : posetCore() {}

    static void init();

    [[nodiscard]] inline bool GetSelfdualId() const {
        return posetCore.GetSelfdualId();
    }

    inline void SetSelfdualId(bool newId) {
        posetCore.SetSelfdualId(newId);
    }


    [[nodiscard]] inline SortableStatus GetStatus() const {
        return posetCore.GetStatus();
    }

    inline void SetUnsortable() {
        assert(posetCore.GetStatus() == SortableStatus::UNFINISHED);
        posetCore.SetStatus(SortableStatus::NO);
    }

    inline void SetSortable() {
        assert(posetCore.GetStatus() == SortableStatus::UNFINISHED);
        posetCore.SetStatus(SortableStatus::YES);
    }

    //when creating a new poset, this needs to be called on Parent!
    void GetAdMatrix(AdjacencyMatrix &bla) const;

    void SetgraphPermutation(const AdjacencyMatrix &adMatrix, VertexList &permutation, const PosetInfo &info);

    void SetgraphPermutationReverse(const AdjacencyMatrix &adMatrix, VertexList &permutation, const PosetInfo &info);

    [[nodiscard]] Boostgraph GetBoostgraph() const;

    [[nodiscard]] Boostgraph GetReducedBoostgraph(int reducedN) const;

    [[nodiscard]] Boostgraph GetRevReducedBoostgraph(int reducedN) const;

    [[nodiscard]] bool isSingletonsAbove(unsigned int startSingletons) const;

    [[nodiscard]] bool isPairs(unsigned int startPairs, unsigned int numPairs) const;

    [[nodiscard]] inline bool isUniqueGraph() const {
        return posetCore.isUniqueGraph();
    }

    inline void setUniqueGraph(bool unique) {
        posetCore.setUniqueGraph(unique);
    }

    [[nodiscard]] inline bool isMarked() const {
        return posetCore.isMarked();
    }

    inline void setMark(bool mark) {
        posetCore.setMark(mark);
    }

    [[nodiscard]] inline bool isEdge(int source, int target) const {
        return is_edge(source, target);
    }

    [[nodiscard]] uint64_t computeHash() const;

    [[nodiscard]] bool SameGraph(const PosetObj &other) const;

    /**
    * Prints the graph of the Poset in adjacency list represantation.
    */
    void print_poset() const;


private:


    /**
    * hash calculation for posets with isUniqueGraph() == false
    */
    [[nodiscard]] uint64_t fullFastHash() const;

    /**
     * Adds an edge to the PosetObj i.e. its graph.
     *
     * @param j The source of the edge
     * @param k The target of the edge
     */
    inline void add_edge(int j, int k) {
        assert(j < k);
        posetCore.graphSet(k + jOffset[j] - j - 1);
    }
	
	inline void add_edgeCond(int j, int k, bool val) {
        assert(j < k);
        posetCore.graphSetToOr(k + jOffset[j] - j - 1, val);
    }

    [[nodiscard]] inline bool is_edge(int j, int k) const {
        if (j >= k)
            return false;
        return posetCore.graphGet(k + jOffset[j] - j - 1);
    }
};

class AnnotatedPosetObj : public PosetObj, public PosetInfoFull {

public:

    uint64_t elIndex;
    LinExtT linExt;

    AnnotatedPosetObj(const PosetObj &poset, PosetInfoFull info, LinExtT linExt) : PosetObj(poset), PosetInfoFull(info), linExt(linExt), elIndex(0) {}

    AnnotatedPosetObj() : PosetObj(), PosetInfoFull(PosetInfo(0, 0), 0) {}

};

#endif // POSETOBJ_H
