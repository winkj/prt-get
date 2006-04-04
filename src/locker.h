////////////////////////////////////////////////////////////////////////
// FILE:        locker.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#ifndef _LOCKER_H_
#define _LOCKER_H_

#include <string>
#include <vector>

using namespace std;

/**
 * prt-get can place packages in the locker, which are then not updated
 * anymore.
 * Locked packages are:
 * - marked in prt-get diff
 * - not shown in prt-get quickdiff
 * - not updated in prt-get sysup
 * 
 * remember to call store!
 */
class Locker
{
public:
    Locker();

    bool store();

    bool lock( const string& package );
    bool unlock( const string& package );
    bool isLocked( const string& package ) const;

    const vector<string>& lockedPackages() const;

    bool openFailed() const;
private:

    vector<string> m_packages;
    static const string LOCKER_FILE;
    static const string LOCKER_FILE_PATH;

    bool m_openFailed;
};

#endif /* _LOCKER_H_ */
