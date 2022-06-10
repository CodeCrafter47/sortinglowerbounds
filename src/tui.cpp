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


#include "tui.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>
#include <filesystem>
#include <fstream>
#include <utility>

#include "config.h"
#include "posetObj.h"
#include "posetInfo.h"
#include "posetHandle.h"
#include "posetPointer.h"
#include "niceGraph.h"
#include "expandedPoset.h"
#include "TimeProfile.h"
#include "eventLog.h"
#include "utils.h"
#include "bidirSearch.h"
#include "stats.h"
#include "storageProfile.h"
#include "mmapAllocator.h"
#include "posetContainer.h"
#include "storeAndLoad.h"
#include "posetAnalyser.h"
#include "searchParams.h"

namespace {
    using namespace ftxui;

    struct {
        std::string log_path = "./outputs";
        std::string bw_path = "./storageBw";
        //std::string save_path = "./storage_" + std::to_string(NCT::N);
        //int poset_storage_selected = 0;
        std::string mmap_fast = "/scratch/usfs/sortingbounds/storage_fast.mmap";
        std::string mmap_slow = "/scratch_medium/usfs/sortingbounds/storage_slow.mmap";
        uint64_t ram_active = 1;
        uint64_t ram_old = 1;
        uint64_t fwSearchParentsInRam;
        int num_threads = 4;
        int search_alg_selected = 0;
        bool reuse_bw = true;
        //bool analyze = false;
        double effBandwidth = 0.1;
        double effBandwidth2 = 0.1;
        unsigned int effBW2Threshold = MAXC;
        unsigned int completeLayers = 10;
    } setupParams;

    struct {
        uint32_t batchSize;
        uint64_t bwSearchPosetLimit;
    } tweakParams;

    template<typename T>
    auto NumberInput(T *model, int64_t min, int64_t max) {
        auto str_model = std::make_shared<std::string>();
        auto error = std::make_shared<std::string>();
        *str_model = std::to_string(*model);
        InputOption verifier;
        verifier.on_change = [=] {
            try {
                size_t pos;
                int64_t num = std::stol(*str_model, &pos);
                if (pos != str_model->length()) {
                    *error = " invalid";
                } else if (num < min || num > max) {
                    *error = " out of range [" + std::to_string(min) + ";" + std::to_string(max) + "]";
                } else {
                    *error = "";
                    *model = static_cast<T>(num);
                }
            } catch (std::invalid_argument const &ex) {
                *error = " invalid";
            } catch (std::out_of_range const &ex) {
                *error = " out of range";
            }
        };
        verifier.on_enter = verifier.on_change;
        auto input = Input(&*str_model, "number", verifier);
        return Renderer(input, [=] {
            return hbox(
                    {
                            input->Render(),
                            text(*error) | color(Color::RedLight),
                    });
        });
    }

    auto NumberInputDouble(double *model, double min, double max) {
        auto str_model = std::make_shared<std::string>();
        auto error = std::make_shared<std::string>();
        *str_model = std::to_string(*model);
        InputOption verifier;
        verifier.on_change = [=] {
            try {
                size_t pos;
                double num = std::stod(*str_model, &pos);
                if (pos != str_model->length()) {
                    *error = " invalid";
                } else if (num < min || num > max) {
                    *error = " out of range [" + std::to_string(min) + ";" + std::to_string(max) + "]";
                } else {
                    *error = "";
                    *model = num;
                }
            } catch (std::invalid_argument const &ex) {
                *error = " invalid";
            } catch (std::out_of_range const &ex) {
                *error = " out of range";
            }
        };
        verifier.on_enter = verifier.on_change;
        auto input = Input(&*str_model, "number", verifier);
        return Renderer(input, [=] {
            return hbox(
                    {
                            input->Render(),
                            text(*error) | color(Color::RedLight),
                    });
        });
    }

    static std::string charset[11] = {
#if defined(FTXUI_MICROSOFT_TERMINAL_FALLBACK)
            // Microsoft's terminals often use fonts not handling the 8 unicode
    // characters for representing the whole gauge. Fallback with less.
    " ", " ", " ", " ", "▌", "▌", "▌", "█", "█", "█",
#else
            " ", " ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█",
#endif
            // An extra character in case when the fuzzer manage to have:
            // int(9 * (limit - limit_int) = 9
            "█"};

    class Gauge2 : public Node {
    public:
        Gauge2(float progress1, float progress2) :
                progress1(std::min(std::max(progress1, 0.f), 1.f)),
                progress2(std::min(std::max(progress2, 0.f), 1.f)) {}

        void ComputeRequirement() override {
            requirement_.flex_grow_x = 1;
            requirement_.flex_grow_y = 0;
            requirement_.flex_shrink_x = 1;
            requirement_.flex_shrink_y = 0;
            requirement_.min_x = 1;
            requirement_.min_y = 1;
        }

        void Render(Screen &screen) override {
            int y = box_.y_min;
            if (y > box_.y_max)
                return;

            int limit1_int = box_.x_min + progress1 * (box_.x_max - box_.x_min + 1);
            float limit2 = box_.x_min + progress2 * (box_.x_max - box_.x_min + 1);
            int limit2_int = limit2;
            int x = box_.x_min;
            while (x < limit1_int)
                screen.at(x++, y) = charset[9];
            while (x < limit2_int) {
                screen.PixelAt(x, y).dim = true;
                screen.at(x++, y) = charset[9];
            }
            screen.PixelAt(x, y).dim = true;
            screen.at(x++, y) = charset[int(9 * (limit2 - limit2_int))];
            while (x <= box_.x_max)
                screen.at(x++, y) = charset[0];
        }

    private:
        float progress1;
        float progress2;
    };

    Element gauge2(float progress1, float progress2) {
        return std::make_shared<Gauge2>(progress1, progress2);
    }

    auto progressTab(Search &search) {
        return Renderer([&] {
            Elements profile;
            profile.push_back(hbox(text("C    finished    parents   marked    p progress")));
            for (int c = 0; c < search.oldGenMap.size(); c++) {
                auto &sprofile = search.oldGenMap[c].profile;
                auto finished = sprofile[SortableStatus::YES] + sprofile[SortableStatus::NO];
                LayerState &state = search.layerState[c];
                auto parents = state.posetListEnd - state.posetListBegin;
                auto marked = state.parentsEnd - state.parentsBegin;
                auto marked_complete = state.parentsSliceBegin - state.parentsBegin;
                auto marked_active = state.parentsSliceEnd - state.parentsBegin;
                auto phase = state.phase;

                std::stringstream outLine;
                outLine.width(2);
                outLine << c;
                outLine << ":  " << std::left << std::setw(11) << (finished) << ' ';
                outLine << std::left << std::setw(9) << parents << ' ';
                outLine << std::left << std::setw(9) << marked << ' ';
                outLine << std::left << std::setw(1) << phase << ' ';
                Element text_line = text(outLine.str());
                if (c == search.forwardC) {
                    text_line = text_line | color(Color::GreenLight);
                }
                if (!search.collided && c == search.backwardC) {
                    text_line = text_line | color(Color::RedLight);
                }
                if (marked > 0 && c <= search.forwardC) {
                    profile.push_back(hbox(text_line,
                                           gauge2(static_cast<float>(marked_complete) / marked, static_cast<float>(marked_active) / marked)));
                } else {
                    profile.push_back(hbox(text_line));
                }
            }
            return vbox(profile);
        });
    }

    auto outputTab() {
        return Renderer([] {
            Elements children;
            for (auto &entry: EventLog::getHistory(25)) {
                children.push_back(text(entry) | flex_shrink);
            }
            return vbox(children);
        });
    }

    auto statsTab() {
        return Renderer([] {
            Elements stats;
            for (auto &line: Stats::detailed()) {
                stats.push_back(hbox(text(line)));
            }
            return vbox(stats);
        });
    }

    auto fwMapProfileTab(Search &search) {
        return Renderer([&] {
            Elements profile;
            for (auto line: search.getMapProfile()) {
                profile.push_back(hbox(text(line)));
            }
            return vbox(profile);
        });
    }

    auto fwProfileTab(Search &search) {
        return Renderer([&] {
            Elements profile;
            for (auto &line: search.getFwProfile()) {
                profile.push_back(hbox(text(line)));
            }
            return vbox(profile);
        });
    }

    auto profileTab() {
        return Renderer([] {
            Elements profile;
            for (auto &line: StorageProfile::summary()) {
                profile.push_back(hbox(text(line)));
            }
            return vbox(profile);
        });
    }

    auto tweaksTab() {
        tweakParams.batchSize = SearchParams::batchSize;
        tweakParams.bwSearchPosetLimit = SearchParams::bwSearchPosetLimit;

        auto input_batchSize = NumberInput<uint32_t>(&tweakParams.batchSize, 1, std::numeric_limits<int>::max());
        auto input_bwSearchPosetLimit = NumberInput<uint64_t>(&tweakParams.bwSearchPosetLimit, 0, std::numeric_limits<int64_t>::max());

        auto button_apply = Button("Apply", [=]() {
            SearchParams::batchSize = tweakParams.batchSize;
            SearchParams::bwSearchPosetLimit = tweakParams.bwSearchPosetLimit;

            EventLog::write(true, "Params changed:");
            EventLog::write(true, "batchSize = " + std::to_string(SearchParams::batchSize));
            EventLog::write(true, "fwSearchChildrenLimit = " + std::to_string(SearchParams::fwSearchChildrenLimit));
            EventLog::write(true, "fwSearchParentsInRam = " + std::to_string(SearchParams::fwSearchParentsInRam));
            EventLog::write(true, "bwSearchPosetLimit = " + std::to_string(SearchParams::bwSearchPosetLimit));
        });

        auto container = Container::Vertical(
                {
                        input_batchSize,
                        input_bwSearchPosetLimit,
                        button_apply,
                });

        return Renderer(container, [=]() {
            return vbox(
                    {
                            text("Current") | bold,
                            hbox(text("  batchSize            : "), text(std::to_string(SearchParams::batchSize))),
                            hbox(text("  bwSearchPosetLimit   : "), text(std::to_string(SearchParams::bwSearchPosetLimit))),
                            text("New") | bold,
                            hbox(text("  batchSize            : "), input_batchSize->Render()),
                            hbox(text("  bwSearchPosetLimit   : "), input_bwSearchPosetLimit->Render()),
                            button_apply->Render() | center,
                    });
        });
    }

    void startSearch() {

        NCT::initThread();

        const std::filesystem::path log_path = setupParams.log_path;
        if (!std::filesystem::exists(log_path)) {
            std::filesystem::create_directories(log_path);
        }

        std::ofstream outputStream(log_path / ("output" + std::to_string(NCT::N) + "__" + currentDateTime() + ".txt"));
        std::ofstream outputStreamEvents(log_path / ("output" + std::to_string(NCT::N) + "__" + currentDateTime() + "_events.txt"));
        EventLog::init(&outputStream, &outputStreamEvents);
        EventLog::writeCout = false;

        Search search{};

        search.scratchFast = setupParams.mmap_fast;
        search.scratchMedium = setupParams.mmap_slow;
        search.bw_storage_path = setupParams.bw_path;
        if (setupParams.search_alg_selected == 0) {
            search.do_fw_search = true;
            search.do_bw_search = false;
        } else if (setupParams.search_alg_selected == 1) {
            search.do_fw_search = false;
            search.do_bw_search = true;
        } else if (setupParams.search_alg_selected == 2) {
            search.do_fw_search = true;
            search.do_bw_search = true;
        }
        search.activePosetMemory = setupParams.ram_active << 30;
        search.oldGenMemory = setupParams.ram_old << 30;
        search.effBandwidth = setupParams.effBandwidth;
        search.fullLayers = setupParams.completeLayers;
        search.reuse_bw = setupParams.reuse_bw;
        search.effBandwidth2 = setupParams.effBandwidth2;
        search.effBand2Thr = setupParams.effBW2Threshold;

        std::thread searchThread{[&] {
            search.run();
        }};

        auto screen = ScreenInteractive::Fullscreen();

        std::thread updateThread{[&] {
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                screen.PostEvent(Event::Custom);
            }
        }};

        std::vector<std::string> tab_values{
                "Progress",
                "Output",
                "FW Profile",
                "Old Gen Map",
                "BW Profile",
                "Stats",
                //"Tweaks",
        };
        int tab_selected = 0;
        auto tab_toggle = Toggle(&tab_values, &tab_selected);

        auto tab_progress_renderer = progressTab(search);
        auto tab_output_renderer = outputTab();
        auto tab_map_profile_renderer = fwMapProfileTab(search);
        auto tab_fw_profile_renderer = fwProfileTab(search);
        auto tab_bw_profile_renderer = profileTab();
        auto tab_stats_renderer = statsTab();
        //auto tab_tweaks = tweaksTab();

        auto tab_container = Container::Tab(
                {
                        tab_progress_renderer,
                        tab_output_renderer,
                        tab_fw_profile_renderer,
                        tab_map_profile_renderer,
                        tab_bw_profile_renderer,
                        tab_stats_renderer,
                        //tab_tweaks,
                },
                &tab_selected);

        auto container = Container::Vertical({
                                                     tab_toggle,
                                                     tab_container,
                                             });

        //std::string algNames[] = {"Forward Search", "Backward Search", "Bidirectional Search"};

        auto renderer = Renderer(container, [&] {
            std::string algname = "Bidirectional Search";//algNames[setupParams.search_alg_selected];
            return vbox({
                                text(algname + " N=" + std::to_string(NCT::N) + " C=" + std::to_string(NCT::C)),
                                separatorHeavy(),
                                tab_toggle->Render(),
                                separator(),
                                tab_container->Render() | flex_grow,
                                separatorHeavy(),
                                text(search.profile.summary()),
                                gauge(search.progress),
                        }) | borderHeavy | yflex_grow;
        });

        screen.Loop(renderer);

        EventLog::init(nullptr, nullptr);
    }

    auto setupTab(ScreenInteractive &screen) {

        NCT::initThread();

        static const std::vector<std::string> poset_storage_options{
                "RAM",
                "Mmap"
        };
        static const std::vector<std::string> search_alg_options{
                "Forward search",
                "Backward search",
                "Bidirectional search"
        };

        auto input_log_path = Input(&setupParams.log_path, "log path");
        auto input_save_path = Input(&setupParams.bw_path, "save path");
        auto checkbox_resume = Checkbox("Reuse BW search results", &setupParams.reuse_bw);
//        auto radiobox_poset_storage = Radiobox(&poset_storage_options, &setupParams.poset_storage_selected);
        auto input_mmap_fast_path = Input(&setupParams.mmap_fast, "mmap path");
        auto input_mmap_slow_path = Input(&setupParams.mmap_slow, "mmap path");
        auto input_num_threads = NumberInput<unsigned int>(&NCT::num_threads_glob, 1, MAXTHREADS);
        auto input_ram_active = NumberInput<uint64_t>(&setupParams.ram_active, 1, 1000);
        auto input_ram_old = NumberInput<uint64_t>(&setupParams.ram_old, 1, 1000);

        auto input_eff_bandwidth = NumberInputDouble(&setupParams.effBandwidth, 0.0, 1.0);
        auto input_complete_layers = NumberInput<unsigned int>(&setupParams.completeLayers, 0, NCT::C);

        auto input_eff_bandwidth2 = NumberInputDouble(&setupParams.effBandwidth2, 0.0, 1.0);
        auto input_eff_band2_threshold = NumberInput<unsigned int>(&setupParams.effBW2Threshold, 0, MAXC);

        auto radiobox_search_alg = Radiobox(&search_alg_options, &setupParams.search_alg_selected);
//        auto checkbox_analyse = Checkbox("Analyse posets", &setupParams.analyze);
        auto button_start_search = Button("Start Search", [] {
            startSearch();
        });
        auto button_quit = Button("Quit", screen.ExitLoopClosure());

        auto button_row = Container::Horizontal(
                {
                        button_start_search,
                        button_quit,
                });

        auto tab_search_setup = Container::Vertical(
                {
                        input_log_path,
                        input_save_path,
                        checkbox_resume,

                        input_mmap_fast_path,
                        input_mmap_slow_path,

                        input_ram_active,
                        input_ram_old,

                        input_eff_bandwidth,
                        input_complete_layers,

                        input_eff_bandwidth2,
                        input_eff_band2_threshold,

                        input_num_threads,

                        radiobox_search_alg,
                        button_row,
                }
        );

        auto tab_search_setup_renderer = Renderer(tab_search_setup, [=] {
            return vbox({
                                hbox(text(" Log path      : "), input_log_path->Render()),
                                hbox(text(" Bw storage    : "), input_save_path->Render()),
                                hbox(text("                 "), checkbox_resume->Render()),
                                hbox(text(" Mmap fast     : "), input_mmap_fast_path->Render()),
                                hbox(text(" Mmap slow     : "), input_mmap_slow_path->Render()),
                                text(""),
                                hbox(text(" RAM active Gb : "), input_ram_active->Render()),
                                hbox(text(" RAM old Gb    : "), input_ram_old->Render()),
                                text(""),
                                hbox(text(" Eff Bandwidth : "), input_eff_bandwidth->Render()),
                                hbox(text(" Compl. Layers : "), input_complete_layers->Render()),
                                text(""),
                                hbox(text(" Eff Bandwidth2: "), input_eff_bandwidth2->Render()),
                                hbox(text(" Eff Band2 Thr : "), input_eff_band2_threshold->Render()),
                                text(""),
                                hbox(text(" Threads       : "), input_num_threads->Render()),
                                text(""),
                                hbox(text(" Algorithm     : "), radiobox_search_alg->Render()),
                                separator(),
                                text(" Check if N=" + (std::to_string(NCT::N)) + " elements can be sorted with C=" +
                                     (std::to_string(NCT::C)) + " comparisons."),
                                hbox({
                                             button_start_search->Render(),
                                             button_quit->Render()
                                     }) | center,
                        });
        });

        return tab_search_setup_renderer;
    }

    auto advancedParamsTab() {

        auto input_batchSize = NumberInput<uint32_t>(&SearchParams::batchSize, 1, std::numeric_limits<int>::max());
        auto input_fwSearchChildrenLimit = NumberInput<uint64_t>(&SearchParams::fwSearchChildrenLimit, 1, std::numeric_limits<int64_t>::max());
        auto input_fwSearchParentsInRam = NumberInput<uint64_t>(&SearchParams::fwSearchParentsInRam, 1, std::numeric_limits<int64_t>::max());
        auto input_bwSearchPosetLimit = NumberInput<uint64_t>(&SearchParams::bwSearchPosetLimit, 0, std::numeric_limits<int64_t>::max());
        return Renderer(Container::Vertical(
                                {
                                        input_batchSize,
                                        input_fwSearchChildrenLimit,
                                        input_fwSearchParentsInRam,
                                        input_bwSearchPosetLimit
                                }
                        ),
                        [=] {
                            return vbox(
                                    {
                                            hbox(text(" batchSize            : "), input_batchSize->Render()),
                                            hbox(text(" fwSearchChildrenLimit: "), input_fwSearchChildrenLimit->Render()),
                                            hbox(text(" fwSearchParentsInRam : "), input_fwSearchParentsInRam->Render()),
                                            hbox(text(" bwSearchPosetLimit   : "), input_bwSearchPosetLimit->Render()),
                                    });
                        });
    }

    auto infoTab() {
        return Renderer([] {
            auto sizes = vbox(
                    {
                            hbox(text(" sizeof(PosetObj)          : "), text(std::to_string(sizeof(PosetObj)))),
                            hbox(text(" sizeof(AnnotatedPosetObj) : "), text(std::to_string(sizeof(AnnotatedPosetObj)))),
                            //hbox(text(" sizeof(PosetPointer)      : "), text(std::to_string(sizeof(PosetPointer)))),
                            hbox(text(" sizeof(PosetInfo)         : "), text(std::to_string(sizeof(PosetInfo)))),
                            hbox(text(" sizeof(PosetInfoFull)     : "), text(std::to_string(sizeof(PosetInfoFull)))),
                            hbox(text(" sizeof(PosetHandle)       : "), text(std::to_string(sizeof(PosetHandle)))),
                            hbox(text(" sizeof(PosetHandleFull)   : "), text(std::to_string(sizeof(PosetHandleFull)))),
                            hbox(text(" sizeof(LinExtT)           : "), text(std::to_string(sizeof(LinExtT)))),
                            hbox(text(" sizeof(VertexList)        : "), text(std::to_string(sizeof(VertexList)))),
                            hbox(text(" sizeof(AdjacencyMatrix)   : "), text(std::to_string(sizeof(AdjacencyMatrix)))),
                            hbox(text(" sizeof(LayerStructure)    : "), text(std::to_string(sizeof(LayerStructure)))),
                            hbox(text(" sizeof(NiceGraph)         : "), text(std::to_string(sizeof(NiceGraph)))),
                            hbox(text(" sizeof(ExpandedPosetChild): "), text(std::to_string(sizeof(ExpandedPosetChild)))),
                    });
            auto bits = vbox(
                    {
                            hbox(text(" pointerHashWidth : "), text(std::to_string(pointerHashWidth))),
                            hbox(text(" numGraphBits     : "), text(std::to_string(PosetObjCore::numGraphBits))),
                            hbox(text(" statusWidth      : "), text(std::to_string(PosetObjCore::statusWidth))),
                    });
            return vbox(
                    {
                            sizes,
                            separator(),
                            bits,
                    });
        });
    }
}

void tuiLoop(std::string log_path) {

    setupParams.log_path = std::move(log_path);

    auto screen = ScreenInteractive::Fullscreen();

    std::vector<std::string> tab_values{
            "Search Setup",
            //"Advanced Settings",
            "Info",
    };
    int tab_selected = 0;
    auto tab_toggle = Toggle(&tab_values, &tab_selected);

    auto tab_search_setup_renderer = setupTab(screen);
    //auto tab_advanced_params_renderer = advancedParamsTab();
    auto tab_info_renderer = infoTab();

    auto tab_container = Container::Tab(
            {
                    tab_search_setup_renderer,
                    //tab_advanced_params_renderer,
                    tab_info_renderer,
            },
            &tab_selected);

    auto container = Container::Vertical({
                                                 tab_toggle,
                                                 tab_container,
                                         });

    auto renderer = Renderer(container, [&] {
        return vbox({
                            tab_toggle->Render(),
                            separatorHeavy(),
                            tab_container->Render(),
                    }) | borderHeavy | yflex_grow;
    });

    screen.Loop(renderer);
}

bool isTuiSupported() {
    return Dimension::Full().dimx > 0;
}