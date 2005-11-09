////////////////////////////////////////////////////////////////////////
// FILE:        repository.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#ifndef _REPOSITORY_H_
#define _REPOSITORY_H_

#include <string>
#include <list>
#include <map>
#include <utility>
using namespace std;

#include "package.h"

/*!
  \class Repository
  \brief Repository of available ports

  The repository is an abstraction of the available ports in the ports tree
*/
class Repository
{
public:
    Repository(bool useRegex);
    ~Repository();

    const Package* getPackage( const string& name ) const;
    const map<string, Package*>& packages() const;
    const map<string, pair<string, string> >& shadowedPackages() const;

    void searchMatchingPackages( const string& pattern,
                                 list<Package*>& target,
                                 bool searchDesc ) const;

    void getMatchingPackages( const string& pattern,
                              list<Package*>& target ) const;

    void initFromFS( const list< pair<string, string> >& rootList,
                     bool listDuplicate );


     /*! Result of a cache write operation */
    enum CacheReadResult {
        ACCESS_ERR,     /*!< Error creating/accessing the file */
        FORMAT_ERR,     /*!< bad/old format */
        READ_OK    /*!< Success */
    };
    CacheReadResult initFromCache( const string& cacheFile );

    /*! Result of a cache write operation */
    enum WriteResult {
        DIR_ERR,  /*!< Error creating/accessing the directory */
        FILE_ERR, /*!< Error creating/accessing the file */
        SUCCESS   /*!< Success */
    };
    WriteResult writeCache( const string& cacheFile );

    static bool createOutputDir( const string& path );
    void addDependencies( std::map<string, string>& deps );

    static const std::string EXTERNAL_DEPENDENCY_FILE;

private:
    static const std::string CACHE_VERSION;
    bool m_useRegex;
    
    void parseDependencyList();

    map<string, pair<string, string> > m_shadowedPackages;
    map<string, Package*> m_packageMap;
};

#endif /* _REPOSITORY_H_ */
