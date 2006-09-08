////////////////////////////////////////////////////////////////////////
// FILE:        repository.cpp
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
#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;


#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>

#include "datafileparser.h"
#include "repository.h"
#include "stringhelper.h"
#include "pg_regex.h"
using namespace StringHelper;


const string Repository::CACHE_VERSION = "V5";
const string Repository::EXTERNAL_DEPENDENCY_FILE = 
    LOCALSTATEDIR"/lib/pkg/prt-get.deplist";

/*!
  Create a repository
*/
Repository::Repository(bool useRegex)
    : m_useRegex(useRegex)
{
}

/*!
  Destroy a repository
*/
Repository::~Repository()
{
    map<string, Package*>::const_iterator it = m_packageMap.begin();
    for ( ; it != m_packageMap.end(); ++it ) {
        delete it->second;
    }
}


/*!
  \return a map of available packages
*/
const map<string, Package*>& Repository::packages() const
{
    return m_packageMap;
}


/*!
  Returns a map of duplicate packages in the repository. The key is the name
  of the package, the value a pair where \a first the shadowed port and
  \a second is the port which preceeds over \a first
  \return a map of duplicate packages in the repository
*/
const map<string, pair<Package*, Package*> >& Repository::shadowedPackages() const
{
    return m_shadowedPackages;
}


void Repository::parseDependencyList()
{
    map<string, string> depMap;
    if (DataFileParser::parse(EXTERNAL_DEPENDENCY_FILE, depMap)) {
        addDependencies(depMap);
    }
}


/*!
  \param name the package name to be returned
  \return a Package pointer for a package name or 0 if not found
*/
const Package* Repository::getPackage( const string& name ) const
{
    map<string, Package*>::const_iterator it = m_packageMap.find( name );
    if ( it == m_packageMap.end() ) {
        return 0;
    }
    return it->second;
}


/*!
  Search packages for a match of \a pattern in name, and description of
  \a searchDesc is true.
  \note Name searches can often done without opening the Pkgfiles, but not
  description search. Therefore, the later is much slower

  \param pattern the pattern to be found
  \param searchDesc whether descriptions should be searched as well
  \return a list of matching packages
*/

void Repository::searchMatchingPackages( const string& pattern,
                                         list<Package*>& target,
                                         bool searchDesc ) const
    // note: searchDesc true will read _every_ Pkgfile
{
    map<string, Package*>::const_iterator it = m_packageMap.begin();
    if (m_useRegex) {
        RegEx re(pattern);
        for ( ; it != m_packageMap.end(); ++it ) {
            if (re.match(it->first)) {
                target.push_back( it->second );
            } else if ( searchDesc ) {
                if ( re.match(it->second->description())) {
                    target.push_back( it->second );
                }
            }
        }
    } else {
        for ( ; it != m_packageMap.end(); ++it ) {
            if ( it->first.find( pattern ) != string::npos ) {
                target.push_back( it->second );
            } else if (searchDesc ) {
                string s = toLowerCase( it->second->description() );
                if ( s.find( toLowerCase( pattern ) ) != string::npos ) {
                    target.push_back( it->second );
                }
            }
        }
    }
}


/*!
  init repository by reading the directories passed. Doesn't search
  recursively, so if you want /dir and /dir/subdir checked, you have to
  specify both

  \param rootList a list of directories to look for ports in
  \param listDuplicate whether duplicates should registered (slower)
*/
void Repository::initFromFS( const list< pair<string, string> >& rootList,
                             bool listDuplicate )
{
    list< pair<string, string> >::const_iterator it = rootList.begin();
    DIR* d;
    struct dirent* de;
    string name;

    std::map<string, bool> alreadyChecked;


    for ( ; it != rootList.end(); ++it ) {

        string path = it->first;
        string pkgInput = stripWhiteSpace( it->second );

        if ( alreadyChecked[path] ) {
            continue;
        }

        bool filter = false;
        if ( pkgInput.length() > 0 ) {
            filter = true;
            // create a proper input string
            while ( pkgInput.find( " " ) != string::npos ) {
                pkgInput = pkgInput.replace( pkgInput.find(" "), 1, "," );
            }
            while ( pkgInput.find( "\t" ) != string::npos ) {
                pkgInput = pkgInput.replace( pkgInput.find("\t"), 1, "," );
            }
            while ( pkgInput.find( ",," ) != string::npos ) {
                pkgInput = pkgInput.replace( pkgInput.find(",,"), 2, "," );
            }
        }

        if (!filter) {
            alreadyChecked[path] = true;
        }

        list<string> packages;
        split( pkgInput, ',', packages );



        // TODO: think about whether it would be faster (more
        // efficient) to put all packages into a map, and the iterate
        // over the list of allowed packages and copy them
        // over. depending in the efficiency of find(), this might be
        // faster
        d = opendir( path.c_str() );
        while ( ( de = readdir( d ) ) != NULL ) {
            name = de->d_name;

            // TODO: review this
            struct stat buf;
            if ( stat( (path + "/" + name + "/Pkgfile").c_str(), &buf )
                 != 0 ) {
                // no Pkgfile -> no port
                continue;
            }

            if ( filter && find( packages.begin(),
                                 packages.end(), name ) == packages.end() ) {
                // not found -> ignore this port
                continue;
            }

            if ( name != "." && name != ".." ) {

                map<string, Package*>::iterator hidden;
                hidden = m_packageMap.find( name );
                Package* p = new Package( name, path );
                if ( p ) {
                    if ( hidden == m_packageMap.end() ) {
                        // no such package found, add
                        m_packageMap[name] = p;
                    } else if ( listDuplicate ) {
                        m_shadowedPackages[name] = make_pair( p, hidden->second );
                    } else {
                        delete p;
                    }
                }
            }
        }
        closedir( d );
    }
    
    parseDependencyList();
}

/*!
  Init from a cache file
  \param cacheFile the name of the cache file to be parser
  \return true on success, false indicates file opening problems
*/
Repository::CacheReadResult
Repository::initFromCache( const string& cacheFile )
{
    FILE* fp = fopen( cacheFile.c_str(), "r" );
    if ( !fp ) {
        return ACCESS_ERR;
    }

    const int length = BUFSIZ;
    char input[length];
    string line;

    // read version
    if ( fgets( input, length, fp ) ) {
        line = stripWhiteSpace( input );
        if ( line != CACHE_VERSION ) {
            return FORMAT_ERR;
        }
    }

    // FIELDS:
    // name, path, version, release,
    // description, dependencies, url,
    // packager, maintainer, hasReadme;
    // hasPreInstall, hasPostInstall
    const int fieldCount = 12;
    string fields[fieldCount];
    int fieldPos = 0;

    while ( fgets( input, length, fp ) ) {
        line = StringHelper::stripWhiteSpace( input );

        fields[fieldPos] = line;
        ++fieldPos;
        if ( fieldPos == fieldCount ) {
            fieldPos = 0;
            Package* p = new Package( fields[0], fields[1],
                                      fields[2], fields[3],
                                      fields[4], fields[5], fields[6],
                                      fields[7], fields[8], fields[9],
                                      fields[10], fields[11]);
            m_packageMap[p->name()] = p;
            fgets( input, length, fp ); // read empty line
        }
    }
    fclose( fp );

    parseDependencyList();
    
    return READ_OK;
}

/*!
  Store repository data in a cache file
  \param cacheFile the file where the data is stored
  \return whether the operation was successfully
*/
Repository::WriteResult Repository::writeCache( const string& cacheFile )
{
    string path = cacheFile;
    string::size_type pos = cacheFile.rfind( '/' );
    if ( pos != string::npos ) {
        path = path.erase( pos );
    }
    if ( !createOutputDir( path ) ) {
        return DIR_ERR;
    }

    FILE* fp = fopen( cacheFile.c_str(), "w" );
    if ( !fp ) {
        return FILE_ERR;
    }

    map<string, Package*>::const_iterator it = m_packageMap.begin();

    char yesStr[] = "yes";
    char noStr[] = "no";
    char* hasReadme;
    char* hasPreInstall;
    char* hasPostInstall;

    // write version
    fprintf( fp, "%s\n", CACHE_VERSION.c_str() );

    for ( ; it != m_packageMap.end(); ++it ) {
        const Package* p = it->second;

        // TODO: encode
        hasReadme = noStr;
        if ( p->hasReadme() ) {
            hasReadme = yesStr;
        }

        hasPreInstall = noStr;
        if ( p->hasPreInstall() ) {
            hasPreInstall = yesStr;
        }

        hasPostInstall = noStr;
        if ( p->hasPostInstall() ) {
            hasPostInstall = yesStr;
        }

        fprintf( fp, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n\n",
                 p->name().c_str(),
                 p->path().c_str(),
                 p->version().c_str(),

                 p->release().c_str(),
                 p->description().c_str(),
                 p->dependencies().c_str(),
                 p->url().c_str(),
                 p->packager().c_str(),
                 p->maintainer().c_str(),
                 hasReadme, hasPreInstall, hasPostInstall );
    }

    fclose( fp );
    return SUCCESS;
}

/*!
  create all components of \path which don't exist
  \param path the path to be created
  \return true on success. false indicates permission problems
 */
bool Repository::createOutputDir( const string& path )
{
    list<string> dirs;
    split( path, '/', dirs, 1 );
    string tmpPath;

    for ( list<string>::iterator it = dirs.begin(); it != dirs.end(); ++it ) {

        tmpPath += *it + "/";
        DIR* d;
        if ( ( d = opendir( tmpPath.c_str() ) ) == NULL ) {
            // doesn't exist
            if ( mkdir( tmpPath.c_str(), 0755 ) == -1 ) {
                cout << "- can't create output directory " << tmpPath
                     << endl;
                return false;
            }
        } else {
            closedir( d );
        }

    }
    return true;
}


/*!
  Search packages for a match of \a pattern in name. The name can
  contain shell wildcards.

  \param pattern the pattern to be found
  \return a list of matching packages
*/

void Repository::getMatchingPackages( const string& pattern,
                                      list<Package*>& target ) const
{
    map<string, Package*>::const_iterator it = m_packageMap.begin();
    RegEx re(pattern);

    if (m_useRegex) {
        for ( ; it != m_packageMap.end(); ++it ) {
            if (re.match(it->first)) {
                target.push_back( it->second );
            }
        }
    } else {
        for ( ; it != m_packageMap.end(); ++it ) {
            // I assume fnmatch will be quite fast for "match all" (*), so
            // I didn't add a boolean to check for this explicitely
            if ( fnmatch( pattern.c_str(), it->first.c_str(), 0  ) == 0 ) {
                target.push_back( it->second );
            }
        }
    }
}

void Repository::addDependencies( std::map<string, string>& deps )
{
    map<string, string>::iterator it = deps.begin();
    for ( ; it != deps.end(); ++it ) {
        map<string, Package*>::const_iterator pit =
            m_packageMap.find( it->first );
        if ( pit != m_packageMap.end() ) {
            Package* p = pit->second;
            if (p->dependencies().length() == 0) {
                // only use if no dependencies in Pkgfile
                p->setDependencies(it->second);
            }
        }
    }
}
