////////////////////////////////////////////////////////////////////////
// FILE:        depresolver.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#ifndef _DEPRESOLVER_H_
#define _DEPRESOLVER_H_

#include <list>
using namespace std;

/*!
  \class DepResolver
  \brief a dependency resolver
  
  A dependency resolver
*/
class DepResolver
{
public:
    void addDependency( int, int );
    bool resolve( list<int>& result );

private:
    /*! simple int pair, so we don't have to use std::pair */
    struct Pair {
        Pair( int f, int s ) : first( f ), second( s ) {}
        int first;
        int second;
    };

    bool topSort( list<int>& result );

    list<Pair> m_dependencies;
};

#endif /* _DEPRESOLVER_H_ */
