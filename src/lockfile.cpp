////////////////////////////////////////////////////////////////////////
// FILE:        lockfile.cpp
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

#include "lockfile.h"

using namespace std;

string LockFile::LOCK_SUFFIX       = ".lock";
string LockFile::WRITE_LOCK_STRING = "write_lock";
string LockFile::READ_LOCK_STRING  = "read_lock";
string LockFile::READ_WRITE_LOCK_STRING  = "read_write_lock";


/*!
  \brief a file locking class
 */
LockFile::LockFile()
    : m_readLock( false ), 
      m_writeLock( false )   
{
}


bool LockFile::lockRead()
{
    if ( m_readLock ) {
        // we already lock it, so it's a good lock
        return true;
    }

    if ( m_writeLock ) {
        // can't lock for read and write separatedly
        return false;
    }

    struct stat buf;
    if ( stat( m_fileName.c_str(), &buf) == 0 ) {
        FILE* fp = fopen( m_fileName.c_str(), "w" );
        if ( fp ) {
            // file did not exist, could be created
            m_writeLock = true;
            fprintf( fp, "%s", READ_LOCK_STRING.c_str() );
            fclose( fp );
            return true;
        }
    } else {
        // file exists, but it could be a read lock
        
        // TODO: allow locking, increment lock count
    }
    
    // couldn't lock
    return false;
}


/*!
  lock the file
  \return true if we have a lock
 */
bool LockFile::lockWrite()
{
    if ( m_writeLock ) {
        // we already lock it, so it's a good lock
        return true;
    }
    
    if ( m_readLock ) {
        // can't lock for read and write separatedly
        return false;
    }

    struct stat buf;
    if ( stat( m_fileName.c_str(), &buf) == -1 ) {
        // file can not be stat'ed 
        FILE* fp = fopen( m_fileName.c_str(), "w" );
        if ( fp ) {
            // file did not exist, could be created
            m_writeLock = true;
            fprintf( fp, "%s", WRITE_LOCK_STRING.c_str() );
            fclose( fp );
            return true;
        }
    }
    
    // couldn't lock
    return false;
}

bool LockFile::lockReadWrite()
{
    if ( m_readWriteLock ) {
        // we already lock it, so it's a good lock
        return true;
    }
    
    if ( m_writeLock || m_readLock ) {
        // can't lock for read and write separatedly
        return false;
    }

    struct stat buf;
    if ( stat( m_fileName.c_str(), &buf) == 0 ) {
        FILE* fp = fopen( m_fileName.c_str(), "w" );
        if ( fp ) {
            // file did not exist, could be created
            m_writeLock = true;
            fprintf( fp, "%s", READ_WRITE_LOCK_STRING.c_str() );
            fclose( fp );
            return true;
        }
    }
    
    // couldn't lock
    return false;
    
}

bool LockFile::unlock()
{
    if ( m_readLock || m_writeLock || m_readWriteLock ) {
        // TODO: check whether lock count is 0
        if ( unlink( m_fileName.c_str() ) == 0 ) {
            m_readLock = m_writeLock = m_readWriteLock = false;
            return true;
        }
    }
    
    return false;
}


void LockFile::setFile( const string& fileName )
{
    m_fileName = fileName + LOCK_SUFFIX;
}
