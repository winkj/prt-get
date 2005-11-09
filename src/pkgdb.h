////////////////////////////////////////////////////////////////////////
// FILE:        pkgdb.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#ifndef _PKGDB_H_
#define _PKGDB_H_

#include <map>
#include <utility>
#include <vector>
#include <string>


/*!
  \class PkgDB
  \brief database of installed packages

  A representation of crux' package database of installed packages
*/
class PkgDB
{
public:
    PkgDB( const std::string& installRoot = "" );
    bool isInstalled( const std::string& name,
                      bool useAlias = false,
                      bool* isAlias = 0,
                      string* aliasOrignalName = 0 ) const;


    std::string getPackageVersion( const std::string& name ) const;
    const std::map<std::string, std::string>& installedPackages();
    void getMatchingPackages( const std::string& pattern,
                              map<std::string,std::string>& target,
                              bool useRegex ) const;

    static const std::string ALIAS_STORE;

private:
    bool load() const;

    bool aliasExistsFor(const string& name, string& provider) const;

    mutable bool m_isLoaded;
    mutable std::map<std::string, std::string> m_packages;
    mutable std::map<std::string, std::string> m_aliases;
    mutable std::map<std::string, std::vector<std::string> > m_splitAliases;

    std::string m_installRoot;

    static const std::string PKGDB;
};

#endif /* _PKGDB_H_ */
