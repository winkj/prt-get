////////////////////////////////////////////////////////////////////////
// FILE:        depresolver.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#include <map>
#include <cassert>
using namespace std;

#include "depresolver.h"



/*!
  add a dependency
  \param first the package with dependency
  \param second the pacakge which \a first depends on
*/
void DepResolver::addDependency( int first, int second )
{
    m_dependencies.push_back( Pair( first, second ) );
}


/*!
  resolve the dependencies
  \param result a list which will be filled with resulting indexes in 
  the correct order
  \return true on success, false otherwise (cyclic dependencies)
*/
bool DepResolver::resolve( list<int>& result )
{
    return topSort( result );
}


/*!
  sort the dependencies
*/
bool DepResolver::topSort( list<int>& result )
{
    map<int, int > numPreds;   // elt -> number of predecessors
    map<int, list<int> > successors; // elt -> number of successors

    list<Pair>::iterator it = m_dependencies.begin();
    for ( ; it != m_dependencies.end(); ++it ) {

        // make sure every elt is a key in numpreds
        if ( numPreds.find( it->first ) == numPreds.end() ) {
            numPreds[it->first] = 0;
        }
        if ( numPreds.find( it->second ) == numPreds.end() ) {
            numPreds[it->second] = 0;
        }

        // if they're the same, there's no real dependence
        if ( it->first == it->second ) {
            continue;
        }

        // since first < second, second gains a pred
        numPreds[it->second] = numPreds[it->second] + 1;

        // ... and first gains a succ
        successors[it->first].push_back( it->second );
    }

    // suck up everything without a predecessor
    result.clear();
    map<int, int>::iterator mit = numPreds.begin();
    for ( ; mit != numPreds.end(); ++mit ) {
        if ( mit->second == 0 ) {
            result.push_back( mit->first );
        }
    }

    // for everything in answer, knock down the pred count on
    // its successors; note that answer grows *in* the loop
    list<int>::iterator lit = result.begin();
    for ( ; lit != result.end(); ++lit ) {
        assert( numPreds[*lit] == 0 );
        numPreds.erase( *lit );
        if ( successors.find( *lit ) != successors.end() ) {
            list<int>::iterator succIt = successors[*lit].begin();
            for ( ; succIt != successors[*lit].end(); ++succIt ) {
                numPreds[*succIt] = numPreds[*succIt] - 1;
                if ( numPreds[*succIt] == 0 ) {
                    result.push_back( *succIt );
                }
            }
            successors.erase( *lit );
        }
    }

    return numPreds.size() == 0;
}
