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


#pragma once
#ifndef LINEXTCALCULATOR_H
#define LINEXTCALCULATOR_H

#include <queue>
#include "stats.h"
#include "utils.h"
#include "posetObj.h"
#include "posetHandle.h"

#ifndef LARGE_INT
#define BRANCHLESS
//#define FULL_TABLE_FILL  //not good on Plankton -- I guess it is better with AVX512
#endif // !LARGE_INT




//#define LINEEXT_CALC_DEBUG
//#define LINEEXT_CALC_OLD
struct alignas(8) UdSetItem32;
struct alignas(8) UdSetItemFull;

template< class UdSetItem, typename linExtTableType, bool fixN, bool AVX32bit>
class LinearExtensionCalculatorInternal;

class LinearExtensionCalculator {
	
	
	
#if defined(LINEEXT_CALC_OLD) || defined(LINEEXT_CALC_DEBUG)
    std::vector<LinExtT[3]> linExtTempMemory;
#endif



public:
    std::array<std::array<LinExtT, MAXN>, MAXN> linExtTable;
private:
    std::array<std::array<uint32_t, MAXN>, MAXN> linExtTable32;




#ifdef LINEEXT_CALC_DEBUG
    std::array<std::array<LinExtT, MAXN>, MAXN> linExtTable2;
#endif



	int C;

	LinearExtensionCalculatorInternal<UdSetItemFull,LinExtT,false, false>* internalCalcFull;
	LinearExtensionCalculatorInternal<UdSetItem32,uint32_t,true, true>* internalCalc32;


public:


    LinearExtensionCalculator(unsigned int N, unsigned int cc);

	~LinearExtensionCalculator();



#if defined(LINEEXT_CALC_OLD) || defined(LINEEXT_CALC_DEBUG)


    LinExtT calculateLinExtensions(PosetHandle & poset, int n) {
#ifdef LINEEXT_CALC_DEBUG
        std::array<std::array<LinExtT, MAXN>, MAXN>& t = linExtTable2;
#else
        std::array<std::array<LinExtT, MAXN>, MAXN> & t = linExtTable;
#endif
        std::vector<LinExtT[3]> & table = linExtTempMemory; //1 << n == 2**n
        typedef uint32_t BitS;

        for (int i = 0; i < MAXN; i++) {
            for (int j = 0; j < MAXN; j++) {
                t[i][j] = 0;
            }
        }

        std::queue<BitS> que; //binary numbers representing sets
        const BitS set0 = 0; //empty set
        BitS set1_p = 0;

        BitS i_mask = 1;
        for (int i = 0; i<n; i++, i_mask <<= 1) {
            set1_p |= i_mask;
        }

        const BitS set1 = set1_p; //set1 is all ones

        table[0][0] = 1; // = {1, 0, 0};
        table[set1][1] = 1; // = {0, 1, 0};

        que.push(set0);

        BitS inVertexMask[MAXN];
        BitS outVertexMask[MAXN];


        for (int i = 0; i<n; i++) {
            inVertexMask[i] = 0;
            for (int j = n - 1; j >= 0; j--) {
                inVertexMask[i] <<= 1;
                if (poset->isEdge(j, i))
                    inVertexMask[i] |= 1;
            }
        }

        for (int i = 0; i<n; i++) {
            outVertexMask[i] = 0;
            for (int j = n - 1; j >= 0; j--) {
                outVertexMask[i] <<= 1;
                if (poset->isEdge(i, j))
                    outVertexMask[i] |= 1;
            }
        }


        uint64_t visit = 1; //timestamp
        table[set0][2] = visit;

        while (!que.empty()) {
            BitS bstring = que.front();
            que.pop();
            BitS i_mask = 1;
            for (int i = 0; i<n; i++, i_mask <<= 1) {
                //check if nodes (i) not in set can be added to set (starting with lowest node number)
                if (!(bstring & i_mask)) {

                    if ((~bstring) & inVertexMask[i]) {//if not_downset
                        continue;
                    }
                    else {
                        BitS idx = bstring | i_mask;

                        if (table[idx][2] < visit) {
                            que.push(idx);
                            table[idx][2] = visit;
                            //addition of values
                            table[idx][0] = table[bstring][0];
                        }
                        else
                        {
                            //addition of values
                            table[idx][0] += table[bstring][0];
                        }
                    }
                }
            }
        }
        visit = 0;
        que.push(set1);
        table[set1][2] = visit;

        while (!que.empty()) {
            BitS bstring = que.front();
            que.pop();

            BitS j_mask = 1;
            for (int j = 0; j<n; j++, j_mask <<= 1) {
                //check if nodes (j) in the set can be removed from set
                if (bstring & j_mask) {
                    int node = j; //current node

                    if (bstring & outVertexMask[j]) {//if not_downset
                        continue;
                    }
                    else {
                        BitS idx = bstring & (~j_mask) & set1;

                        if (table[idx][2] > visit) {
                            que.push(idx);
                            table[idx][2] = visit;
                            //addition of values
                            table[idx][1] = table[bstring][1];
                        }
                        else {
                            //addition of values
                            table[idx][1] += table[bstring][1];
                        }

                        //calculation of t[j,k]
                        LinExtT d_v = table[idx][0];
                        LinExtT u_w = table[bstring][1];
                        LinExtT product = d_v * u_w;



#ifdef BRANCHLESS
                        BitS bstringShift = ~(bstring >> (node + 1));
                        for (int k = node + 1; k<n; k++, bstringShift >>= 1) { //only fill upper triangle of t
                                                                               //checks if u_k not in W
                            t[node][k] += product & (-((int64_t)bstringShift & 1));
                        }


#else					
                        BitS k_mask = 1 << (node + 1);
                        for (int k = node + 1; k<n; k++, k_mask <<= 1) { //only fill upper triangle of t
                                                                         //checks if u_k not in W
                            if (!(bstring & k_mask)) {
                                t[node][k] += product;
                            }
                        }
#endif				

                    }
                }
            }
        }
        LinExtT e_p = table[set1][0];
        for (int i = 1; i < n; i++) { //calculate lower triangle
            for (int j = 0; j<i; j++) {
                t[i][j] = e_p - t[j][i];
            }
        }

         return e_p;

    }

#endif

    /**
    * Calculates the number of linear extensions on a graph, by reducing it to contain 2 singletons at max
    * and computes for the the missing values for t[j,k] in a faster way.
    * Uses calculateLinExtensions() for the reduced graph.
    *
    * @return e_p Number of linear extensions
    */
	LinExtT calculateLinExtensionsSingleton(PosetHandle &poset, unsigned int c, bool fillTable, bool overflowCheck);

};




#endif // EXPANDEDPOSET_H
