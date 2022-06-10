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

#pragma once
#ifndef NICEGRAPH_H
#define NICEGRAPH_H

#include<array>
#include<cassert>
#include <algorithm>
#include <cstddef>

#include "config.h"

class NiceGraph;

class VertexList {
    int num;
    std::array<int, MAXN> vertices;
public:

    VertexList() : num(0), vertices() {}

    void add(int newVertex) {
        DEBUG_ASSERT(num < MAXN);
        vertices[num] = newVertex;
        num++;
    }

    void addCond(int newVertex, bool flag) {
        DEBUG_ASSERT(num < MAXN);
        vertices[num] = newVertex;
        num += flag;
    }
	
    void reset() {
        num = 0;
    }

    int size() const {
        return num;
    }

    int operator[] (int i) const {
        return vertices[i];
    }

	int & operator[] (int i) {
        return vertices[i];
    }

    bool operator==(const VertexList &other);

    auto begin() {
        return vertices.begin();
    }
    auto end() {
        return vertices.begin() + num;
    }

    void extendPermutation(int n);
};

//libstc++ insertionSort
template<typename Iter, typename Compare>
void insertionSort(Iter first, Iter end, Compare less) {
    if (first == end) return;

    for (Iter it = first + 1; it != end; ++it) {
        int val = *it;
        Iter next = it;
        Iter next2 = it;
        --next;
        while (next2 != first && less(val, *next)) {
            *next2 = *next;
            next2 = next;
            --next;
        }
        *next2 = val;
    }
}

struct LayerStructure {
    int numLayers = 0;
    std::array<VertexList, MAXN> layers;

    bool operator==(const LayerStructure &other);

    template<typename Compare>
    void
    sortLayersAndGetPermutation(Compare less, VertexList &permutation, VertexList &flipIsos, VertexList &cycleIsoStarts,
                                VertexList &cycleIsoLengths, const std::array<uint64_t, MAXN> &idSeq) {
        int numTotal = 0;
        for (int i = 0; i < numLayers; i++) {
            insertionSort(layers[i].begin(), layers[i].end(), less);
            assert(std::is_sorted(layers[i].begin(), layers[i].end(), less));
            assert(layers[i].size() >= 1);
            int last = layers[i][0];
            int lastIndex = 0;
            numTotal++;
            permutation.add(last);
            for (int j = 1; j < layers[i].size(); j++, numTotal++)
			{
				if (idSeq[last] == idSeq[layers[i][j]]){
					if(lastIndex == j-1)
						flipIsos.add(numTotal);
					else{
						if( j + 1 == layers[i].size() || idSeq[last] != idSeq[layers[i][j + 1]]){
							cycleIsoStarts.add(numTotal);
							cycleIsoLengths.add(j + 1 - lastIndex);
						}
					}
				}
				else{
					last = layers[i][j];
					lastIndex = j;
				}
				permutation.add(layers[i][j]);
			}
		}		
	}
};

class AdjacencyMatrix {
    unsigned int n;
    std::array<uint32_t, MAXN> data;
public:
    explicit AdjacencyMatrix(unsigned int num) : n(num) {
        reset(num);
    }

    void reset(unsigned int num);

    [[nodiscard]] inline bool get(int source, int target) const {
        return (data[source] & (1<<target))!=0;
    }

    inline void set(int source, int target) {
        data[source] |= (1 << target);
    }
	
	inline void setToOr(int source, int target, bool val) {
        data[source] |= (val << target);
    }
	
    inline void deleteEdge(int source, int target) {
        data[source] &= ~(1 << target);
    }

    void removeEdges(int target) {
        for (int i = 0; i < n; i++)
            deleteEdge(i, target);
    }

    [[nodiscard]] inline unsigned int size() const {
        return n;
    }

    inline uint32_t getOutVector(int source) {
        return data[source];
    }

    bool operator==(const AdjacencyMatrix &other);

    void print() const;

    void reverse();

    void reorder(VertexList &permutation);

    LayerStructure getLayerStructureRev() const;

    LayerStructure getLayerStructureRev(int reduced_N) const;

    LayerStructure getLayerStructureRevOld(int reduced_N) const;

    void writeToGraph(NiceGraph &graph) const;

    void TransitiveClosure();

    void transReduction(int newsource, int newtarget, NiceGraph &niceGraphClosure);

    size_t edgeCount() const;
};

class NiceGraph {
    int n;
public:

    explicit NiceGraph(int numV) : n(numV) {
        DEBUG_ASSERT(n <= MAXN);
    }

    void reset(int numV);

    int size() const {
        return n;
    }

    void add_edge(int j, int k) {
        outLists[j].add(k);
        inLists[k].add(j);
    }

    void reverse() {
        std::swap(outLists, inLists);
    }

    void set(const AdjacencyMatrix &adMatrix) {
        adMatrix.writeToGraph(*this);
    }

    std::array<VertexList, MAXN> outLists;
    std::array<VertexList, MAXN> inLists;
};




#endif