////////////////////////////////////////////////////////////////////////
// FILE:        file.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <fnmatch.h>
#include <libgen.h>

using namespace std;

#include "stringhelper.h"
#include "pg_regex.h"

namespace File
{

bool fileExists( const string& fileName )
{
    struct stat result;
    return stat( fileName.c_str(), &result ) == 0;

}

bool grep( const string& fileName,
           const string& pattern,
           list<string>& result,
           bool fullPath,
           bool useRegex)
{
    FILE* fp;
    fp = fopen( fileName.c_str(), "r" );
    if ( !fp ) {
        return false;
    }
    const int length = BUFSIZ;
    char input[length];
    char* p;
    char* end;
    string line;
    string entry;
    
    RegEx re(pattern);

    while ( fgets( input, length, fp ) ) {
        p = strtok( input, "\t" );
        p = strtok( NULL, "\t" );
        p = strtok( NULL, "\t" );

        if ( p ) {
            // prepend slash to string
            p--;
            p[0] = '/';
            
            entry = p;
            end = strstr(p, "->");
            if (end) {
                *end = '\0';
            }
            p[strlen(p)-1] = '\0'; // strip newline
            
            char* name = p;
            if (!fullPath) {
                name = basename(p);
            }
                
            if (useRegex) {
                if (re.match(name)) {
                    result.push_back(StringHelper::stripWhiteSpace(entry));
                }
            } else {
                if ( fnmatch(pattern.c_str(), name, FNM_CASEFOLD) == 0 ) {
                    result.push_back( StringHelper::stripWhiteSpace(entry) );
                }
            }
        }
    }

    fclose( fp );
    return true;
}

}
