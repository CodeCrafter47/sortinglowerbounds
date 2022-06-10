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

#ifndef STATS_H
#define STATS_H

#include <array>
#include <cstdint>
#include <mutex>
#include <cassert>
#include <vector>

struct StatTag {
	char name[48];
};

struct StatTagAvMax {
	char name[48];
	int largeThreshold;
};

enum STAT {
	NPhase1,
	NPhase2,
	NPhase3,
	NMarkFirst,
	NMarkSecond,
	NChildMapBWFind,
	NChildMapBWFindNo,
	NChildMapBWFindYes,
	NChildMapBWFindUnf,
	NChildMapOldFind,
	NChildMapOldFindNo,
	NChildMapOldFindYes,
	NCompOneChild,
	NCompTwoChildren,
	NParentUnsortableBWLimit,
	NPredLimitEdgeCount,

	NPtrHashEqualTest,
	NEqualTest,
	NPointerHashDiff,
	NInPosetHashDiff,	
	NSingletonsDiff,
	NPairsDiff,	
	NIsoTest,
	NIsoPositive,
	NRevIsoTest,
	NBoostIsoTest,
	NBoostIsoPositive,	
	
	NSelfdualIdCreated,

	NFullLinExtCalc32,
	NFullLinExtCalc64,
	NReducedLinExtCalc,
	NLinExtCalcOverflow,

    NReorderGraph,
	
	NAmbiguous,
	NAmbiguousIso,

	NUM_STATS
};


enum AVMSTAT {
	NDownSets,
	HFindGlobNStepsPos,
	HFindGlobNStepsNeg,
	NAutoFound,
	NCycleAutoFound,

	ELSizePhase1,
	ELSizePhase2,

	PotPredCount,
    PredCount,

	NUM_AVMSTATS
};

//constexpr unsigned int NUM_STATS_TOTAL = NUM_AVMSTATS + NUM_STATS ;

static constexpr std::array< StatTag, NUM_STATS >  build_array( ) noexcept 
{ 
	std::array<StatTag, NUM_STATS> mat = { {"asdf", false} };

	mat[STAT::NPhase1] =                StatTag{"#Phase1"};
	mat[STAT::NPhase2] =                StatTag{"#Phase2"};
	mat[STAT::NPhase3] =                StatTag{"#Phase3"};
	mat[STAT::NMarkFirst] =             StatTag{"#MarkFirst"};
	mat[STAT::NMarkSecond] =            StatTag{"#MarkSecond"};
	mat[STAT::NChildMapBWFind] =        StatTag{"#ChildMapBWFind"};
	mat[STAT::NChildMapBWFindNo] =      StatTag{"#ChildMapBWFindNo"};
	mat[STAT::NChildMapBWFindYes] =     StatTag{"#ChildMapBWFindYes"};
	mat[STAT::NChildMapBWFindUnf] =     StatTag{"#ChildMapBWFindUnf"};
	mat[STAT::NChildMapOldFind] =       StatTag{"#ChildMapOldFind"};
	mat[STAT::NChildMapOldFindNo] =     StatTag{"#ChildMapOldFindNo"};
	mat[STAT::NChildMapOldFindYes] =    StatTag{"#ChildMapOldFindYes"};
	mat[STAT::NCompOneChild] =          StatTag{"#CompOneChild"};
	mat[STAT::NCompTwoChildren] =       StatTag{"#CompTwoChildren"};
	mat[STAT::NParentUnsortableBWLimit]=StatTag{"#ParentUnsortBWLim"};
	mat[STAT::NPredLimitEdgeCount] =    StatTag{"#PredLimitEdgeCount"};




	mat[STAT::NBoostIsoTest] = 			StatTag{"#BoostIsoTest"};
	mat[STAT::NBoostIsoPositive] = 		StatTag{"#BoostIsoPos"};
	mat[STAT::NIsoTest] = 				StatTag{"#IsoT"};
	mat[STAT::NIsoPositive] = 			StatTag{"#IsoPositive"};
	mat[STAT::NRevIsoTest] = 			StatTag{"#RevIsoT"};
	mat[STAT::NEqualTest] = 			StatTag{"#EqTest"};
	mat[STAT::NPtrHashEqualTest] = 		StatTag{"#PtrHashEqTest"};
	
	mat[STAT::NPointerHashDiff] = 		StatTag{"#PtrHashDif"};
	mat[STAT::NInPosetHashDiff] = 		StatTag{"#PosHashDif"};
	
	mat[STAT::NSingletonsDiff] = 		StatTag{"#SingletDiff"};
	mat[STAT::NPairsDiff] = 			StatTag{"#PairsDiff"};

	mat[STAT::NSelfdualIdCreated] = 	StatTag{"#SelfdualIdCr"};

	mat[STAT::NFullLinExtCalc32] = 		StatTag{"#FullLinExt32"};
	mat[STAT::NFullLinExtCalc64] = 		StatTag{"#FullLinExt64"};
	mat[STAT::NReducedLinExtCalc] = 	StatTag{"#RedLinExt"};
	mat[STAT::NLinExtCalcOverflow] = 	StatTag{"#LinExtOverflow"};
    mat[STAT::NReorderGraph] = 			StatTag{"#ReorderGraph"};

	mat[STAT::NAmbiguous] = 			StatTag{"#Ambiguous"};
	mat[STAT::NAmbiguousIso] = 			StatTag{"#AmbiguousIso"};

    return mat;
}



static constexpr std::array< StatTagAvMax, AVMSTAT::NUM_AVMSTATS >  build_array_AvMax( ) noexcept
{ 
	std::array<StatTagAvMax, AVMSTAT::NUM_AVMSTATS> mat = { {"asdf", false} };
	
	mat[AVMSTAT::NDownSets] = 			StatTagAvMax{"#DownSets",   1000};
	mat[AVMSTAT::HFindGlobNStepsPos] = 	StatTagAvMax{"HFdGloPos#Step", 5};
	mat[AVMSTAT::HFindGlobNStepsNeg] = 	StatTagAvMax{"HFdGloNeg#Step", 3};

	mat[AVMSTAT::NAutoFound] = 			StatTagAvMax{"#AutoFound", 1};
	mat[AVMSTAT::NCycleAutoFound] = 	StatTagAvMax{"#CyclAutFound", 1};

	mat[AVMSTAT::ELSizePhase1] =        StatTagAvMax{"ELSizePhase1", 10};
	mat[AVMSTAT::ELSizePhase2] =        StatTagAvMax{"ELSizePhase2", 10};

	mat[AVMSTAT::PotPredCount] =        StatTagAvMax{"PotPredCount", 100};
    mat[AVMSTAT::PredCount]    =        StatTagAvMax{"PredCount", 100};

    return mat;
}


constexpr std::array<StatTag, STAT::NUM_STATS> statTags = build_array();
constexpr std::array<StatTagAvMax, AVMSTAT::NUM_AVMSTATS> statTagsAvMax = build_array_AvMax();



struct AvMaxItem {
	uint64_t num;
	uint64_t sum;
	uint64_t max;
	uint64_t numLarge;	
	
	inline void reset(){
		num = 0;
		sum = 0;
		max = 0;
		numLarge = 0;
	}
	
	inline void accumulate(const AvMaxItem & other){
		num += 		other.num;
		numLarge += other.numLarge;
		sum += 		other.sum;
		max = 		std::max(max, other.max);
		
	}
	
	AvMaxItem()
	{
		num = 0;
		sum = 0;
		max = 0;
		numLarge = 0;
	}
};


class Stats{
	static thread_local std::array<uint64_t, NUM_STATS> loc;
	static std::array<uint64_t, NUM_STATS> glob;
	static std::array<uint64_t, NUM_STATS> globRecent;
	static thread_local std::array<AvMaxItem, NUM_AVMSTATS> locAvMax;
	static std::array<AvMaxItem, NUM_AVMSTATS> globAvMax;
	static std::array<AvMaxItem, NUM_AVMSTATS> globAvMaxRecent;
	
	
	
	static std::mutex lock;
	
public:	
	
	static inline void accumulate(){
		std::lock_guard<std::mutex> lck(lock);
		for (int i = 0; i < NUM_STATS ; i++){
			glob[i] += loc[i];
			globRecent[i] += loc[i];
			loc[i] = 0;
		}
		for (int i = 0 ; i < AVMSTAT::NUM_AVMSTATS ; i++){
			globAvMax[i].accumulate(locAvMax[i]);
			globAvMaxRecent[i].accumulate(locAvMax[i]);
			locAvMax[i].reset();	
		}	
	}
	
	
	static inline void inc(STAT stat){
		assert(stat < STAT::NUM_STATS);
		loc[stat]++;		
	}
	
	static inline uint64_t get(STAT stat) {
		std::lock_guard<std::mutex> lck(lock);
		assert(stat < STAT::NUM_STATS);
		return glob[stat];		
	}
	
	template<AVMSTAT stat>
	static inline void addVal( uint64_t val){
		assert(stat < AVMSTAT::NUM_AVMSTATS );
		locAvMax[stat].num++;
		if(val >= statTagsAvMax[stat].largeThreshold)
			locAvMax[stat].numLarge++;
		locAvMax[stat].sum += val;	
		locAvMax[stat].max = std::max(locAvMax[stat].max, val);		
	}
	
	template<AVMSTAT stat>
	static inline void addValNum(uint64_t val, uint64_t num){
		assert(stat < AVMSTAT::NUM_AVMSTATS );
		locAvMax[stat].num += num;
		if((double)val/ (double)num >= (double) statTagsAvMax[stat].largeThreshold)
			locAvMax[stat].numLarge++;
		locAvMax[stat].sum += val;	
		locAvMax[stat].max = std::max(locAvMax[stat].max, (uint64_t)((double)val / (double)num));		
	}
	
	static void resetRecent(){
		std::lock_guard<std::mutex> lck(lock);
		for (int i = 0; i < NUM_STATS ; i++){
			globRecent[i] = 0;
		}
	}

	static std::vector<std::string> detailed();

	static std::string printFrequent();
};


#endif