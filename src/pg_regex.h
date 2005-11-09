////////////////////////////////////////////////////////////////////////
// FILE:        regex.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2005 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify  
//  it under the terms of the GNU General Public License as published by  
//  the Free Software Foundation; either version 2 of the License, or     
//  (at your option) any later version.                                   
////////////////////////////////////////////////////////////////////////


#ifndef _REGEX_H_
#define _REGEX_H_

#include <sys/types.h>
#include <regex.h>
#include <string>


class RegEx
{
public:
    RegEx(const std::string& pattern, bool caseSensitive=false);
    ~RegEx();

    bool match(const std::string& input);

    static bool match(const std::string& pattern,
                      const std::string& input,
                      bool caseSensitive=false);
    
private:
    regex_t m_pattern;
    bool m_validPattern;
};


#endif /* _REGEX_H_ */
