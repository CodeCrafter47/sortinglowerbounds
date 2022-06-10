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


#include <fstream>
#include <filesystem>

#include "posetAnalyser.h"
#include "linExtCalculator.h"
#include "storeAndLoad.h"
#include "eventLog.h"
#include "niceGraph.h"
#include <boost/graph/connected_components.hpp>

void analysePosets(PosetStorage &storage) {

    NCT::initThread();

    constexpr bool aSortable = true;
    constexpr bool aLinExt = true;
    constexpr bool aEdgeCount = true;
    constexpr bool aReducedSize = true;
    constexpr bool aComponents = true;

    // read posets
    EventLog::write(false, "Reading posets");
    std::vector<std::vector<AnnotatedPosetObj>> posets;
    posets.reserve(NCT::C + 1);
    for (unsigned int c = 0; c <= NCT::C; c++) {
        EventLog::write(false, "Reading c=" + std::to_string(c));
        auto &vec = posets.emplace_back();
        vec.reserve(storage.unfinishedElements[c] + storage.finishedElements[c]);
        storage.readAll(vec, c);
    }

    // output
    std::filesystem::path path = "./outputs";
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
    std::ofstream of{path / ("analysis_n" + std::to_string(NCT::N) + "_" + currentDateTime() + ".csv")};

    of << "c";
    if (aSortable) { of << ",sortable"; }
    if (aLinExt) { of << ",linExt"; }
    if (aEdgeCount) { of << ",edgeCount"; }
    if (aReducedSize) { of << ",reducedN"; }
    if (aComponents) { of << ",components"; }
    of << '\n';

    // analysis
    EventLog::write(false, "Analysing posets");
    for (unsigned int c = 0; c <= NCT::C; c++) {
        EventLog::write(false, "Layer c=" + std::to_string(c));

        auto& vec = posets[c];

        for (auto &poset: vec) {
            auto handle = PosetHandle::fromPoset(poset);

            of << c;

            if (aSortable) {
                bool sortable = handle->GetStatus() == SortableStatus::YES;
                of << "," << sortable;
            }

            if (aLinExt) {
                LinExtT ext = poset.extensions;
                of << "," << ext;
            }

            if (aEdgeCount) {
                AdjacencyMatrix mat{NCT::N};
                handle->GetAdMatrix(mat);
                auto edgeCount = mat.edgeCount();
                of << "," << edgeCount;
            }

            if (aReducedSize) {
                of << "," << handle.GetReducedN();
            }

            if (aComponents) {
                unsigned int reducedN = handle.GetReducedN();
                if (reducedN > 0) {
                    boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> G{reducedN};
                    for (int j = 0; j < reducedN; j++) {
                        for (int k = j + 1; k < reducedN; k++) {
                            if (handle->isEdge(j, k))
                                boost::add_edge(j, k, G);
                        }
                    }
                    std::vector<int> component(num_vertices(G));
                    int num = connected_components(G, &component[0]);
                    of << "," << num;
                } else {
                    of << ",0";
                }
            }

            of << '\n';
        }
    }

    of.close();
}