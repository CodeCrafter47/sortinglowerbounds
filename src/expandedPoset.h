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

#ifndef EXPANDEDPOSET_H
#define EXPANDEDPOSET_H

#include "config.h"
#include "niceGraph.h"
#include "posetObj.h"
#include "posetInfo.h"
#include "posetHandle.h"

class PosetHandle;

class ExpandedPosetChild {

private:
    NiceGraph niceGraphClosure;
	PosetObj poset;
	PosetInfo info;
	LinExtT linExt;

public:

    /**
    * Adds an edge to the PosetObj i.e. its graph and revBoostgraph.
    *
    * @param k1 The source of the edge
    * @param k2 The target of the edge
    * @param linExt number of linear extensions
    */
	ExpandedPosetChild(PosetHandle &parent, LinExtT linExt, int kk1, int kk2);

	/**
	 * Create expanded poset from adj matrix and info
	 */
	ExpandedPosetChild(const AdjacencyMatrix& p, const PosetInfo& info, LinExtT linExt, int k1, int k2);

	/**
	 * Create expanded poset from adj matrix and info
	 */
	ExpandedPosetChild(const AdjacencyMatrix& p, const PosetInfo& info, LinExtT linExt);

	/**
	* Tells if this poset is definitely sortable in cLeft comparisons.
	* If this method outputs true, then it is definitely sortable in cLeft comparisons.
	* If the output is false it might still be sortable in cLeft comparisons.
	*
	* @param cLeft The number of comparisons left.
	* @return true if definitely sortable in cLeft
	*         false otherwise.
	*/
	bool isEasilySortableUnrelatedPairs(unsigned int cLeft);

	AnnotatedPosetObj getHandle();
};

#endif // EXPANDEDPOSET_H
