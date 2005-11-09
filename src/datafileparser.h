////////////////////////////////////////////////////////////////////////
// FILE:        datafileparser.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2004 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify  
//  it under the terms of the GNU General Public License as published by  
//  the Free Software Foundation; either version 2 of the License, or     
//  (at your option) any later version.                                   
////////////////////////////////////////////////////////////////////////

#ifndef _DATAFILEPARSER_H_
#define _DATAFILEPARSER_H_

#include <map>
#include <string>

/**
 * Parser for files of the format
 * key : value1,value2
 */
class DataFileParser
{
public:
    static bool parse(const std::string& fileName, 
                      std::map<std::string, std::string>& target);
};

#endif /* _DATAFILEPARSER_H_ */
