////////////////////////////////////////////////////////////////////////
// FILE:        lockfile.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify  
//  it under the terms of the GNU General Public License as published by  
//  the Free Software Foundation; either version 2 of the License, or     
//  (at your option) any later version.                                   
////////////////////////////////////////////////////////////////////////

#ifndef _LOCKFILE_H_
#define _LOCKFILE_H_

#include <string>

class LockFile
{
public:
    LockFile();
    
    bool lockRead();
    bool lockWrite();
    bool lockReadWrite();
    
    bool unlock();
    
    void setFile( const std::string& fileName );
    
private:
    std::string m_fileName;

    bool m_readLock;
    bool m_writeLock;
    bool m_readWriteLock;

    static std::string LOCK_SUFFIX;
    
    static std::string WRITE_LOCK_STRING;
    static std::string READ_LOCK_STRING;
    static std::string READ_WRITE_LOCK_STRING;
    
};

#endif /* _LOCKFILE_H_ */
