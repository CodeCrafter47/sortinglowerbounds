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


#ifndef SEMIOFFLINEVECTOR_H
#define SEMIOFFLINEVECTOR_H

#include <vector>
#include <atomic>
#include <cassert>
#include <mutex>
#include <cstdlib>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>


template<class T>
class SemiOfflineVector {

private:
    const size_t onlineCapacity;
    const size_t offlineCapacity;
    boost::interprocess::managed_mapped_file::segment_manager *const segment_manager;

    T* onlineVec;
    T* offlineVec;

    size_t required_online = 0;
    std::atomic<size_t> sizeTotal{};
    std::atomic<size_t> sizeOffline{};

    std::mutex lock;
public:
    SemiOfflineVector(size_t onlineCapacity, size_t offlineCapacity, boost::interprocess::managed_mapped_file::segment_manager *segment_manager) : onlineCapacity(onlineCapacity),
                                                                                                          offlineCapacity(offlineCapacity),
                                                                                                          segment_manager(segment_manager),
                                                                                                          lock(),
                                                                                                          sizeOffline(0),
                                                                                                          sizeTotal(0) {
        onlineVec = static_cast<T*>(malloc(onlineCapacity * sizeof(T)));
        offlineVec = static_cast<T*>(segment_manager->allocate(offlineCapacity * sizeof(T)));
    }

    ~SemiOfflineVector() {
        free(onlineVec);
        segment_manager->deallocate(offlineVec);
    }

    [[nodiscard]] size_t size() const {
        return sizeTotal;
    }

    size_t insert(const T &element) {
        size_t pos = sizeTotal.fetch_add(1);
        if (pos >= onlineCapacity + sizeOffline) {
            std::lock_guard guard(lock);
            if (pos >= onlineCapacity + sizeOffline) {
                assert(false);
                ensureOnlineAvailable(1024);
            }
        }
        size_t onlinePos = pos % onlineCapacity;
        onlineVec[onlinePos] = element;
        return pos;
    }

    template<class iter>
    size_t insert(iter begin, iter end) {
        auto len = end - begin;
        size_t pos = sizeTotal.fetch_add(len);
        if (pos >= onlineCapacity + sizeOffline) {
            std::lock_guard guard(lock);
            assert(false);
            while (pos >= onlineCapacity + sizeOffline) {
                ensureOnlineAvailable(1024);
            }
        }
        for (int i = 0; i < len; i++) {
            size_t onlinePos = (pos + i) % onlineCapacity;
            onlineVec[onlinePos] = *(begin + i);
        }
        return pos;
    }

    T &operator[](size_t pos) {
        assert(pos >= sizeOffline);
        assert(pos < onlineCapacity + sizeOffline);
        size_t onlinePos = pos % onlineCapacity;
        return onlineVec[onlinePos];
    }

    T &get(size_t pos) {
        return this->operator[](pos);
    }

    void resize(size_t new_size) {
        if(new_size < sizeOffline) {
            sizeTotal = new_size;
            sizeOffline = new_size;
        }
        if(new_size > sizeOffline + onlineCapacity) {
            ensureOnlineAvailable(new_size - sizeTotal);
        }
        sizeTotal = new_size;
    }

    void ensureOnlineAvailable(size_t requiredAvailable) {
        if (onlineCapacity - (sizeTotal - sizeOffline) < requiredAvailable) {
            size_t count = requiredAvailable - (onlineCapacity - (sizeTotal - sizeOffline));
            size_t end = sizeOffline + count;
            assert(end <= required_online);
            assert(offlineCapacity >= end);
            for (size_t i = sizeOffline; i < end; i++) {
                offlineVec[i] = this->operator[](i);
            }
            sizeOffline = end;
        }
    }

    void ensureOnlineFrom(size_t begin) {
        if (begin < sizeOffline) {
            assert(onlineCapacity >= sizeTotal - begin);
            size_t end = sizeOffline;
            sizeOffline = begin;
            for (size_t i = begin; i < end; i++) {
                this->operator[](i) = offlineVec[i];
            }
        }
        required_online = begin;
    }
};

#endif //SEMIOFFLINEVECTOR_H
