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

#include <string>
#include <thread>

#include "bidirSearch.h"
#include "config.h"
#include "posetObj.h"
#include "eventLog.h"
#include "posetMap.h"
#include "niceGraph.h"
#include "storageProfile.h"
#include "TimeProfile.h"
#include "stats.h"
#include "storeAndLoad.h"
#include "backwardSearch.h"
#include "forwardSearch.h"
#include "searchParams.h"
#include "utils.h"
#include "oldGenMap.h"

static std::chrono::steady_clock::time_point lastStats;

Search::Search() :
        profile(Section::OTHER),
        progress(0),
        layerState(),
        oldGenMap(),
        forwardC(0),
        backwardC(NCT::C),
        collided(false) {

}

void Search::run() {

    // initialize lastStats with current time
    lastStats = std::chrono::steady_clock::now();
    
    std::string search_alg;
    if (do_fw_search && do_bw_search) {
        search_alg = "bidirectional";
    } else if (do_fw_search) {
        search_alg = "forward";
    } else if (do_bw_search) {
        search_alg = "backward";
    } else {
        assert(false);
    }

    NCT::initThread();
    EventLog::write(false, "Starting " + search_alg + " search n = " + std::to_string(NCT::N) + ", C = " + std::to_string(NCT::C) + ", threads=" +
                           std::to_string(NCT::num_threads));

    std::vector<PosetMap> posetMapBW;
    LinExtT bwSearchLimit[NCT::C + 1];
    std::string result;

    if (do_bw_search) {
        EventLog::write(false, "BW Search poset storage directory: " + bw_storage_path);
        PosetStorage storageBw{bw_storage_path, reuse_bw};

        std::array<const StorageEntry *, MAXENDC> bwResults{};

        // efficiency limit
        double c0Efficiency = static_cast<double>(factorial(NCT::N)) / std::pow(2.0, NCT::C);
        double efficiencyLimitBW = std::min(1.0, c0Efficiency + effBandwidth);
        double efficiencyLimitBW2 = std::min(1.0, c0Efficiency + effBandwidth2);
        if (!do_fw_search) {
            fullLayers = NCT::C + 1;
        }
        if (fullLayers > NCT::C + 1) {
            fullLayers = NCT::C + 1;
        }
        LinExtT extLimitBaseBW = factorial(NCT::N) / efficiencyLimitBW;
        LinExtT extLimitBaseBW2 = factorial(NCT::N) / efficiencyLimitBW2;
        for (int c = 0; c <= NCT::C; c++) {
            bwSearchLimit[c] = (extLimitBaseBW >> c);
            if (c >= effBand2Thr) {
                bwSearchLimit[c] = (extLimitBaseBW2 >> c);
            }
            if (c >= NCT::C + 1 - fullLayers) {
                bwSearchLimit[c] = 1;
            }
        }
        EventLog::write(false, "BW Search Params:");
        EventLog::write(false, "  Start Efficiency (c=0): " + std::to_string(c0Efficiency));
        if (do_fw_search) {
            EventLog::write(false, "  Efficiency Bandwidth  : " + std::to_string(effBandwidth));
            EventLog::write(false, "  Efficiency Limit      : " + std::to_string(efficiencyLimitBW));
            EventLog::write(false, "  Full Layers           : " + std::to_string(fullLayers));
            if (effBand2Thr < NCT::C) {
                EventLog::write(false, "  Efficiency Limit2     : " + std::to_string(efficiencyLimitBW2));
                EventLog::write(false, "  Efficiency Band Thr2  : " + std::to_string(effBand2Thr));
            }
        }

        // BW search
        for (backwardC = NCT::C; backwardC >= 0; backwardC--) {
            const StorageEntry *entry = storageBw.getEntry(backwardC, bwSearchLimit[backwardC]);
            if (entry == nullptr) {
                if (backwardC == NCT::C) {
                    EventLog::write(true, "Creating initial poset for bw search");
                    createInitialPosetBW(storageBw);
                } else {
                    EventLog::write(true, "Backward step, parentC=" + std::to_string(backwardC));
                    LinExtT limitParents = bwSearchLimit[backwardC];
                    LinExtT limitChildren = bwSearchLimit[backwardC + 1];

                    profile.section(Section::BW_IO);
                    const auto &entry = *bwResults[backwardC + 1];
                    const auto &meta = entry.meta;
                    PosetMap childMapBW{meta.numUnf + meta.numYes};
                    entry.read(childMapBW);
                    std::vector<PosetObj> childList;
                    childList.reserve(childMapBW.countPosets());
                    childMapBW.fill(childList);

                    LinExtT minExt = limitParents > meta.getMaxLinExt() ? limitParents - meta.getMaxLinExt() : 1;
                    assert(meta.getMaxLinExt() <= (LinExtT(1) << (NCT::C - backwardC - 1)));
                    for (unsigned int c2 = backwardC + 2; c2 <= NCT::C; c2++) {
                        const auto &entry2 = *bwResults[c2];
                        const auto &meta2 = entry2.meta;
                        // Check if the layer has posets we need
                        if (meta2.getMaxLinExt() >= minExt) {
                            entry2.read(childMapBW, true);
                        }
                    }

                    doBackwardStep(profile, progress, storageBw, backwardC, limitParents, limitChildren, childList, childMapBW);

                    // print stats if more than 1 minute since last print
                    auto now = std::chrono::steady_clock::now();
                    auto diff = now - lastStats;
                    if (std::chrono::duration_cast<std::chrono::minutes>(diff).count() > 1) {
                        for (auto &line: Stats::detailed())
                            EventLog::write(true, line);
                        EventLog::write(true, "BW Profile:");
                        for (auto &line: StorageProfile::summary())
                            EventLog::write(true, line);
                        Stats::resetRecent();
                        lastStats = now;
                    }
                    EventLog::write(true, profile.summary());
                }
                entry = storageBw.getEntry(backwardC, bwSearchLimit[backwardC]);
                assert(entry != nullptr);
            } else {
                EventLog::write(true,
                                "Using existing bw search results for parentC=" + std::to_string(backwardC) + " from file " + entry->path.filename().string());
                StorageProfile::update(backwardC, {entry->meta.numUnf, entry->meta.numYes, 0, 0, 0, 0, 0, 0});
            }
            bwResults[backwardC] = entry;
            if (backwardC == 0) {
                break;
            }
        }

        // Results
        auto &meta = bwResults[0]->meta;
        if (meta.numYes + meta.numUnf > 1) {
            result = "Backward search result inconclusive!";
        } else if (meta.numYes == 1) {
            result = std::to_string(NCT::N) + " elements SORTABLE in " + std::to_string(NCT::C) + " comparisons";
        } else {
            result = std::to_string(NCT::N) + " elements NOT SORTABLE in " + std::to_string(NCT::C) + " comparisons";
        }

        if (do_fw_search) {
            // prepare hash maps
            posetMapBW.reserve(NCT::N + 1);
            EventLog::write(true, "Preparing hash maps with bw search results");
            profile.section(Section::BW_IO);
            for (int c = 0; c <= NCT::C; c++) {
                if (c < 1 || c > std::max(NCT::C - fullLayers + 1, 1u)) {
                    posetMapBW.emplace_back(1);
                } else {
                    const auto &entry = *bwResults[c];
                    const auto &meta = entry.meta;
                    auto &map = posetMapBW.emplace_back(meta.numUnf + meta.numYes);
                    for (int c2 = c; c2 <= NCT::C; c2++) {
                        const auto &entry2 = *bwResults[c2];
                        const auto &meta2 = entry2.meta;
                        if (meta2.maxLinExt[NCT::C] >= meta.completeAbove) {
                            entry2.read(map);
                        }
                    }
                }
            }
        }
    }

    if (do_fw_search) {
        uint64_t childPosetLimit = activePosetMemory / (sizeof(AnnotatedPosetObj) + sizeof(uint64_t) * 10) / 3;
        uint64_t childEdgeLimit = childPosetLimit * 9;
        uint64_t oldGenEntries = oldGenMemory / sizeof(uint16_t);

        // create storage
        if (std::filesystem::exists(scratchFast)) {
            std::filesystem::remove(scratchFast);
        }
        if (std::filesystem::exists(scratchMedium)) {
            std::filesystem::remove(scratchMedium);
        }
        boost::interprocess::managed_mapped_file mmap{boost::interprocess::open_or_create, scratchMedium.data(), activePosetMemory / 3 * (NCT::C + 2)};
        boost::interprocess::managed_mapped_file mmapFast{boost::interprocess::open_or_create, scratchFast.data(), oldGenEntries * (sizeof(PosetObj) + 1)};

        SemiOfflineVector<AnnotatedPosetObj> posetList{childPosetLimit * 3, childPosetLimit * NCT::C, mmap.get_segment_manager()};
        SemiOfflineVector<uint64_t> edgeList{childEdgeLimit * 3, childEdgeLimit * NCT::C, mmap.get_segment_manager()};
        layerState.resize(NCT::C + 1);
        oldGenMap.reserve(NCT::C + 1);
        uint64_t oldGenSmall = oldGenEntries / 100 / NCT::C;
        uint64_t oldGenMedium = oldGenSmall + (oldGenEntries / 100 * 49) / (NCT::C * 2 / 5 + 1);
        uint64_t oldGenBig = oldGenMedium + (oldGenEntries / 100 * 50) / (NCT::C * 2 / 5 / 4 + 1);
        unsigned oldGenMediumBegin = NCT::C * 2 / 5 + 3;
        unsigned oldGenMediumEnd = NCT::C * 4 / 5;
        if (NCT::N == 18) {
            oldGenMediumBegin = 30;
            oldGenMediumEnd = 40;
            oldGenSmall = oldGenEntries / 10000 / NCT::C;
            oldGenMedium = oldGenSmall + (oldGenEntries / 100 * 99) / (oldGenMediumEnd - oldGenMediumBegin);
        }
        for (int i = 0; i <= NCT::C; i++) {
            if (i < oldGenMediumBegin || i >= oldGenMediumEnd) {
                oldGenMap.emplace_back(mmapFast.get_segment_manager(), oldGenSmall);
            } else if ((i - oldGenMediumBegin) % 4 == 3 && NCT::N != 18) {
                oldGenMap.emplace_back(mmapFast.get_segment_manager(), oldGenBig);
            } else {
                oldGenMap.emplace_back(mmapFast.get_segment_manager(), oldGenMedium);
            }
        }
        std::vector<uint64_t> tempVec;
        tempVec.reserve(childPosetLimit + 100000);
        PosetMapExt childMap{posetList, childPosetLimit};

        if (!do_bw_search) {
            posetMapBW.reserve(NCT::N + 1);
            for (int c = 0; c <= NCT::C; c++) {
                posetMapBW.emplace_back(1);
            }
        }

        // create initial poset
        EventLog::write(true, "Creating initial poset");
        createInitialPosetFW(posetList, layerState[0]);


        collided = true;

        int steps = 0;

        while (true) {

            steps += 1;

            // do forward step
            LinExtT limit = LinExtT(1) << (NCT::C - forwardC - 1);
            EventLog::write(true, "Forward step, parentC=" + std::to_string(forwardC));
            LinExtT completeAbove;
            if (do_bw_search) {
                completeAbove = bwSearchLimit[forwardC + 1];
            } else {
                completeAbove = std::numeric_limits<LinExtT>::max();
            }
            PosetMap &childMapBW = posetMapBW[forwardC + 1];
            doForwardStep(posetList, edgeList, layerState[forwardC], layerState[forwardC + 1], forwardC, completeAbove, childMap, childMapBW,
                          oldGenMap[forwardC + 1],
                          oldGenMap[forwardC], limit, progress, profile, tempVec, childPosetLimit, childEdgeLimit);

            // check termination
            profile.section(Section::OTHER);
            if (forwardC == 0) {
                posetList.ensureOnlineFrom(0);
                auto status = posetList[0].GetStatus();
                if (status == SortableStatus::YES) {
                    result = std::to_string(NCT::N) + " elements SORTABLE in " + std::to_string(NCT::C) + " comparisons";
                    break;
                } else if (status == SortableStatus::NO) {
                    result = std::to_string(NCT::N) + " elements NOT SORTABLE in " + std::to_string(NCT::C) + " comparisons";
                    break;
                }
            }

            // print stats if more than 1 minute since last print
            auto now = std::chrono::steady_clock::now();
            auto diff = now - lastStats;
            if (std::chrono::duration_cast<std::chrono::minutes>(diff).count() > 1) {
                for (auto &line: Stats::detailed())
                    EventLog::write(true, line);
                EventLog::write(true, "Steps: " + std::to_string(steps));
                EventLog::write(true, "FW Profile Complete:");
                for (auto &line: getFwProfile())
                    EventLog::write(true, line);
                EventLog::write(true, "Old Gen Map Profile:");
                for (auto &line: getMapProfile())
                    EventLog::write(true, line);
                Stats::resetRecent();
                lastStats = now;
            }
            EventLog::write(true, profile.summary());
        }
        EventLog::write(false, "Steps: " + std::to_string(steps));
        EventLog::write(false, "FW Profile Complete:");
        for (auto &line: getFwProfile())
            EventLog::write(false, line);
        EventLog::write(false, "Old Gen Map Profile:");
        for (auto &line: getMapProfile())
            EventLog::write(false, line);
        oldGenMap.clear();
    }

    profile.section(Section::OTHER);

    EventLog::write(false, "Finished.");
    for (auto &line: Stats::detailed())
        EventLog::write(false, line);
    if (do_bw_search) {
        EventLog::write(false, "BW Profile:");
        for (auto &line: StorageProfile::summary())
            EventLog::write(false, line);
    }
    EventLog::write(false, profile.summary());
    EventLog::write(false, result);

    if (do_fw_search) {
        EventLog::write(false, "Removing temp files.");
        std::filesystem::remove(scratchFast);
        std::filesystem::remove(scratchMedium);
        EventLog::write(false, "Done.");
    }

    profile.section(Section::END);
}

std::vector<std::string> Search::getFwProfile() {
    uint64_t totalNum = 0;
    std::vector<std::string> result{};
    result.reserve(NCT::C + 1);
    for (int c = 0; c < oldGenMap.size(); c++) {
        std::stringstream outLine;
        auto &sprofile = oldGenMap[c].profile;
        totalNum += sprofile[SortableStatus::YES] + sprofile[SortableStatus::NO];
        outLine << "c = ";
        outLine.width(2);
        outLine << c;
        outLine << ":  " << std::left << std::setw(11) << (sprofile[SortableStatus::YES] + sprofile[SortableStatus::NO]);
        outLine << "\t YES:  " << std::left << std::setw(11) << sprofile[SortableStatus::YES];
        outLine << "\t NO: " << std::left << std::setw(11) << sprofile[SortableStatus::NO];
        result.push_back(outLine.str());
    }
    result.push_back("Total elements: " + std::to_string(totalNum));
    return result;
}

std::vector<std::string> Search::getMapProfile() {
    uint64_t totalNum = 0;
    std::vector<std::string> result{};
    result.reserve(NCT::C + 1);
    for (int c = 0; c < oldGenMap.size(); c++) {
        std::stringstream outLine;
        auto &sprofile = oldGenMap[c].profileStorage;
        totalNum += sprofile[SortableStatus::YES] + sprofile[SortableStatus::NO];
        outLine << "c = ";
        outLine.width(2);
        outLine << c;
        outLine << ": MAX: " << std::left << std::setw(11) << oldGenMap[c].size;
        outLine << "\t ALL: " << std::left << std::setw(11) << (sprofile[SortableStatus::YES] + sprofile[SortableStatus::NO]);
        outLine << "\t YES: " << std::left << std::setw(11) << sprofile[SortableStatus::YES];
        outLine << "\t NO: " << std::left << std::setw(11) << sprofile[SortableStatus::NO];
        result.push_back(outLine.str());
    }
    result.push_back("Total elements: " + std::to_string(totalNum));
    return result;
}