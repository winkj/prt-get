////////////////////////////////////////////////////////////////////////
// FILE:        pg_regex.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2005 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify  
//  it under the terms of the GNU General Public License as published by  
//  the Free Software Foundation; either version 2 of the License, or     
//  (at your option) any later version.                                   
////////////////////////////////////////////////////////////////////////


#include <string>
using namespace std;

#include "pg_regex.h"


RegEx::RegEx(const string& pattern, bool caseSensitive) 
{
    int additionalFlags = 0;
    if (!caseSensitive) {
        additionalFlags |= REG_ICASE;
    }

    m_validPattern = 
        regcomp(&m_pattern, 
                pattern.c_str(), 
                REG_EXTENDED|REG_NOSUB|additionalFlags) == 0;
}

RegEx::~RegEx() 
{
    regfree(&m_pattern);
}

bool RegEx::match(const string& input) 
{
    if (!m_validPattern) {
        return false;
    }
    bool success = (regexec(&m_pattern, input.c_str(), 0, 0, 0) == 0);
    return success;
}

bool RegEx::match(const string& pattern,
                  const string& input,
                  bool caseSensitive) 
{
    RegEx re(pattern, caseSensitive);
    return re.match(input);
}
