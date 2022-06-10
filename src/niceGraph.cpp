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


#include "niceGraph.h"
#include <iostream>
#include <bitset>

bool VertexList::operator==(const VertexList &other) {
    if (num != other.num)
        return false;
    for (int i = 0; i < num; i++)
        if (vertices[i] != other.vertices[i])
            return false;
    return true;
}

void VertexList::extendPermutation(int n) {
    DEBUG_ASSERT(n <= MAXN);
    for (int i = num; i < n; i++)
        vertices[i] = i;
    num = n;
}

bool LayerStructure::operator==(const LayerStructure &other) {
    if (numLayers != other.numLayers)
        return false;
    for (int i = 0; i < numLayers; i++)
        if (!(layers[i] == other.layers[i]))
            return false;
    return true;
}

void AdjacencyMatrix::reset(unsigned int num) {
    for (int i = 0; i < num; i++)
        data[i] = 0;
}

bool AdjacencyMatrix::operator==(const AdjacencyMatrix &other) {
#ifdef FIXSIZE
    if (n != other.n)
        return false;
    else
        return data == other.data;
#else
    if(n!= other.n)
            return false;
        else
            for (int i = 0; i < n; i++)
                if(data[i] != other.data[i])
                    return false;
        return true;

#endif
}

void AdjacencyMatrix::print() const {
    std::cout << std::endl;
    for (int i = 0; i < n; i++) {
        for (int j = 0 ; j < n ; j++) {
            std::cout << get(i, j)<<" ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

}

void AdjacencyMatrix::reverse() {
    std::array<uint32_t, MAXN> newdata;
    for (int i = 0; i < n; i++) {
        newdata[i] = 0;
        for (int j = n - 1; j >= 0; j--) {
            newdata[i] <<= 1;
            newdata[i] |= (uint32_t) get(j, i);
        }
    }
    for (int i = n; i < MAXN; i++) {
        newdata[i] = 0;
    }
    data = newdata;
}

void AdjacencyMatrix::reorder(VertexList &permutation) {
    std::array<uint32_t, MAXN> newdata;
    int numPermuted = permutation.size();
    DEBUG_ASSERT(numPermuted <= n);
    if (numPermuted < n)
        permutation.extendPermutation(n);
    for (int i = 0; i < n; i++) {
        newdata[i] = 0;
        for (int j = n - 1; j >= 0; j--) {
            newdata[i] <<= 1;
//                newdata[i] |= get(permutation[i], permutation[j]);
            newdata[i] |= (data[permutation[i]] >> permutation[j]) & 1;

        }
    }
    for (int i = n; i < MAXN; i++) {
        newdata[i] = 0;
    }
    data = newdata;
}

LayerStructure AdjacencyMatrix::getLayerStructureRev() const {
    LayerStructure result;
    uint32_t startMask = (1 << NCT::N) - 1;
    uint32_t availableMask = startMask;
    result.numLayers = 0;
    assert(n == NCT::N);
    while (availableMask) {
        uint32_t layermask = 0;
        uint32_t j_mask = 1;
        for (int j = 0; j < NCT::N; j++, j_mask<<=1) {
            bool flag = ((availableMask & (data[j] | j_mask)) == j_mask);
            result.layers[result.numLayers].addCond(j,flag);
            layermask |= (flag ? j_mask : 0);
        }
        availableMask &= ~layermask;
        result.numLayers++;
    }
    return result;
}

LayerStructure AdjacencyMatrix::getLayerStructureRev(int reduced_N) const {
    LayerStructure result;
    uint32_t startMask = (1 << reduced_N) - 1;
    uint32_t availableMask = startMask;
    result.numLayers = 0;
    assert(reduced_N <= n);
    assert(n <= MAXN);
    while (availableMask) {
        uint32_t layermask = 0;
        uint32_t j_mask = 1;
        for (int j = 0; j < reduced_N; j++, j_mask <<= 1) {
            bool flag = ((availableMask & (data[j] | j_mask)) == j_mask);
            result.layers[result.numLayers].addCond(j, flag);
            layermask |= (flag ? j_mask : 0);
        }
        availableMask &= ~layermask;
        result.numLayers++;
    }
    return result;
}

LayerStructure AdjacencyMatrix::getLayerStructureRevOld(int reduced_N) const {
    LayerStructure result;
    int counter = 0;
    uint32_t availableMask = 0xFFFFFFFF;
    result.numLayers = 0;
    assert(reduced_N <= n);
    assert(n <= MAXN);
    while (counter < reduced_N) {
        uint32_t layermask = 0;
        uint32_t j_mask = 1;
        for (int j = 0; j < reduced_N; j++, j_mask <<= 1) {
            if (((availableMask & j_mask) != 0) && (availableMask & data[j]) == 0) {
                result.layers[result.numLayers].add(j);
                layermask |= j_mask;
                counter++;
            }
        }
        availableMask &= ~layermask;
        result.numLayers++;
    }
    return result;
}

void AdjacencyMatrix::writeToGraph(NiceGraph &graph) const {
    graph.reset(n);
    for (int j = 0; j < n; j++) {
        uint32_t outVector = data[j];
        for (int k = 0; outVector != 0; k++, outVector >>= 1) {
            if (outVector & 1)
                graph.add_edge(j, k);
        }
    }
}

void AdjacencyMatrix::TransitiveClosure() {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            uint32_t sourcemask = -((int32_t)(data[i] >> k) & 1);
            //               uint32_t sourcemask = get(i, k) ? 0xFFFFFFFF : 0;
            data[i]  |= sourcemask & data[k];
        }
    }
}

void AdjacencyMatrix::transReduction(int newsource, int newtarget, NiceGraph &niceGraphClosure) {

    for (auto out_iter = (niceGraphClosure.outLists[newtarget]).begin();
         out_iter != niceGraphClosure.outLists[newtarget].end(); out_iter++) {
        deleteEdge(newsource, *out_iter);
    }
    for (auto in_iter = (niceGraphClosure.inLists[newsource]).begin();
         in_iter != niceGraphClosure.inLists[newsource].end(); in_iter++) {
        deleteEdge(*in_iter, newtarget);
    }


    for (auto out_iter = (niceGraphClosure.outLists[newtarget]).begin();
         out_iter != niceGraphClosure.outLists[newtarget].end(); out_iter++) {
        for (auto in_iter = (niceGraphClosure.inLists[newsource]).begin(); in_iter != niceGraphClosure.inLists[newsource].end(); in_iter++) {
            deleteEdge(*in_iter, *out_iter);
        }
    }
}

size_t AdjacencyMatrix::edgeCount() const {
    size_t count = 0;
    for (int i = 0; i < this->n; i++) {
        count += std::bitset<32>(this->data[i]).count();
    }
    return count;
}

void NiceGraph::reset(int numV) {
    n = numV;
    for (int i = 0; i < n; i++) {
        outLists[i].reset();
        inLists[i].reset();
    }
}
