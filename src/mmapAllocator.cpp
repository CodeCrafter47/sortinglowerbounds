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


#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <cassert>

#include "eventLog.h"
#include "mmapAllocator.h"
#include "posetObj.h"

MmapAllocator::MmapAllocator() {
    std::lock_guard lck(allocatorMutex);
    allocateMmapFile(numTperFile);
}

MmapAllocator::~MmapAllocator() {
    std::lock_guard lck(allocatorMutex);
    assert(	mmapFileDescriptors.size() == mmapBasePointers.size() && mmapBasePointers.size() == numMmapFiles );
    for (int i = 0; i < mmapFileDescriptors.size() ; i++){
        EventLog::write(true, "file i: " + std::to_string(i) + "unmap and close file (id) " + std::to_string(mmapFileDescriptors[i]) + " name: " + mmapFileNames[i]);
        munmap(mmapBasePointers[i], mmapFileSizes[i]);
        close(mmapFileDescriptors[i]);
        std::remove(mmapFileNames[i].c_str());
    }
}

void MmapAllocator::returnMemory(PosetObj *ptr, uint32_t num) {
    std::lock_guard lck(allocatorMutex);
    assert(ptr != nullptr);
    assert(num > 0);
    freeMemoryList.emplace_back(ptr, num);
}

PosetObj *MmapAllocator::requestMemory(uint32_t sizeRequest) {
    std::lock_guard lck(allocatorMutex);
    assert(sizeRequest < numTperFile/4);

    for (auto freeMemIter = freeMemoryList.begin(); freeMemIter != freeMemoryList.end() ; freeMemIter++) {
        if (freeMemIter->size == sizeRequest) {
            PosetObj* result = freeMemIter->start;
            freeMemoryList.erase(freeMemIter);
            assert(result != nullptr);
            return result;
        }
    }


    if(currentStartIndex + sizeRequest > numTperFile){
        PosetObj* allocatePtr = (PosetObj*)allocateMmapFile(numTperFile);
        if (allocatePtr == nullptr)
            return nullptr;
    }

    assert(currentStartIndex + sizeRequest <= numTperFile);


    PosetObj* resultPtr = mmapBasePointers[numMmapFiles - 1] + currentStartIndex;

    currentStartIndex += sizeRequest;
//		std::cout<< "request ptr " << (uint64_t)resultPtr<< "size: "<< sizeRequest<<std::endl;
    return resultPtr;

}

std::string MmapAllocator::path = "./temp";

char *MmapAllocator::allocateMmapFile(unsigned int newSize) {

    std::string filenameS = path + "/mmapFile_" + std::to_string(numMmapFiles) + ".tmp";

    const char * filename = filenameS.c_str();

    const uint64_t numChars = newSize * sizeof(PosetObj);

    FILE *pFile = fopen (filename, "w");

    if (pFile == nullptr)
    {
        EventLog::write(true, "could not open file " + filenameS);
        return nullptr;
    }

    fseek ( pFile , numChars - 1 , SEEK_SET );

    char x = 's';

    fwrite ( &x , sizeof(char), 1, pFile);

    if (ferror (pFile))
    {
        EventLog::write(true, "error when writing file " + filenameS);
        return nullptr;
    }

    fclose (pFile);


    auto fd = open(filename, O_RDWR);
    if (fd == -1) {
        EventLog::write(true, "mmap file open error: " + filenameS);
        return nullptr;
    } else {
        EventLog::write(true, "mmap file open success: " + filenameS);
    }


    char* mmapPtr = (char*)mmap(nullptr, numChars, PROT_READ | PROT_WRITE, MAP_NORESERVE | MAP_SHARED, fd, 0);  //MAP_PRIVATE


    if(	mmapPtr == nullptr)
    {
        EventLog::write(true, "mmap returned nullptr: " + filenameS);
        return nullptr;
    }

    numMmapFiles++;
    mmapBasePointers.push_back((PosetObj*) mmapPtr);
    mmapFileDescriptors.push_back(fd);
    mmapFileNames.push_back(filenameS);
    mmapFileSizes.push_back(numChars);
    currentStartIndex = 0;
    return mmapPtr;
}
