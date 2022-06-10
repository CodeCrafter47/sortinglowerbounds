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

#ifndef STORAGE_PROFILE_H
#define STORAGE_PROFILE_H

#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

#include "config.h"

struct StorageProfileC {
	std::array<uint64_t, 8> data;

	StorageProfileC();

	void updateCounts(std::array<uint64_t, 8> counts);

	void updateCountsDiff(std::array<uint64_t, 8> before, std::array<uint64_t, 8> after);

	uint64_t sum();
};


struct StorageProfileFull {
	std::array<StorageProfileC ,MAXENDC> profiles;

	StorageProfileFull() noexcept;

	void update(unsigned int c, std::array<uint64_t, 8> counts);

	void updateDiff(unsigned int c, std::array<uint64_t, 8> before, std::array<uint64_t, 8> after);

	std::vector<std::string> summary();

	size_t countInRange(unsigned int begin, unsigned int end);
};

struct StorageProfile {
	static StorageProfileFull profile;

	static void update(unsigned int c, std::array<uint64_t, 8> counts);

	static void updateDiff(unsigned int c, std::array<uint64_t, 8> before, std::array<uint64_t, 8> after);

	static std::vector<std::string> summary();

	static size_t countInRange(unsigned int begin, unsigned int end);

};

#endif