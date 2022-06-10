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

#include "isoTest.h"

//#include <boost/graph/graph_utility.hpp>
//#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/vf2_sub_graph_iso.hpp>
#include <boost/graph/adjacency_list.hpp>

#include "stats.h"
#include "posetObj.h"

using Boostgraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS>;

template <typename Boostgraph1, typename Boostgraph2> //used for isomorphism test, since vf2_graph_iso(g1, g2, callback).
struct vf2_simple_callback {

    vf2_simple_callback(const Boostgraph1& graph1, const Boostgraph2& graph2)
            : graph1_(graph1), graph2_(graph2) {}

    template <typename CorrespondenceMap1To2, typename CorrespondenceMap2To1>
    bool operator()(CorrespondenceMap1To2 f, CorrespondenceMap2To1) const {
        return false; // abort search as soon as one correspondance is found
    }

private:
    const Boostgraph1& graph1_;
    const Boostgraph2& graph2_;
};

bool boost_is_isomorphic(const PosetObj& first, const PosetObj& second, unsigned int reduced_n) {

    assert(first.GetSelfdualId() == second.GetSelfdualId());
    Stats::inc(STAT::NBoostIsoTest);

    Boostgraph g1 = first.GetReducedBoostgraph(reduced_n);
    Boostgraph g2 = second.GetReducedBoostgraph(reduced_n);

    vf2_simple_callback<Boostgraph, Boostgraph> callback(g1, g2);
    bool result = boost::vf2_graph_iso(g1, g2, callback);
    if (result)
        Stats::inc(STAT::NBoostIsoPositive);

    if(result && first.isUniqueGraph())
    {
        std::cout << "Graph doch nicht unique." << std::endl;
        std::cout << "IsoTest, g1:" << std::endl;
        boost::print_graph(g1);
        std::cout << "IsoTest, g2:" << std::endl;

        boost::print_graph(g2);
        std::cout << std::endl;
        std::cout << std::endl;


        std::cout<<"poset1:" <<std::endl;
        first.print_poset();
        std::cout<<"poset2:" <<std::endl;
        second.print_poset();
		assert(false);
    }

    return result;
}

bool boost_is_rev_isomorphic(const PosetObj& first, const PosetObj& second, unsigned int reduced_n) {

    Stats::inc(STAT::NRevIsoTest);

    assert(first.GetSelfdualId() && second.GetSelfdualId());
    Stats::inc(STAT::NBoostIsoTest);


    Boostgraph g1 = first.GetReducedBoostgraph(reduced_n);
    Boostgraph g2 = second.GetRevReducedBoostgraph(reduced_n);

    vf2_simple_callback<Boostgraph, Boostgraph> callback(g1, g2);
    bool result = boost::vf2_graph_iso(g1, g2, callback);
    if (result)
        Stats::inc(STAT::NBoostIsoPositive);
    return result;
}