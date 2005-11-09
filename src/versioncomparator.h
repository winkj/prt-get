////////////////////////////////////////////////////////////////////////
// FILE:        versioncomparator.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2004 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify  
//  it under the terms of the GNU General Public License as published by  
//  the Free Software Foundation; either version 2 of the License, or     
//  (at your option) any later version.                                   
////////////////////////////////////////////////////////////////////////

#ifndef _VERSIONCOMP_H_
#define _VERSIONCOMP_H_

namespace VersionComparator
{

enum COMP_RESULT { LESS, GREATER, EQUAL, UNDEFINED };

COMP_RESULT compareVersions(const string& v1, const string& v2) ;
void tokenizeIntoBlocks(const string& version, vector<string>& blocks);
int normalizeVectors(vector<string>& v1, vector<string>& v2);
void tokenizeMixed(const string& s, vector<string>& tokens);

}

#endif /* _VERSIONCOMP_H_ */

