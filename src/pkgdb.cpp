////////////////////////////////////////////////////////////////////////
// FILE:        pkgdb.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <fstream>
#include <map>
#include <iostream>
using namespace std;

#include <cstring>
#include <cstdio>

#include <fnmatch.h>

#include "pkgdb.h"
#include "datafileparser.h"
#include "stringhelper.h"
#include "pg_regex.h"


const string PkgDB::PKGDB = "/var/lib/pkg/db";
const string PkgDB::ALIAS_STORE = LOCALSTATEDIR"/lib/pkg/prt-get.aliases";

/*!
  Create a PkgDB object
*/
PkgDB::PkgDB( const string& installRoot )
    : m_isLoaded( false ),
      m_installRoot( installRoot )
{
}

/*!
  Check whether a package is installed

  \param name the name of the package to check
  \param isAlias whether a package is installed as alias
  \param aliasOriginalName the original name of an aliased package

  \return whether package \a name is installed
*/
bool PkgDB::isInstalled( const string& name,
                         bool useAlias,
                         bool* isAlias,
                         string* aliasOrignalName ) const
{
    if ( !load() ) {
        return false;
    }

    bool installed = m_packages.find( name ) != m_packages.end();
    if (!installed && useAlias) {
        string provider;
        installed = aliasExistsFor(name, provider);

        if (isAlias) {
            *isAlias = installed;

            if (installed && aliasOrignalName) {
                *aliasOrignalName = provider;
            }
        }
    }

    return installed;
}


bool PkgDB::aliasExistsFor(const string& name, string& providerName) const
{
    // when used the first time, split alias names
    if (m_splitAliases.size() < m_aliases.size()) {
        map<string, string>::iterator it = m_aliases.begin();
        for (; it != m_aliases.end(); ++it) {
            StringHelper::split(it->second, ',',
                                m_splitAliases[it->first]);            
        }
    }
    
    map<string, vector<string> >::iterator it = m_splitAliases.begin();
    for (; it != m_splitAliases.end(); ++it) {
        if (find(it->second.begin(), it->second.end(), name) != 
            it->second.end()) {
            providerName = it->first;
            return true;
        }
    }

    return false;
}

/*!
  load the package db
*/
bool PkgDB::load() const
{
    if ( m_isLoaded ) {
        return true;
    }

#if 0
    // check this one out to see a really slow IO library :-(


    ifstream db( PKGDB );
    string line;
    bool emptyLine = true;
    while ( db.good() ) {
        getline( db, line );
        if ( emptyLine ) {
            if ( !line.empty() ) {
                m_packages.push_back( line );
            }
            emptyLine = false;
        }
        if ( line == "" ) {
            emptyLine = true;
        }
    }
    db.close();
#endif

    std::map<std::string, std::string> aliases;
    DataFileParser::parse(ALIAS_STORE, aliases);

    const int length = 256;
    char line[length];
    bool emptyLine = true;
    bool nameRead = false;
    string name;

    string pkgdb = "";
    if (m_installRoot != "") {
        pkgdb = m_installRoot;
    }
    pkgdb += PKGDB;

    FILE* fp = fopen( pkgdb.c_str(), "r" );
    if ( fp ) {
        while ( fgets( line, length, fp ) ) {
            if ( emptyLine ) {
                line[strlen(line)-1] = '\0';
                name = line;
                emptyLine = false;
                nameRead = true;
            } else if ( nameRead ) {
                line[strlen(line)-1] = '\0';
                m_packages[ name ] = line;
                nameRead = false;
                if (aliases.find(name) != aliases.end()) {
                    m_aliases[name] = aliases[name];
                }
            }
            if ( line[0] == '\n' ) {
                emptyLine = true;
            }

        }
    } else {
        return false;
    }

    m_isLoaded = true;

    fclose( fp );

    return true;
}

/*!
  return a map of installed packages, where the key is the package name and
  the value is the version/release string
  \return a map of installed packages (key=name, value=version/release)
*/
const map<string, string>& PkgDB::installedPackages()
{
    load();
    return m_packages;
}

/*!
  \return a package's version and release or an empty string if not found
*/
string PkgDB::getPackageVersion( const string& name ) const
{
    if ( !load() ) {
        return "";
    }

    map<string, string>::const_iterator it = m_packages.find( name );
    if ( it == m_packages.end() ) {
        return "";
    }

    return it->second;
}

/*!
  Search packages for a match of \a pattern in name. The name can
  contain shell wildcards.

  \param pattern the pattern to be found
  \return a list of matching packages
*/
void PkgDB::getMatchingPackages( const string& pattern,
                                 map<string,string>& target,
                                 bool useRegex ) const
{
    if ( !load() ) {
        return;
    }

    RegEx re(pattern);
    map<string, string>::const_iterator it = m_packages.begin();
    if (useRegex) {
        for ( ; it != m_packages.end(); ++it ) {
            if (re.match(it->first)) {
                target[it->first] = it->second;
            }
        }
    } else {
        for ( ; it != m_packages.end(); ++it ) {
            // I assume fnmatch will be quite fast for "match all" (*), so
            // I didn't add a boolean to check for this explicitely
            if ( fnmatch( pattern.c_str(), it->first.c_str(), 0  ) == 0 ) {
                target[it->first] = it->second;
            }
        }
    }
}
