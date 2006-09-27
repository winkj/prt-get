////////////////////////////////////////////////////////////////////////
// FILE:        stringhelper.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#ifndef _STRINGHELPER_H_
#define _STRINGHELPER_H_

#include <list>
#include <string>
using namespace std;

/*!
  \brief A generic place with string functions
*/
namespace StringHelper
{

template <class T>
void split( const string& s, char del,
            T& target,
            int startPos=0, bool useEmpty=true  );

string stripWhiteSpace( const string& s );
bool startsWith( const string& s, const string& with );
bool startsWithNoCase( const string& s1, const string& s2 );

string getValue( const string& s, char del );
string getValueBefore( const string& s, char del );

string toLowerCase( const string& s );
string toUpperCase( const string& s );

string replaceAll( string& in,
                   const string& oldString,
                   const string& newString );


/*!
  split a string into parts

  \param s string to be split
  \param del delimter
  \param startPos position to start at
  \param useEmpty include empty (whitespace only9 results in the result

  \return a list of string
*/
template <class T>
void split( const string& s, char del,
            T& target,
            int startPos, bool useEmpty )
{
    string line = s;

    string::size_type pos;
    int offset = startPos;
    while ( ( pos = line.find( del, offset ) ) != string::npos ) {
        offset = 0;

        string val = line.substr( 0, pos );
        if ( useEmpty || !stripWhiteSpace( val ).empty() ) {
            target.push_back( val );
        }
        line.erase( 0, pos+1 );
    }

    if ( line.length() > 0 ) {
        target.push_back( line );
    }
}


};
#endif /* _STRINGHELPER_H_ */
