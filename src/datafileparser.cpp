////////////////////////////////////////////////////////////////////////
// FILE:        datafileparser.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify  
//  it under the terms of the GNU General Public License as published by  
//  the Free Software Foundation; either version 2 of the License, or     
//  (at your option) any later version.                                   
////////////////////////////////////////////////////////////////////////

#include <cstdio>

#include "datafileparser.h"
#include "stringhelper.h"

using namespace std;
using namespace StringHelper;

bool DataFileParser::parse(const string& fileName, 
                           map<string, string>& target)
{
    FILE* fd = fopen(fileName.c_str(), "r");
    char buffer[512];
    string input;
    
    if (!fd) {
        return false;
    }
    
    while (fgets(buffer, 512, fd)) {
        input = buffer;
        input = stripWhiteSpace(input);

        if (input.length() > 0 && input[0] != '#') {
            string::size_type pos = input.find(":");
            if (pos != string::npos) {
                string name = stripWhiteSpace(input.substr(0, pos));
                string deps = stripWhiteSpace(input.substr(pos+1));
                deps = StringHelper::replaceAll(deps, "  ", " ");
                deps = StringHelper::replaceAll(deps, " ", ",");
                deps = StringHelper::replaceAll(deps, ",,", ",");

                target[name] = deps;
            }
        }
    }
    fclose(fd);
    return true;
}
