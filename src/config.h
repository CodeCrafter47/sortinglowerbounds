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

#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>


#ifdef DEBUG_ASSERTIONS
#include <cassert>
#define DEBUG_ASSERT(assertion) assert(assertion)
#else
#define DEBUG_ASSERT(assertion)
#endif

#if NUMEL <= 20
typedef uint64_t LinExtT;
typedef int64_t LinExtTSigned;
#elif NUMEL <= 33
typedef unsigned __int128 LinExtT;
typedef __int128 LinExtTSigned;
#else
#include <boost/multiprecision/cpp_int.hpp>
typedef boost::multiprecision::int256_t LinExtT;
typedef boost::multiprecision::int256_t LinExtTSigned;
#endif

#if NUMEL <= 32
typedef uint32_t BitS;
typedef int32_t BitSSigned;
#else
typedef uint64_t BitS;
typedef int64_t BitSSigned;
#endif


constexpr unsigned int maxThreadBits = 6;

constexpr unsigned int MAXTHREADS = (1UL << maxThreadBits);


constexpr unsigned int pointerHashWidth = 8; //ausprobieren wie viele gut sind...
constexpr uint64_t pointerHashMax = (1UL << pointerHashWidth) - 1;
constexpr uint64_t pointerHashMask = pointerHashMax;


struct NCT {
	// information theoretic lower bound
	static constexpr unsigned int cTableITLB[] = {
			0, 0, 1, 3, 5, 7, 10, 13, 16, 19,
			22, 26, 29, 33, 37, 41, 45, 49, 53, 57,
			62, 66, 70, 75, 80, 84, 89, 94, 98, 103,
			108, 113, 118, 123, 128, 133, 139, 144, 149, 154,
			160, 165, 170, 176, 181, 187, 192, 198};

	// comparisons required by the Ford-Johnson Algorithm
	static constexpr unsigned int cTableFJA[] = {
			0, 0, 1, 3, 5, 7, 10, 13, 16, 19,
			22, 26, 30, 34, 38, 42, 46, 50, 54, 58,
			62, 66, 71, 76, 81, 86, 91, 96, 101, 106,
			111, 116, 121, 126, 131, 136, 141, 146, 151, 156,
			161, 166, 171, 177, 183, 189, 195, 201};

#ifdef VARIABLE_N
	static thread_local unsigned int N;
	static unsigned int Nglob;
#else
	static constexpr unsigned int N = NUMEL;
#endif

	static thread_local unsigned int C;
	static unsigned int Cglob;

	static thread_local unsigned int num_threads;
	static unsigned int num_threads_glob;

	static void initThread() {
#ifdef VARIABLE_N
		N = Nglob;
#endif
		C = Cglob;
		num_threads = num_threads_glob;
	}
};

constexpr unsigned int  MAXN = NUMEL;
constexpr unsigned int  MAXC = NCT::cTableFJA[NUMEL];
constexpr unsigned int  MAXENDC = MAXC + 1;

static_assert(MAXN <= 32);


#define PRIME1 3835324147ULL
#define PRIME2 2662418543ULL
#define PRIME3 3672298121ULL
#define MULT1 2232306541ULL
#define MULT2 1267922251ULL
#define MULT3 2864081526ULL

#endif


