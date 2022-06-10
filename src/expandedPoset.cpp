
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

#include "expandedPoset.h"
#include <iostream>

#include "niceGraph.h"
#include "posetHandle.h"
#include "stats.h"
#include "posetInfo.h"

static PosetInfo addEdge(PosetInfo &parent, int k1, int k2) {
    unsigned int numSingletons = parent.GetnumSingletons() - parent.isSingleton(k1) - parent.isSingleton(k2);
    unsigned int numPairs = (parent.isSingleton(k1) && parent.isSingleton(k2)) ? (parent.GetNumPairs() + 1) :
                            (parent.GetNumPairs() - parent.isInPair(k1) - parent.isInPair(k2));
    PosetInfo info{numSingletons, numPairs};

    assert(parent.isPairComp(k1, k2) == (parent.GetNumPairs() == 2));
    assert(k1 != k2);
    if ((k1 >= parent.GetFirstSingleton() + 2) || (k2 >= parent.GetFirstSingleton() + 2)) {
        std::cout << "N: " << NCT::N << std::endl;
        std::cout << k1 << ", " << k2 << " and reducedN: " << info.GetReducedN();
        std::cout << std::endl;
    }
    assert((k1 < parent.GetFirstSingleton() + 2) && (k2 < parent.GetFirstSingleton() + 2));
    assert((k1 < info.GetFirstSingleton()) && (k2 < info.GetFirstSingleton()));
    assert(info.GetNumPairs() <= 2);

    return {numSingletons, numPairs};
}

//helper for reorderGraphCanonically
template<typename T>
void reorderIds(std::array<T, MAXN> &ids, const VertexList &permutation) {
    std::array<T, MAXN> idcopy = ids;
    for (int i = 0; i < permutation.size(); i++) {
        ids[i] = idcopy[permutation[i]];
    }

}


template<bool isFullN>
void reorderGraphCanonically(AdjacencyMatrix &adMatrix, AdjacencyMatrix &adMatrixClosure, const PosetInfo &info,
                             PosetObj &poset, NiceGraph niceGraphClosure) {

    Stats::inc(STAT::NReorderGraph);

    const int n = NCT::N;
    const int reduced_n = (isFullN) ? NCT::N : info.GetReducedN();

    std::array<int, MAXN> outDegrees{};
    std::array<int, MAXN> inDegrees{};

    const int multiplier = 23;

    std::array<int, MAXN> degreeSequence{};
    std::array<int, MAXN> degreeSequenceRev{};


    std::array<uint64_t, MAXN> idSeq1{};
    std::array<uint64_t, MAXN> idSeq1Rev{};


    poset.SetSelfdualId(false);


    for (int node = 0; node < reduced_n; node++) {


        outDegrees[node] = niceGraphClosure.outLists[node].size();
        inDegrees[node] = niceGraphClosure.inLists[node].size();


        idSeq1[node] =
                ((1ULL << (2 * outDegrees[node] + 5)) + ((1ULL << (3 * inDegrees[node])) % PRIME1) * MULT1) % PRIME1;
        idSeq1Rev[node] =
                ((1ULL << (2 * inDegrees[node] + 5)) + ((1ULL << (3 * outDegrees[node])) % PRIME1) * MULT1) % PRIME1;


        degreeSequence[node] = multiplier * outDegrees[node] + inDegrees[node];
        degreeSequenceRev[node] = multiplier * inDegrees[node] + outDegrees[node];

        idSeq1[node] += degreeSequence[node];
        idSeq1Rev[node] += degreeSequenceRev[node];
    }


    std::array<uint64_t, MAXN> idSeq1A{};
    std::array<uint64_t, MAXN> idSeq1RevA{};


    const int numrounds = n / 3;

    for (int round = 0; round < numrounds; round++) {
        for (int i = 0; i < reduced_n; i++) {
            int node = i; //current node
            idSeq1A[node] = (idSeq1[node] * MULT1);
            idSeq1RevA[node] = (idSeq1Rev[node] * MULT1);
            for (auto out_iter = (niceGraphClosure.outLists[node]).begin();
                 out_iter != niceGraphClosure.outLists[node].end(); out_iter++) {
                idSeq1A[node] += idSeq1[*out_iter];
                idSeq1RevA[node] += idSeq1Rev[*out_iter];
            }
            for (auto in_iter = (niceGraphClosure.inLists[node]).begin();
                 in_iter != niceGraphClosure.inLists[node].end(); in_iter++) {
                idSeq1A[node] += idSeq1[*in_iter];
                idSeq1RevA[node] += idSeq1Rev[*in_iter];
            }
        }
        for (int i = 0; i < reduced_n; i++) {
            int node = i; //current node
            idSeq1[node] = idSeq1A[node] ^ (((idSeq1A[node] << 5) & (degreeSequence[node] + 0x0101FFFF00001111ULL)) +
                                            (idSeq1A[node] >> 2));
            idSeq1Rev[node] = idSeq1RevA[node] ^
                              (((idSeq1RevA[node] << 5) & (degreeSequenceRev[node] + 0x0101FFFF00001111ULL)) +
                               (idSeq1RevA[node] >> 2));
        }
    }

    auto lambda = [&idSeq1](int i, int j) {
        if (idSeq1[i] < idSeq1[j])
            return true;
        else
            return false;
    };

    auto lambdaRev = [&idSeq1Rev](int i, int j) {
        if (idSeq1Rev[i] < idSeq1Rev[j])
            return true;
        else
            return false;
    };

    AdjacencyMatrix adMatrixClosureRev = adMatrixClosure;
    adMatrixClosureRev.reverse();

    auto layersStructure = (isFullN) ? adMatrixClosureRev.getLayerStructureRev()
                                     : adMatrixClosureRev.getLayerStructureRev(reduced_n);

    auto layersStructureRev = (isFullN) ? adMatrixClosure.getLayerStructureRev() : adMatrixClosure.getLayerStructureRev(
            reduced_n);

    VertexList permutation;
    VertexList Revpermutation;

    VertexList flipIsos;
    VertexList cycleIsoStarts;
    VertexList cycleIsoLengths;

    VertexList flipIsosRev;
    VertexList cycleIsoStartsRev;
    VertexList cycleIsoLengthsRev;

    layersStructure.sortLayersAndGetPermutation(lambda, permutation, flipIsos, cycleIsoStarts, cycleIsoLengths, idSeq1);
    layersStructureRev.sortLayersAndGetPermutation(lambdaRev, Revpermutation, flipIsosRev, cycleIsoStartsRev,
                                                   cycleIsoLengthsRev, idSeq1Rev);

    if (permutation.size() != reduced_n) {
        adMatrixClosure.print();
        std::cout << " recuded_n: " << reduced_n << " permutation.size(): " << permutation.size()
                  << std::endl << std::endl;
        assert(false);
    }

    assert(permutation.size() == reduced_n);
    assert(Revpermutation.size() == reduced_n);

    int numIsoFound = 0;
    int numIsoFoundRev = 0;
    int numCycleIsoFound = 0;
    int numCycleIsoFoundRev = 0;
    bool ambiguousGraph = false;
    if (flipIsos.size() == 0 && flipIsosRev.size() == 0 && cycleIsoLengths.size() == 0 &&
        cycleIsoLengthsRev.size() == 0)
        ambiguousGraph = false;
    else {
        AdjacencyMatrix adMatrixClosureOriginal = adMatrixClosure;
        AdjacencyMatrix adMatrixClosureOriginalRev = adMatrixClosureRev;

        VertexList auxPermutation = permutation;
        VertexList auxPermutationRev = Revpermutation;

        adMatrixClosureOriginal.reorder(auxPermutation);
        adMatrixClosureOriginalRev.reorder(auxPermutationRev);

        for (int i = 0; i < flipIsos.size(); i++) {
            VertexList flipIsoPermutation = permutation;
            AdjacencyMatrix adMatrixClosureFlip = adMatrixClosure;
            std::swap(flipIsoPermutation[flipIsos[i]], flipIsoPermutation[flipIsos[i] - 1]);

            assert(idSeq1[flipIsoPermutation[flipIsos[i]]] == idSeq1[flipIsoPermutation[flipIsos[i] - 1]]);

            adMatrixClosureFlip.reorder(flipIsoPermutation);

            if (adMatrixClosureOriginal == adMatrixClosureFlip) {
                numIsoFound++;
            } else {
                ambiguousGraph = true;
                goto finishAmbiguousGraphTest;
            }
        }
        for (int i = 0; i < flipIsosRev.size(); i++) {
            VertexList flipIsoPermutationRev = Revpermutation;
            AdjacencyMatrix adMatrixClosureFlipRev = adMatrixClosureRev;
            std::swap(flipIsoPermutationRev[flipIsosRev[i]], flipIsoPermutationRev[flipIsosRev[i] - 1]);

            assert(idSeq1Rev[flipIsoPermutationRev[flipIsosRev[i]]] ==
                   idSeq1Rev[flipIsoPermutationRev[flipIsosRev[i] - 1]]);

            adMatrixClosureFlipRev.reorder(flipIsoPermutationRev);
            if (adMatrixClosureOriginalRev == adMatrixClosureFlipRev) {
                numIsoFoundRev++;
            } else {
                ambiguousGraph = true;
                goto finishAmbiguousGraphTest;
            }
        }

        for (int i = 0; i < cycleIsoStarts.size(); i++) {
            AdjacencyMatrix adMatrixClosureCycle = adMatrixClosure;
            VertexList cycleIsoPermutation = permutation;

            int endIndex = cycleIsoStarts[i];
            int temp = cycleIsoPermutation[endIndex];
            for (int j = 1; j < cycleIsoLengths[i]; j++)
                cycleIsoPermutation[endIndex - j + 1] = cycleIsoPermutation[endIndex - j];
            cycleIsoPermutation[endIndex - cycleIsoLengths[i] + 1] = temp;

            assert(idSeq1[cycleIsoPermutation[cycleIsoStarts[i]]] ==
                   idSeq1[cycleIsoPermutation[cycleIsoStarts[i] - 1]]);

            adMatrixClosureCycle.reorder(cycleIsoPermutation);
            if (adMatrixClosureOriginal == adMatrixClosureCycle) {
                numCycleIsoFound++;
            } else {
                ambiguousGraph = true;
                goto finishAmbiguousGraphTest;
            }
        }

        for (int i = 0; i < cycleIsoStartsRev.size(); i++) {
            AdjacencyMatrix adMatrixClosureCycleRev = adMatrixClosureRev;
            VertexList cycleIsoPermutationRev = Revpermutation;

            int endIndex = cycleIsoStartsRev[i];
            int temp = cycleIsoPermutationRev[endIndex];
            for (int j = 1; j < cycleIsoLengthsRev[i]; j++)
                cycleIsoPermutationRev[endIndex - j + 1] = cycleIsoPermutationRev[endIndex - j];
            cycleIsoPermutationRev[endIndex - cycleIsoLengthsRev[i] + 1] = temp;

            assert(idSeq1Rev[cycleIsoPermutationRev[cycleIsoStartsRev[i]]] ==
                   idSeq1Rev[cycleIsoPermutationRev[cycleIsoStartsRev[i] - 1]]);
            adMatrixClosureCycleRev.reorder(cycleIsoPermutationRev);
            if (adMatrixClosureOriginalRev == adMatrixClosureCycleRev) {
                numCycleIsoFoundRev++;
            } else {
                ambiguousGraph = true;
                goto finishAmbiguousGraphTest;
            }
        }
    }

    finishAmbiguousGraphTest:
	
	assert(ambiguousGraph || numIsoFound == numIsoFoundRev);
	assert(ambiguousGraph || numCycleIsoFound == numCycleIsoFoundRev);
    Stats::addVal<NAutoFound>(std::max({numIsoFound, numIsoFoundRev, numCycleIsoFound, numCycleIsoFoundRev}));
    Stats::addVal<NCycleAutoFound>(std::max(numCycleIsoFound, numCycleIsoFoundRev));

    if (ambiguousGraph) {
        Stats::inc(STAT::NAmbiguous);
        poset.setUniqueGraph(false);
    } else {
        poset.setUniqueGraph(true);
        Stats::inc(STAT::NAmbiguousIso);
    }

    reorderIds(idSeq1, permutation);
    reorderIds(idSeq1Rev, Revpermutation);
    bool reverse = false;
    if (std::lexicographical_compare(idSeq1.begin(), idSeq1.begin() + reduced_n, idSeq1Rev.begin(),
                                     idSeq1Rev.begin() + reduced_n))
        reverse = true;
    else if (std::equal(idSeq1.begin(), idSeq1.begin() + reduced_n, idSeq1Rev.begin())) {
        if (ambiguousGraph) {
            poset.SetSelfdualId(true);
            poset.setUniqueGraph(false);
            Stats::inc(STAT::NSelfdualIdCreated);
        } else {
            AdjacencyMatrix adMatrixClosureOriginal = adMatrixClosure;
            AdjacencyMatrix adMatrixClosureOriginalRev = adMatrixClosureRev;

            VertexList auxPermutation = permutation;
            VertexList auxPermutationRev = Revpermutation;

            adMatrixClosureOriginal.reorder(auxPermutation);
            adMatrixClosureOriginalRev.reorder(auxPermutationRev);

            if (!(adMatrixClosureOriginal == adMatrixClosureOriginalRev)) {
                poset.SetSelfdualId(true);
                poset.setUniqueGraph(false);
                Stats::inc(STAT::NSelfdualIdCreated);
            }
        }
    }

    if (reverse) {
        poset.SetgraphPermutationReverse(adMatrix, Revpermutation, info);
    } else {
        poset.SetgraphPermutation(adMatrix, permutation, info);
    }
}

ExpandedPosetChild::ExpandedPosetChild(PosetHandle &parent, LinExtT linExt, int kk1, int kk2) :
        niceGraphClosure(NCT::N),
        poset(),
        info(addEdge(parent, kk1, kk2)),
        linExt(linExt) {
    const int n = NCT::N;

    AdjacencyMatrix adMatrix(n);
    AdjacencyMatrix adMatrixClosure(n);

    if (parent.GetNumPairs() >= 1 && (parent.isSingleton(kk1) || parent.isSingleton(kk2)) &&
        (parent.isInBigPart(kk1) || parent.isInBigPart(kk2))) {
        assert(false);
    }
    parent->GetAdMatrix(adMatrix);
    adMatrix.set(kk1, kk2);

    adMatrixClosure = adMatrix;
    adMatrixClosure.TransitiveClosure();
    niceGraphClosure.set(adMatrixClosure);

    adMatrix.transReduction(kk1, kk2, niceGraphClosure);

    if (info.GetReducedN() == NCT::N)
        reorderGraphCanonically<true>(adMatrix, adMatrixClosure, info, poset, niceGraphClosure);
    else
        reorderGraphCanonically<false>(adMatrix, adMatrixClosure, info, poset, niceGraphClosure);

    if (info.GetNumPairs() >= 1) {
        unsigned int reduced_n = info.GetReducedN();
        assert(poset.isEdge(reduced_n, reduced_n + 1));
        if (info.GetNumPairs() >= 2) {
            assert(poset.isEdge(reduced_n + 2, reduced_n + 3));
        }
    }
}

ExpandedPosetChild::ExpandedPosetChild(const AdjacencyMatrix& p, const PosetInfo& info, LinExtT linExt, int k1, int k2) :
        niceGraphClosure(NCT::N),
        poset(),
        info(info),
        linExt(linExt) {
    const int n = NCT::N;

    AdjacencyMatrix adMatrix = p;
    AdjacencyMatrix adMatrixClosure(n);

    adMatrixClosure = adMatrix;
    adMatrixClosure.TransitiveClosure();
    niceGraphClosure.set(adMatrixClosure);

    adMatrix.transReduction(k1, k2, niceGraphClosure);

    if (info.GetReducedN() == NCT::N)
        reorderGraphCanonically<true>(adMatrix, adMatrixClosure, info, poset, niceGraphClosure);
    else
        reorderGraphCanonically<false>(adMatrix, adMatrixClosure, info, poset, niceGraphClosure);

    if (info.GetNumPairs() >= 1) {
        unsigned int reduced_n = info.GetReducedN();
        assert(poset.isEdge(reduced_n, reduced_n + 1));
        if (info.GetNumPairs() >= 2) {
            assert(poset.isEdge(reduced_n + 2, reduced_n + 3));
        }
    }
}

ExpandedPosetChild::ExpandedPosetChild(const AdjacencyMatrix& p, const PosetInfo& info, LinExtT linExt) :
        niceGraphClosure(NCT::N),
        poset(),
        info(info),
        linExt(linExt) {
    const int n = NCT::N;

    AdjacencyMatrix adMatrix = p;
    AdjacencyMatrix adMatrixClosure(n);

    adMatrixClosure = adMatrix;
    adMatrixClosure.TransitiveClosure();
    niceGraphClosure.set(adMatrixClosure);

    if (info.GetReducedN() == NCT::N)
        reorderGraphCanonically<true>(adMatrix, adMatrixClosure, info, poset, niceGraphClosure);
    else
        reorderGraphCanonically<false>(adMatrix, adMatrixClosure, info, poset, niceGraphClosure);

    if (info.GetNumPairs() >= 1) {
        unsigned int reduced_n = info.GetReducedN();
        assert(poset.isEdge(reduced_n, reduced_n + 1));
        if (info.GetNumPairs() >= 2) {
            assert(poset.isEdge(reduced_n + 2, reduced_n + 3));
        }
    }
}

bool ExpandedPosetChild::isEasilySortableUnrelatedPairs(unsigned int cLeft) {
    if (cLeft <= 6) {
        const unsigned int n = NCT::N;
        unsigned int totalOut = 0;
        for (int i = 0; i < n; i++) {
            totalOut += niceGraphClosure.outLists[i].size();
        }
        int numUnrelated = (n * (n - 1) / 2) - totalOut;


        if (numUnrelated <= cLeft) {
            return true;
        }
    }
    return false;
}

AnnotatedPosetObj ExpandedPosetChild::getHandle() {
    return {poset, PosetInfoFull(info, poset.computeHash()), linExt};
}
