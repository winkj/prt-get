////////////////////////////////////////////////////////////////////////
// FILE:        stringhelper.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#include "stringhelper.h"
#include <cctype>

using namespace std;

namespace StringHelper
{




/*!
  \param s the string to be searched
  \param del the delimiter char
  \return the value after the first occurance of \a del
 */
string getValue( const string& s, char del )
{
    string::size_type pos = s.find( del );
    if ( pos != string::npos && pos+1 < s.length() ) {
        return s.substr( pos + 1 );
    }
    return "";
}

/*!
  \param s the string to be searched
  \param del the delimiter char
  \return the value before the first occurance of \a del
 */
string getValueBefore( const string& s, char del )
{
    string::size_type pos = s.find( del );
    if ( pos != string::npos ) {
        return s.substr( 0, pos );
    }
    return s;
}

/*!
  strip whitespace in the beginning and end of string \a s
  \return a stripped string
*/
string stripWhiteSpace( const string& s )
{
    if ( s.empty() ) {
        return s;
    }

    int pos = 0;
    string line = s;
    string::size_type len = line.length();
    while ( pos < len && isspace( line[pos] ) ) {
        ++pos;
    }
    line.erase( 0, pos );
    pos = line.length()-1;
    while ( pos > -1 && isspace( line[pos] ) ) {
        --pos;
    }
    if ( pos != -1 ) {
        line.erase( pos+1 );
    }
    return line;
}

/*!
  make sure s1 starts with s2
*/
bool startsWith( const string& s, const string& with )
{
    if (s.length() < with.length())
        return false;
 
    return s.substr(0, with.length()) == with;
}

/*!
  make sure s1 starts with s2
*/
bool startsWithNoCase( const string& s1, const string& s2 )
{
    string::const_iterator p1 = s1.begin();
    string::const_iterator p2 = s2.begin();

    while ( p1 != s1.end() && p2 != s2.end() ) {
        if ( toupper( *p1 ) != toupper( *p2 ) ) {
            return false;
        }
        ++p1;
        ++p2;
    }

    if ( p1 == s1.end() && p2 != s2.end() ) {
        return false;
    }

    return true;
}

/*!
  Convert a string into a lowercase representation
  \param s the string to be converted
  \return a lowercase representation of \a s
*/
string toLowerCase( const string& s )
{
    string result = "";
    for ( string::size_type i = 0; i < s.length(); ++i ) {
        result += tolower( s[i] );
    }

    return result;
}

/*!
  Convert a string into a uppercase representation
  \param s the string to be converted
  \return a uppercase representation of \a s
*/
string toUpperCase( const string& s )
{
    string result = "";
    for ( string::size_type i = 0; i < s.length(); ++i ) {
        result += toupper( s[i] );
    }

    return result;
}

/*!
  replace all occurances of \a oldString in \a in with \a newString
*/
string replaceAll( string& in,
                   const string& oldString,
                   const string& newString )
{
    size_t pos;
    while ( (pos = in.find( oldString )) != string::npos ) {
        in =
            in.replace( pos, oldString.length(), newString );
    }

    return in;
}


}; // Namespace
