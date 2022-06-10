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

#include "linExtCalculator.h"

#include <x86intrin.h>
#include <iostream>

#ifdef __AVX2__
inline __m256i get_mask64(const uint32_t mask) {
	  __m256i vmask(_mm256_set1_epi64x(mask));
	  const __m256i bit_mask(_mm256_setr_epi64x(0xfffffffffffffffe,0xfffffffffffffffd,0xfffffffffffffffb,0xfffffffffffffff7));
	  vmask = _mm256_or_si256(vmask, bit_mask);
	  return _mm256_cmpeq_epi64(vmask, _mm256_set1_epi64x(-1));
}


inline __m256i get_mask32(const uint32_t mask) {
	  __m256i vmask(_mm256_set1_epi32(mask));
	  const __m256i bit_mask(_mm256_setr_epi32(0xfffffffe,0xfffffffd,0xfffffffb,0xfffffff7,0xffffffef,0xffffffdf,0xffffffbf,0xffffff7f));
	  vmask = _mm256_or_si256(vmask, bit_mask);
	  return _mm256_cmpeq_epi32(vmask, _mm256_set1_epi32(-1));
}
#endif

#if NUMEL < 25
#define LINEXT_TABLE_EXP 1
#else
#define LINEXT_TABLE_EXP 0
#endif

struct alignas(8) UdSetItem32 {
    typedef uint32_t ValueType;
    typedef int32_t SignedType;
#if LINEXT_TABLE_EXP == 0
    BitS set;
#endif
    ValueType downVal;
    ValueType upVal;
};

struct alignas(8) UdSetItemFull {
    typedef LinExtT ValueType;
    typedef LinExtTSigned SignedType;
#if LINEXT_TABLE_EXP == 0
    BitS set;
#endif
    ValueType downVal;
    ValueType upVal;
};


template< class UdSetItem, typename linExtTableType, bool fixN, bool AVX32bit>
class LinearExtensionCalculatorInternal {


#if LINEXT_TABLE_EXP
    UdSetItem* udSetVectorVal;
    BitS* udSetVectorSet;
#else
    UdSetItem* udSetVector;
#endif
    std::array<std::array<linExtTableType, MAXN>, MAXN> & linExtTable;

public:
	uint64_t allocatedMemorySize = 0;


    LinearExtensionCalculatorInternal(int N, std::array<std::array<linExtTableType, MAXN>, MAXN> & linETab ): linExtTable(linETab){}

#if LINEXT_TABLE_EXP
    void setPointer(void* pointer, void* pointer2)
    {
        udSetVectorVal = (UdSetItem*) pointer;
        udSetVectorSet = (BitS*) pointer2;
    }

    void* getPointer() {
        return udSetVectorVal;
    }

    void* getPointer2() {
        return udSetVectorSet;
    }
#else
    void setPointer(void* pointer)
    {
        udSetVector = (UdSetItem*) pointer;
    }

    void* getPointer() {
        return udSetVector;
    }
#endif

    template <bool overflowCheck>
    LinExtT calculateLinExtensionsNew(PosetObj& poset, bool fillTable, unsigned int nn = 0) {
        std::array<std::array<linExtTableType, MAXN>, MAXN>& t = linExtTable;
        const unsigned int n = fixN ? NCT::N : nn;
        if  constexpr (fixN)
            assert(nn == 0);

        for (int i = 0; i < MAXN; i++) {
            for (int j = 0; j < MAXN; j++) {
                t[i][j] = 0;
            }
        }

        BitS inVertexMask[MAXN];


        for (int i = 0; i < n; i++) {
            inVertexMask[i] = 0;
            for (int j = n - 1; j >= 0; j--) {
                inVertexMask[i] <<= 1;
                int edge = poset.isEdge(j, i);
                inVertexMask[i] |= edge;
            }
        }

        // compute out vertex mask
        BitS outVertexMask[MAXN];

        for (int i = 0; i < n; i++) {
            outVertexMask[i] = 0;
            for (int j = n - 1; j >= 0; j--) {
                outVertexMask[i] <<= 1;
                int edge = poset.isEdge(i, j);
                outVertexMask[i] |= edge;
            }
        }


        const BitS set1 = (BitS(1) << n) - 1; //set1 is all ones
        const BitS set0 = 0; //empty set






        //fill downset vector
#if LINEXT_TABLE_EXP
        udSetVectorSet[0] = set0;
        udSetVectorVal[set0].downVal = 1;
#else
        int curReadIndex[MAXN];
		curReadIndex[0] = 0;

        udSetVector[0].downVal = 1;
        udSetVector[0].set = set0;
#endif
        int lastEnd = 1;
        BitS endNode_mask = 1;
        int writeIndex = 1;
        for (int endNode = 0; endNode < n; endNode++, endNode_mask <<= 1) {
			if(lastEnd >= allocatedMemorySize/2)
			{
				std::cout << "lastEnd: " << lastEnd << ", allocatedMemorySize: " << allocatedMemorySize << std::endl;
				assert(false);
			}

            if constexpr (overflowCheck) {
                constexpr uint32_t limit = std::numeric_limits<uint32_t>::max() / MAXN;
#if LINEXT_TABLE_EXP
                BitS downSet = udSetVectorSet[lastEnd - 1];
                if (udSetVectorVal[downSet].downVal > limit) {
#else
                if (udSetVector[lastEnd - 1].downVal > limit) {
#endif
                    return 0;
                }
            }
			
            for (int j = 0; j < lastEnd; j++) {
#if LINEXT_TABLE_EXP
                if ((udSetVectorSet[j] | inVertexMask[endNode]) == udSetVectorSet[j]) {//if downset
                    udSetVectorSet[writeIndex] = udSetVectorSet[j] | endNode_mask;
                    BitS curSet = udSetVectorSet[writeIndex];
                    udSetVectorVal[curSet].downVal = udSetVectorVal[udSetVectorSet[j]].downVal;
#else
                if ((udSetVector[j].set | inVertexMask[endNode]) == udSetVector[j].set) {//if not_downset
                    udSetVector[writeIndex].set = udSetVector[j].set | endNode_mask;
                    udSetVector[writeIndex].downVal = udSetVector[j].downVal;
                    BitS curSet = udSetVector[writeIndex].set;
#endif

                    int i = __builtin_ctz(curSet);
                    BitS curSetShift = curSet >> (i+1);
                    while(curSetShift) {

//						std::cout<< i <<", "<< curSet <<std::endl;
                        BitS preCurSet = curSet & (~(BitS(1) << i));
                        assert(preCurSet < curSet);

                        if (!(preCurSet & outVertexMask[i])) {
#if LINEXT_TABLE_EXP
                            udSetVectorVal[curSet].downVal += udSetVectorVal[preCurSet].downVal;
#else
                            int readIndex = curReadIndex[i];

                            while (udSetVector[readIndex].set < preCurSet)
                                readIndex++;

                            udSetVector[writeIndex].downVal += udSetVector[readIndex].downVal;
                            readIndex++;

                            curReadIndex[i] = readIndex;
#endif
                        }

                        int shift = __builtin_ctz(curSetShift) + 1;
                        curSetShift >>= shift;
                        i += shift;
                    }

                    writeIndex++;
                }
            }
            lastEnd = writeIndex;

#if LINEXT_TABLE_EXP == 0
			for (int i = 0; i <= endNode; i++)
				curReadIndex[i] = lastEnd - 1;
#endif
        }

        int numsets = lastEnd;
        int lastSet = numsets - 1;
#if LINEXT_TABLE_EXP
        assert(udSetVectorSet[lastSet] == set1);
#else
        assert(udSetVector[lastSet].set == set1);
#endif

        Stats::addVal<AVMSTAT::NDownSets>(numsets);

        if (!fillTable) {
#if LINEXT_TABLE_EXP
            return udSetVectorVal[set1].downVal;
#else
            return udSetVector[lastSet].downVal;
#endif
        }



#if LINEXT_TABLE_EXP == 0
        for (int i = 0; i < MAXN; i++)
            curReadIndex[i] = lastSet;
#endif

#if LINEXT_TABLE_EXP
        udSetVectorVal[set1].upVal = 1;
#else
        udSetVector[lastSet].upVal = 1;
#endif
        for (int curWriteIndex = lastSet - 1; curWriteIndex >=0 ; curWriteIndex--) {
#if LINEXT_TABLE_EXP
            BitS curSet = udSetVectorSet[curWriteIndex];
            udSetVectorVal[curSet].upVal = 0;
#else
            udSetVector[curWriteIndex].upVal = 0;
            BitS curSet = udSetVector[curWriteIndex].set;
#endif

			BitS curSetShift = (~curSet) & set1;
			int i = -1;
			while(curSetShift) {
				int shift = __builtin_ctz(curSetShift);
				curSetShift >>= shift+1;
				i += shift + 1;
				BitS preCurSet = curSet | (BitS(1)<<i);


#if LINEXT_TABLE_EXP
                if ((curSet | inVertexMask[i]) == curSet) {//if preCurSet is down set
                    udSetVectorVal[curSet].upVal += udSetVectorVal[preCurSet].upVal;

                    //calculation of t[j,k]
                    typename UdSetItem::ValueType d_v = udSetVectorVal[curSet].downVal; //   table[idx][0];
                    typename UdSetItem::ValueType u_w = udSetVectorVal[preCurSet].upVal;
                    typename UdSetItem::ValueType product = d_v * u_w;
#else
				int readIndex = curReadIndex[i];
				while (udSetVector[readIndex].set > preCurSet)
					readIndex--;
				if (udSetVector[readIndex].set == preCurSet) {
					udSetVector[curWriteIndex].upVal += udSetVector[readIndex].upVal;

					//calculation of t[j,k]
					typename UdSetItem::ValueType d_v = udSetVector[curWriteIndex].downVal; //   table[idx][0];
					typename UdSetItem::ValueType u_w = udSetVector[readIndex].upVal; //table[bstring][1];
					typename UdSetItem::ValueType product = d_v * u_w;

					readIndex--;
					curReadIndex[i] =  readIndex;
#endif

#ifdef __AVX2__
					if constexpr(AVX32bit){

						BitS bstringShift = (~preCurSet) & set1;

						constexpr unsigned int fullRounds = (MAXN - 1)   / 8;
						constexpr unsigned int remainder = MAXN - 8* fullRounds;
						constexpr unsigned int rightShift = 8 - (MAXN - 8* fullRounds);

						bstringShift <<= rightShift;


						const unsigned int jStart = (i+1 + rightShift)/8;

						const __m256i productBroadcast = _mm256_set1_epi32(product);

						bstringShift >>= jStart*8;

                        //last round
						const int startIndex = MAXN - 8*( fullRounds + 1 - jStart);

						__m256i maskTest =    get_mask32(bstringShift);
						__m256i v1 =  _mm256_maskload_epi32((int*)&(t[i][startIndex]), maskTest);
						__m256i sum =  _mm256_add_epi32(v1, productBroadcast);
						_mm256_maskstore_epi32((int*)&(t[i][startIndex]), maskTest, sum);



						for(unsigned int j = startIndex + 8; j < MAXN  ; j+=8)
						{
							bstringShift >>= 8;
							__m256i v1 =  _mm256_loadu_si256((__m256i*)&(t[i][j]));
							__m256i maskTest =    get_mask32(bstringShift);
							__m256i sum =  _mm256_add_epi32(v1, productBroadcast);
							_mm256_maskstore_epi32((int*)&(t[i][j]), maskTest, sum);
						}
					} else {
#else
                    {
#endif

#ifdef BRANCHLESS
						BitS bstringShift = ~(preCurSet >> (i + 1));
						for (int k = i + 1; k < n; k++, bstringShift >>= 1) { //only fill upper triangle of t
																			   //checks if u_k not in W
							t[i][k] += product & (-((typename UdSetItem::SignedType)bstringShift & 1));
						}
#else
						BitS k_mask = BitS(1) << (i + 1);
						for (int k = i + 1; k < n; k++, k_mask <<= 1) { //only fill upper triangle of t
																		 //checks if u_k not in W
							if (!(preCurSet & k_mask)) {
								t[i][k] += product;
							}
						}
#endif
					}

                }

            }
        }

#if LINEXT_TABLE_EXP
        LinExtT e_p = udSetVectorVal[set1].downVal;
#else
        LinExtT e_p = udSetVector[lastSet].downVal;
#endif
//#ifndef FULL_TABLE_FILL
//        for (int i = 1; i < n; i++) { //calculate lower triangle
//            for (int j = 0; j < i; j++) {
//                t[i][j] = e_p - t[j][i];
//            }
//        }
//#endif
        return e_p;

    }

};

template <bool fixN, typename T>
void fillFullTable(std::array<std::array<LinExtT, MAXN>, MAXN>& target, std::array<std::array<T, MAXN>, MAXN>& source, LinExtT e_p, unsigned int nn = 0) {

    const unsigned int n = fixN ? NCT::N : nn;
    std::array<std::array<LinExtT, MAXN>, MAXN>& t = target;
    for (int i = 1; i < n; i++) { //calculate lower triangle
        for (int j = 0; j < i; j++) {
            t[j][i] = source[j][i];
            t[i][j] = e_p - source[j][i];
            DEBUG_ASSERT((T)t[j][i] == source[j][i]);
        }
    }
}


template <bool fixN>
void fillFullTable(std::array<std::array<LinExtT, MAXN>, MAXN> & t,  LinExtT e_p, unsigned int nn = 0) {

    const unsigned int n = fixN ? NCT::N : nn;

    for (int i = 1; i < n; i++) { //calculate lower triangle
        for (int j = 0; j < i; j++) {
            t[i][j] = e_p - t[j][i];
        }
    }
}

LinearExtensionCalculator::LinearExtensionCalculator(unsigned int N, unsigned int cc) :
#if defined(LINEEXT_CALC_OLD) || defined(LINEEXT_CALC_DEBUG)
        linExtTempMemory(1ULL << N),
#endif
        linExtTable(),
        C(cc)
{
    assert(N == NCT::N);
#if defined(LINEEXT_CALC_OLD) || defined(LINEEXT_CALC_DEBUG)

    for (int i = 0; i < (1 << N); i++)
        {
            linExtTempMemory[i][0] = 0;
            linExtTempMemory[i][1] = 0;
            linExtTempMemory[i][2] = 0;
        }
#endif


    internalCalcFull = new LinearExtensionCalculatorInternal<UdSetItemFull, LinExtT, false, false>(N,linExtTable);
    internalCalc32 = new LinearExtensionCalculatorInternal<UdSetItem32, uint32_t, true,true>(N,linExtTable32);

    size_t newTempSize = pow(1.74, N + 4);

#if LINEXT_TABLE_EXP
    void* pointer = std::malloc(sizeof(UdSetItemFull)*(1ULL << MAXN));
    void* pointer2 = std::malloc(sizeof(BitS)*newTempSize);
    internalCalcFull->allocatedMemorySize = newTempSize;
    internalCalc32->allocatedMemorySize = newTempSize;

    internalCalcFull->setPointer(pointer, pointer2);
    internalCalc32->setPointer(pointer, pointer2);
#else
    void* pointer = std::malloc(sizeof(UdSetItemFull)*newTempSize);
	internalCalcFull->allocatedMemorySize = newTempSize;
	internalCalc32->allocatedMemorySize = newTempSize;

    internalCalcFull->setPointer(pointer);
    internalCalc32->setPointer(pointer);
#endif
}

LinExtT LinearExtensionCalculator::calculateLinExtensionsSingleton(PosetHandle &poset, unsigned int c, bool fillTable, bool overflowCheck) {
    int n = NCT::N;

    if (poset.GetnumSingletons() <= 1) {

#ifdef LINEEXT_CALC_OLD
        LinExtT e_p = calculateLinExtensions(poset, n); //sets linExtensions and linExtTable
#else

        LinExtT e_p;
		if (overflowCheck && C - c < 27) {
            Stats::inc(STAT::NFullLinExtCalc32);
            e_p = internalCalc32->calculateLinExtensionsNew<true>(*poset, fillTable);
            if (e_p == 0) {
                Stats::inc(STAT::NFullLinExtCalc64);
                Stats::inc(STAT::NLinExtCalcOverflow);
                e_p = internalCalcFull->calculateLinExtensionsNew<false>(*poset, fillTable, n); //sets linExtensions and linExtTable
                if (fillTable) {
                    fillFullTable<false>(linExtTable, e_p, n);
                }
            } else if (fillTable) {
                fillFullTable<true>(linExtTable, linExtTable32, e_p);
            }
        } else if (!overflowCheck && C - c < 32) {
            Stats::inc(STAT::NFullLinExtCalc32);
            e_p = internalCalc32->calculateLinExtensionsNew<false>(*poset, fillTable); //sets linExtensions and linExtTable
			if(fillTable)
				fillFullTable<true>(linExtTable, linExtTable32, e_p);
        } else {
            Stats::inc(STAT::NFullLinExtCalc64);
            e_p = internalCalcFull->calculateLinExtensionsNew<false>(*poset, fillTable, n); //sets linExtensions and linExtTable
			if(fillTable)
				fillFullTable<false>(linExtTable, e_p, n);
        }
#endif

#ifdef LINEEXT_CALC_DEBUG
       LinExtT e_p_2 = calculateLinExtensions(poset, n);
	   if(fillTable){
		   for (unsigned int i = 0; i < n; i++) {
			   for (unsigned int j = 0; j < n; j++){
				   if(linExtTable[i][j] != linExtTable2[i][j]){
					   std::cout<<"c: " << c <<"i,j " << i<< "," << j << " linextTable: "<<linExtTable[i][j]<< " linextTable2: "<<linExtTable2[i][j]<<std::endl;
				   }
//				   assert(linExtTable[i][j] == linExtTable2[i][j]);
			   }
		   }
	   }
	   assert(e_p == e_p_2);
#endif

        return e_p;
    }
    else {
        unsigned int reduced_n = NCT::N - poset.GetnumSingletons() + 1;

        Stats::inc(STAT::NReducedLinExtCalc);
#ifdef LINEEXT_CALC_OLD
        LinExtT e_p = calculateLinExtensions(poset, reduced_n);
#else
        LinExtT e_p = internalCalcFull->calculateLinExtensionsNew<false>(*poset, fillTable, reduced_n);
		if(fillTable)
			fillFullTable<false>(linExtTable, e_p, reduced_n);
#endif

#ifdef LINEEXT_CALC_DEBUG
        LinExtT e_p_2 = calculateLinExtensions(poset, reduced_n);
		if(fillTable){
		   for (unsigned int i = 0; i < reduced_n; i++) {
			   for (unsigned int j = 0; j < reduced_n; j++){
				   if(linExtTable[i][j] != linExtTable2[i][j]){
					   std::cout<<"c: " << c <<"i,j " << i<< "," << j << " linextTable: "<<linExtTable[i][j]<< " linextTable2: "<<linExtTable2[i][j]<<std::endl;
				   }
//				   assert(linExtTable[i][j] == linExtTable2[i][j]);
			   }
		   }
		}
		assert(e_p == e_p_2);
#endif



        int k = poset.GetnumSingletons();
        LinExtT fac = fallingfactorial(n,reduced_n); //one singleton remains --> +1
        e_p *= fac; //adjust values with factorial
        std::array<std::array<LinExtT, MAXN>, MAXN> & t_reduced = linExtTable;
        for (unsigned int i = 0; i < reduced_n; i++) {
            for (unsigned int j = 0; j < reduced_n; j++)
                t_reduced[i][j] *= fac;
        }


        int lastIdx = reduced_n - 1;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i <= n - k && j <= n - k) { //includes the one singeltons
                    continue;					//  new_t[i][j] = t_reduced[i][j]; //copy entry
                }
                else if (i < n - k && j > n - k) {
                    t_reduced[i][j] = t_reduced[i][lastIdx]; //last entry of the row
                    continue;
                }
                else if (i > n - k && j < n - k) {
                    t_reduced[i][j] = t_reduced[lastIdx][j]; //last entry of the column
                    continue;
                }
                else if (i >= n - k && j >= n - k) {
                    if (i != j) {
                        t_reduced[i][j] = e_p / 2; //compare two singletons
                    }
                    else {
                        t_reduced[i][j] = 0;
                    }
                }
            }
        }
        return e_p;
    }
}

LinearExtensionCalculator::~LinearExtensionCalculator() {
    free(this->internalCalcFull->getPointer());
#if LINEXT_TABLE_EXP
    free(this->internalCalcFull->getPointer2());
#endif
    delete this->internalCalcFull;
    delete this->internalCalc32;
}
