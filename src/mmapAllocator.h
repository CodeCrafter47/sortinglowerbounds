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

#ifndef MMAPALLOCATOR_H
#define MMAPALLOCATOR_H

#include <vector>
#include <string>
#include <mutex>
#include <list>

class PosetObj;

class MmapAllocator {
	static constexpr unsigned int numTperFile = (1 << 30);
public:
	static constexpr unsigned int standardRequestSize = (1 << 22);

    static std::string path;
private:
	unsigned int currentStartIndex = numTperFile;
		
	unsigned int numMmapFiles = 0;
	std::vector<PosetObj*> mmapBasePointers;
	std::vector<int> mmapFileDescriptors;
	std::vector<int> mmapFileSizes;
	std::vector<std::string> mmapFileNames;
    
	std::mutex allocatorMutex;


    struct FreeMemoryListItem {
        PosetObj* start;
        uint32_t size;
        FreeMemoryListItem(PosetObj* ptr, uint32_t num) : start(ptr), size(num) {}
        FreeMemoryListItem() : start(nullptr), size(0) {}
    };

    std::list<FreeMemoryListItem> freeMemoryList;

public:


    MmapAllocator();

    ~MmapAllocator();

    void returnMemory(PosetObj* ptr, uint32_t num);

	PosetObj* requestMemory(uint32_t sizeRequest);

private:

	char* allocateMmapFile(unsigned int newSize);
};



#endif