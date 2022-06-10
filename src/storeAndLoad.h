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

#ifndef STORELOAD_H
#define STORELOAD_H

#include <filesystem>
#include <vector>
#include "posetMap.h"

class AnnotatedPosetObj;

struct Meta {
	unsigned int n;
	unsigned int c;
    unsigned int C;
	LinExtT completeAbove;
	std::array<LinExtT, MAXENDC> maxLinExt;
	size_t numYes;
	size_t numUnf;

	[[nodiscard]] LinExtT getMaxLinExt() const {
		return *std::max_element(std::cbegin(maxLinExt), std::cbegin(maxLinExt) + NCT::C + 1);
	}
};

class StorageEntry {

public:
	const std::filesystem::path path;
	const Meta meta;

	StorageEntry(const Meta &meta, const std::filesystem::path &path);

	void read(PosetMap &map, bool onlyYesIntances = false) const;
};

class PosetStorage {

private:
	std::filesystem::path basePath;
	std::vector<StorageEntry> entries;

public:

	explicit PosetStorage(std::filesystem::path basePath, bool reuse);
	~PosetStorage();

	void storePosets(PosetMap &map, const Meta &meta);

    const StorageEntry * getEntry(unsigned int c, LinExtT limit);
};

#endif

