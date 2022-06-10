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


#include "posetObj.h"

#include <iostream>
#include <boost/graph/graph_utility.hpp>

#include "posetInfo.h"
#include "niceGraph.h"


void PosetObj::GetAdMatrix(AdjacencyMatrix &bla) const {
    bla.reset(NCT::N);
    for (int j = 0; j < NCT::N; j++) {
        for (int k = j + 1; k < NCT::N; k++) {
            bla.setToOr(j, k, is_edge(j, k));
        }
    }
}

void PosetObj::SetgraphPermutation(const AdjacencyMatrix &adMatrix, VertexList & permutation, const PosetInfo& info) {
    posetCore.graphReset();
    int numPermuted = permutation.size();
    DEBUG_ASSERT(numPermuted <= NCT::N);
    DEBUG_ASSERT(adMatrix.size() == NCT::N);
    DEBUG_ASSERT(numPermuted == info.GetFirstInPair());
    for (int i = 0; i < numPermuted; i++) {
        for (int j = i + 1; j < numPermuted; j++) {
            add_edgeCond(i, j, adMatrix.get(permutation[i],permutation[j]));
        }
    }
    if (info.GetNumPairs() >= 1)
    {
        if (!(adMatrix.get(info.GetFirstInPair(), info.GetFirstInPair() + 1))) {
            std::cout << "firstinPair: " << info.GetFirstInPair() << " firstsingleton: " << info.GetFirstSingleton() << std::endl << std::endl;
            print_poset();

            std::cout << "numPairs: " << info.GetNumPairs() << std::endl;
            if ((adMatrix.get(info.GetFirstInPair() + 1, info.GetFirstInPair())))
                std::cout << "reverse Edge is there" << std::endl;
            else
                std::cout << "reverse Edge is NOT there" << std::endl;

            adMatrix.print();
        }

        assert(adMatrix.get(info.GetFirstInPair(), info.GetFirstInPair() + 1));
        add_edge(info.GetFirstInPair(), info.GetFirstInPair() + 1);
    }
    if (info.GetNumPairs() == 2)
    {
        assert(adMatrix.get(info.GetFirstInPair() + 2, info.GetFirstInPair() + 3));
        add_edge(info.GetFirstInPair() + 2, info.GetFirstInPair() + 3);
    }

}

void PosetObj::SetgraphPermutationReverse(const AdjacencyMatrix &adMatrix, VertexList& permutation,
                                          const PosetInfo& info) {
    posetCore.graphReset();
    int numPermuted = permutation.size();
    DEBUG_ASSERT(numPermuted <= NCT::N);
    DEBUG_ASSERT(adMatrix.size() == NCT::N);
    DEBUG_ASSERT(numPermuted == info.GetFirstInPair());
    for (int i = 0; i < numPermuted; i++) {
        for (int j = i + 1; j < numPermuted; j++) {
            add_edgeCond(i, j,adMatrix.get(permutation[j], permutation[i]));
            //              if (adMatrix.get(permutation[j], permutation[i]))
            //                  add_edge(i, j);
        }
    }
    if (info.GetNumPairs() >= 1)
    {
        assert(adMatrix.get(info.GetFirstInPair(), info.GetFirstInPair() + 1));
        add_edge(info.GetFirstInPair(), info.GetFirstInPair() + 1);
    }
    if (info.GetNumPairs() == 2)
    {
        assert(adMatrix.get(info.GetFirstInPair() + 2, info.GetFirstInPair() + 3));
        add_edge(info.GetFirstInPair() + 2, info.GetFirstInPair() + 3);
    }
}

PosetObj::Boostgraph PosetObj::GetBoostgraph() const {
    return this->GetReducedBoostgraph(NCT::N);
}

PosetObj::Boostgraph PosetObj::GetReducedBoostgraph(int reducedN) const {
    int m = reducedN;
    DEBUG_ASSERT(m <= NCT::N);
    Boostgraph bla(m);
    for (int j = 0; j < m; j++) {
        for (int k = j+1; k < m; k++) {
            if (is_edge(j, k))
                boost::add_edge(j, k, bla);
        }
    }
    return bla;
}

PosetObj::Boostgraph PosetObj::GetRevReducedBoostgraph(int reducedN) const {
    int m = reducedN;
    DEBUG_ASSERT(m <= NCT::N);
    Boostgraph bla(m);
    for (int j = 0; j < m; j++) {
        for (int k = j+1; k < m; k++) {
            if (is_edge(j,k))
                boost::add_edge(k, j, bla);
        }
    }
    return bla;
}

bool PosetObj::isSingletonsAbove(unsigned int startSingletons) const {
    DEBUG_ASSERT(startSingletons <= NCT::N);
    //check that no incoming edges
    for (int j = 0; j < startSingletons; j++) {
        for (int k = startSingletons; k < NCT::N; k++) {
            if (is_edge(j,k))
                return false;
        }
    }
    //check that independent set
    for (int j = startSingletons; j < NCT::N; j++) {
        for (int k = j + 1; k < NCT::N; k++) {
            if (is_edge(j,k))
                return false;
        }
    }
    return true;
}

bool PosetObj::isPairs(unsigned int startPairs, unsigned int numPairs) const {
    int endPairs = startPairs + 2 * numPairs;
    DEBUG_ASSERT(endPairs <= NCT::N);
    if(numPairs >= 1){
        if (!is_edge(startPairs,startPairs + 1))
            return false;
        if(numPairs >= 2){
            assert(numPairs == 2);
            if (!is_edge(startPairs + 2,startPairs + 3))
                return false;
            if(is_edge(startPairs, startPairs + 2) || is_edge(startPairs, startPairs + 3) || is_edge(startPairs + 1, startPairs + 2) || is_edge(startPairs + 1, startPairs + 3))
                return false;
        }
        //check that no incoming edges
        for (int j = 0; j < startPairs; j++) {
            for (int k = startPairs; k < endPairs; k++) {
                if (is_edge(j,k))
                    return false;
            }
        }
        //check that no outgoing edges
        for (int j = startPairs; j < endPairs; j++) {
            for (int k = endPairs; k < NCT::N; k++) {
                if (is_edge(j,k))
                    return false;
            }
        }
    }
    return true;
}

uint64_t PosetObj::computeHash() const {
    if(isUniqueGraph())
        return posetCore.hashFromGraph();
    else
        return fullFastHash();
}

uint64_t PosetObj::fullFastHash() const {
    const int n = NCT::N;
    AdjacencyMatrix adMatrix(n);

    GetAdMatrix(adMatrix);

    NiceGraph niceGraph(n);
    niceGraph.set(adMatrix);

    std::array<int, MAXN> outDegrees;
    std::array<int, MAXN> inDegrees;

    const unsigned int multiplier = 23;

    std::array<int, MAXN> degreeSequence;
    std::array<int, MAXN> degreeSequenceRev;

    std::array<uint64_t, MAXN> idSeq1;
    std::array<uint64_t, MAXN> idSeq1Rev;

    for (int node = 0; node < n; node++) {
        outDegrees[node] = niceGraph.outLists[node].size();
        inDegrees[node]  = niceGraph.inLists[node].size();

        idSeq1[node] 	= ((1ULL << (2 * outDegrees[node] + 5)) + ((1ULL << (3 * inDegrees[node])) ) *MULT1) % PRIME1;
        idSeq1Rev[node] = ((1ULL << (2 * inDegrees[node] + 5))  + ((1ULL << (3 * outDegrees[node])) )*MULT1) % PRIME1;

        degreeSequence[node] 	= multiplier*outDegrees[node] + inDegrees[node];
        degreeSequenceRev[node] = multiplier*inDegrees[node]  + outDegrees[node];

        idSeq1[node] 	+= degreeSequence[node];
        idSeq1Rev[node] += degreeSequenceRev[node];
    }


    std::array<uint64_t, MAXN> idSeq1A;
    std::array<uint64_t, MAXN> idSeq1RevA;

    const int numrounds = n / 4;

    for (int round = 0; round < numrounds; round++)
    {
        for (int node = 0; node < n; node++) {
            idSeq1A[node] = (idSeq1[node] * 9);
            idSeq1RevA[node] = (idSeq1Rev[node] * 9);
            for (auto out_iter = (niceGraph.outLists[node]).begin(); out_iter != niceGraph.outLists[node].end(); out_iter++){
                idSeq1A[node] 	 += idSeq1[*out_iter];
                idSeq1RevA[node] += idSeq1Rev[*out_iter];
            }
            for (auto in_iter = (niceGraph.inLists[node]).begin(); in_iter != niceGraph.inLists[node].end(); in_iter++){
                idSeq1A[node] 	 += idSeq1[*in_iter];
                idSeq1RevA[node] += idSeq1Rev[*in_iter];
            }
        }

        for (int node = 0; node < n; node++) {
            idSeq1[node] 	= idSeq1A[node] ^ (((idSeq1A[node] << 25) &  0xF1F1FFFF00001111ULL) + degreeSequence[node] +(idSeq1A[node] >> 2));
            idSeq1Rev[node] = idSeq1RevA[node] ^ (((idSeq1RevA[node] << 25) & 0xF1F1FFFF00001111ULL) + degreeSequenceRev[node] + (idSeq1RevA[node] >> 2));
        }
    }

    std::sort(idSeq1.begin(), idSeq1.begin() + n);
    std::sort(idSeq1Rev.begin(), idSeq1Rev.begin() + n);

    uint64_t id, idRev;
    id =  11;
    idRev =  11;

    uint64_t mult3 = 13453;
    for (int node = 0; node < n; node++) {
        id ^= 		(idSeq1[node] << node) 	  ^  (idSeq1[node] 	 * mult3)  ^ ((id 	&  0xF1F1FFFF00001111ULL) >> 2)	 ^ (id << 17);
        idRev ^= 	(idSeq1Rev[node] << node) ^  (idSeq1Rev[node] * mult3) ^ ((idRev &  0xF1F1FFFF00001111ULL) >> 2) ^ (idRev << 17);

        mult3 *= 0x1001;
        mult3 %= PRIME1;
    }

    return std::min(id, idRev);
}

bool PosetObj::SameGraph(const PosetObj &other) const {
    return posetCore.SameGraph(other.posetCore);
}

void PosetObj::print_poset() const {
    boost::print_graph(this->GetBoostgraph());
}
