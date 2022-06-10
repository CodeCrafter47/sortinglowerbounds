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


#include "storeAndLoad.h"

#include <fstream>
#include <thread>

#include "config.h"
#include "posetObj.h"
#include "linExtCalculator.h"

namespace {
    constexpr size_t bufferSize = 4096;
}

StorageEntry::StorageEntry(const Meta &meta, const std::filesystem::path &path): meta(meta), path(path) {

}

void StorageEntry::read(PosetMap &map, bool onlyYesIntances) const {
    if (onlyYesIntances && meta.numYes == 0) {
        return;
    }
    std::fstream fstream(path, std::ios::in | std::ios::binary);
    fstream.seekp(0, std::ios::end);
    size_t len = fstream.tellp();
    fstream.seekp(sizeof(Meta), std::ios::beg);
    size_t max = meta.numUnf + meta.numYes;
    assert(len == sizeof(Meta) + max * sizeof(PosetObj));

    PosetObj buffer[bufferSize];
    for (unsigned int i = 0; i < max; i+=bufferSize) {
        auto num = std::min(max - i, bufferSize);
        fstream.read((char *) buffer, sizeof(PosetObj) * num);
        for (int j = 0; j < num; j++) {
            auto &poset = buffer[j];
            poset.setMark(false);
            PosetInfo info = PosetInfo::fromPoset(poset);
            AnnotatedPosetObj aposet{poset, PosetInfoFull{info, poset.computeHash()}, 0};
            if (!onlyYesIntances || aposet.GetStatus() == SortableStatus::YES) {
                map.findAndInsert(aposet);
            }
        }
        assert(!fstream.eof());
        assert(!fstream.fail());
        assert(!fstream.bad());
    }
    fstream.close();
}

PosetStorage::PosetStorage(std::filesystem::path basePath, bool reuse) : basePath(basePath) {
    // create directory
    std::filesystem::create_directories(basePath);

    // scan files
    if (reuse) {
        for (const auto &entry: std::filesystem::directory_iterator(basePath)) {
            const auto filename = entry.path().filename().string();
            if (entry.is_regular_file()) {
                std::fstream fstream(entry.path(), std::ios::in | std::ios::binary);
                fstream.seekg(0, std::ios::beg);
                Meta meta;
                fstream.read(static_cast<char *>((void *) &meta), sizeof(Meta));
                fstream.close();
                entries.emplace_back(std::ref(meta), std::ref(entry.path()));
            }
        }
    }

    entries.reserve(entries.size() + NCT::C + 1);
}

PosetStorage::~PosetStorage() { }

void PosetStorage::storePosets(PosetMap &map, const Meta &meta) {
    auto path = basePath / ("n" + std::to_string(meta.n) + "c" + std::to_string(meta.c) + "_" + currentDateTime());
    std::fstream fstream(path, std::ios::out | std::ios::binary | std::ios::trunc);
    fstream.write((char *) &meta, sizeof(Meta));
    PosetObj buffer[bufferSize];
    uint32_t idxBuf = 0;
    for (auto& submap : map.SposetMap) {
        auto& container = submap.container;
        for (size_t i = 0; i < container.size(); i++) {
            auto& poset = container.get(i);
            buffer[idxBuf++] = poset;
            if (idxBuf == bufferSize) {
                fstream.write((char *) buffer, sizeof(PosetObj) * idxBuf);
                idxBuf = 0;
            }
        }
    }
    if (idxBuf > 0) {
        fstream.write((char *) buffer, sizeof(PosetObj) * idxBuf);
    }
    fstream.close();
    entries.emplace_back(std::ref(meta), std::ref(path));
}

const StorageEntry * PosetStorage::getEntry(unsigned int c, LinExtT limit) {
    for (const auto &entry : entries) {
        if (entry.meta.n == NCT::N && entry.meta.C == NCT::C && entry.meta.c == c && entry.meta.completeAbove == limit) {
            return &entry;
        }
    }
    return nullptr;
}

