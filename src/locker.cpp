////////////////////////////////////////////////////////////////////////
// FILE:        locker.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <cstring>
#include <algorithm>

#include "locker.h"
#include "repository.h"

const string Locker::LOCKER_FILE_PATH = "/var/lib/pkg/";
const string Locker::LOCKER_FILE = "prt-get.locker";

Locker::Locker()
    : m_openFailed( false )
{
    string fName = LOCKER_FILE_PATH + LOCKER_FILE;
    FILE* fp = fopen( fName.c_str(), "r" );
    if ( fp ) {
        char input[512];
        while ( fgets( input, 512, fp ) ) {
            if ( input[strlen( input )-1] == '\n' ) {
                input[strlen( input )-1] = '\0';
            }
            if ( strlen( input ) > 0 ) {
                m_packages.push_back( input );
            }
        }

        fclose( fp );
    } else {
        m_openFailed = true;
    }
}

bool Locker::store()
{
    if ( !Repository::createOutputDir(LOCKER_FILE_PATH) ) {
        return false;
    }

    string fName = LOCKER_FILE_PATH + LOCKER_FILE;
    FILE* fp = fopen( fName.c_str(), "w" );
    if ( fp ) {
        vector<string>::iterator it = m_packages.begin();
        for ( ; it != m_packages.end(); ++it ) {
            fprintf( fp, "%s\n", it->c_str() );
        }
        fclose( fp );
        return true;
    }

    return false;
}

/*!
  \return true locking worked, false if already locked
*/
bool Locker::lock( const string& package )
{
    if ( isLocked( package ) ) {
        // already locked
        return false;
    }
    m_packages.push_back( package );
    return true;
}

/*!
  \return true if it could be unlocked, false if it wasn't locked
*/
bool Locker::unlock( const string& package )
{
    vector<string>::iterator it = find( m_packages.begin(), m_packages.end(),
                                        package );
    if ( it != m_packages.end() ) {
        m_packages.erase( it );
        return true;
    }

    return false;
}

bool Locker::isLocked( const string& package ) const
{
    if ( find( m_packages.begin(), m_packages.end(), package ) !=
         m_packages.end() ) {
        return true;
    }

    return false;
}

const vector<string>& Locker::lockedPackages() const
{
    return m_packages;
}

bool Locker::openFailed() const
{
    return m_openFailed;
}
