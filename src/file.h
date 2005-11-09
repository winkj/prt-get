////////////////////////////////////////////////////////////////////////
// FILE:        file.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#ifndef _FILE_H_
#define _FILE_H_

#include <list>
#include <string>

namespace File
{

bool fileExists( const std::string& fileName );
bool grep( const std::string& fileName,
           const std::string& pattern,
           std::list<string>& result,
           bool fullPath,
           bool useRegex);

}

#endif /* _FILE_H_ */
