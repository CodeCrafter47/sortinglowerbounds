
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
#include <boost/program_options.hpp>

#include "posetPointer.h"
#include "TimeProfile.h"
#include "utils.h"
#include "eventLog.h"
#include "bidirSearch.h"
#include "tui.h"
#include "posetAnalyser.h"

namespace po = boost::program_options;

int main(int argc, char *argv[]) {

    std::string log_path;
    std::string bw_path;
    std::string mmap_fast;
    std::string mmap_slow;
    double activePosetMem;
    double oldPosetMem;
    double effBandwidth;
    unsigned int fullLayers;
    bool reuse_bw;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("interactive,i", "run in interactive mode - enables tui")
            ("forward-search,fw", "run forward search, non-interactive")
            ("backward-search,bw", "run backward search, non-interactive")
            ("bidir-search,bd", "run bidirectional search, non-interactive")

#ifdef VARIABLE_N
            ("num-elements,N", po::value<unsigned int>(&NCT::Nglob)->default_value(13), "set number of elements N")
#endif // VARIABLE_N
            ("num-comparisons,C", po::value<unsigned int>(&NCT::Cglob), "set number of comparisons C")
            ("threads,t", po::value<unsigned int>(&NCT::num_threads_glob)->default_value(std::thread::hardware_concurrency()), "set number of threads")

            ("eff-bandwidth", po::value<double>(&effBandwidth)->default_value(0.125), "set efficiency bandwidth for bidir search")
            ("full-layers", po::value<unsigned int>(&fullLayers)->default_value(10), "set full bw layers for bidir search")
            ("reuse-bw", po::value<bool>(&reuse_bw)->default_value(true), "set whether to reuse bw search results from previous runs")

            ("log-path", po::value<std::string>(&log_path)->default_value("./outputs"), "set directory for log files")
            ("bw-path", po::value<std::string>(&bw_path)->default_value("./storageBw"), "set directory for backward search storage")
            ("tempfile-fast", po::value<std::string>(&mmap_fast)->default_value("./temp_fast.mmap"), "fast temp storage file (ssd), fw search only")
            ("tempfile-slow", po::value<std::string>(&mmap_slow)->default_value("./temp_slow.mmap"), "slow temp storage file (hdd), fw search only")

            ("active-poset-mem", po::value<double>(&activePosetMem)->default_value(0.25), "Memory (RAM) for active posets in Gb")
            ("old-poset-mem", po::value<double>(&oldPosetMem)->default_value(0.25), "Memory (RAM) for old posets in Gb")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

#ifdef VARIABLE_N
    if (NCT::Nglob > MAXN) {
        std::cerr << "Maximum value for N is " << MAXN << "." << std::endl;
        return 1;
    }
#endif // VARIABLE_N

    NCT::initThread();

    if (vm.count("num-comparisons") == 0) {
        NCT::Cglob = NCT::cTableITLB[NCT::N];
    } else if (NCT::Cglob > MAXC) {
        std::cerr << "Maximum value for C is " << MAXC << "." << std::endl;
        return 1;
    }

    if (vm.count("interactive")) {
        if (isTuiSupported()) {
            tuiLoop(log_path);
            return 0;
        } else {
            std::cerr << "TUI is not supported by the system. Cannot run in interactive mode." << std::endl;
            return 1;
        }
    }

    //start total time measurement
    TimeProfile profile(Section::OTHER);

    NCT::initThread();

    std::ofstream outputStream( log_path + "/output" + std::to_string(NCT::N) + "__" + currentDateTime() + ".txt"); //output file
    std::ofstream outputStreamEvents( log_path + "/output" + std::to_string(NCT::N) + "__" + currentDateTime() + "_events.txt"); //output file

    EventLog::init(&outputStream, &outputStreamEvents);
    EventLog::writeCout = true;

    assert(NCT::N <= MAXN);

    Search search{};

    search.do_fw_search = true;
    search.do_bw_search = false;

    if (vm.count("forward-search")) {
        search.do_fw_search = true;
        search.do_bw_search = false;
    }

    if (vm.count("backward-search")) {
        search.do_fw_search = false;
        search.do_bw_search = true;
    }

    if (vm.count("bidir-search")) {
        search.do_fw_search = true;
        search.do_bw_search = true;
    }

    search.bw_storage_path = bw_path;

    search.scratchFast = mmap_fast;
    search.scratchMedium = mmap_slow;

    search.effBandwidth = effBandwidth;
    search.fullLayers = fullLayers;
    search.reuse_bw = reuse_bw;

    search.activePosetMemory = uint64_t(activePosetMem * 1024) << 20;
    search.oldGenMemory = uint64_t(oldPosetMem * 1024) << 20;

    search.run();
    //analysePosets(storage);

    EventLog::init(nullptr, nullptr);

    return 0;
}

