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

#ifndef SORTINGLOWERBOUNDS_UTILS_H
#define SORTINGLOWERBOUNDS_UTILS_H

#include <string> // std::string
#include "config.h" // LinExtT, NCT

/**
 * Compute the factorial of n.
 */
inline LinExtT factorial(unsigned int n) {
    LinExtT res = 1;
    for (unsigned int i = 2; i < n + 1; i++) {
        res = res * i;
    }
    return res;
}

/**
 * Compute the falling factorial from n down to stop (exclusive).
 */
inline LinExtT fallingfactorial(int n, int stop) {
    LinExtT res = 1;
    for (int i = n; i > stop; i--) {
        res = res * i;
    }
    return res;
}

/**
 * Checks whether the poset is sortable using the remaining number of comparisons and the number of linear extensions of the poset.
 *
 * This uses a result, that posets with at most seven linear extensions are sortable using ceil(log2(linExt)) comparisons.
 *
 * @return true is sortable, false if that cannot be determined using the above result.
 */
inline bool isEasilySortableLinExt(unsigned int cLeft, LinExtT linExt) {

    // if linExt is at most 7, then log2(linExt) is at most 3
    if (cLeft > 3)
        cLeft = 3;

    return (linExt <= 7 && linExt <= (1ULL << cLeft));
}

/**
 * Compute the number of remaining comparisons for a child of a poset on level c.
 * The child is on level c + 1.
 */
inline unsigned int remainingComparisonsChild(unsigned int c) {
    return NCT::C - c - 1;
}

/**
 * Get current date/time, format is YYYY_MM_DD__HH_mm_ss
 */
std::string currentDateTime();

#endif